#include "state_machine.h"
#include "../adapter/adapter_registry.h"
#include "../test/pad_map_registry.h"
#include "../debug/log.h"
#include "../debug/adapter_self_test.h"
#include <Arduino.h>

static const char* stateName(StateMachine::State s) {
    switch (s) {
        case StateMachine::State::NO_ADAPTER:        return "NO_ADAPTER";
        case StateMachine::State::ADAPTER_DETECTED:  return "ADAPTER_DETECTED";
        case StateMachine::State::READY:             return "READY";
        case StateMachine::State::WRONG_ORIENTATION: return "WRONG_ORIENTATION";
        case StateMachine::State::TESTING:           return "TESTING";
        case StateMachine::State::PASS:              return "PASS";
        case StateMachine::State::FAIL:              return "FAIL";
        case StateMachine::State::FAULT:             return "FAULT";
        default:                                     return "?";
    }
}

// LED colours
static constexpr uint8_t YELLOW_R = 180, YELLOW_G =  50, YELLOW_B = 0;
static constexpr uint8_t GREEN_R  =   0, GREEN_G  = 255, GREEN_B  = 0;
static constexpr uint8_t RED_R    = 255, RED_G    =   0, RED_B    = 0;
static constexpr uint8_t DIM_RED_R =  50, DIM_RED_G =  0, DIM_RED_B = 0;

// Blink periods
static constexpr uint32_t BLINK_SLOW_MS = 800;
static constexpr uint32_t BLINK_FAST_MS = 200;

StateMachine::StateMachine(MuxController&    mux,
                           AdcDriver&        adc,
                           SK6812Controller& leds,
                           Buttons&          buttons,
                           AT21CS01Driver&   eeprom,
                           DutDetector&      dutDetector,
                           TestRunner&       testRunner,
                           HostProtocol&     hostProtocol)
    : _mux(mux)
    , _adc(adc)
    , _leds(leds)
    , _buttons(buttons)
    , _eeprom(eeprom)
    , _dutDetector(dutDetector)
    , _testRunner(testRunner)
    , _hostProtocol(hostProtocol)
{
}

void StateMachine::begin() {
    LOG_I("begin, eeprom present=%d", _eeprom.isPresent());
    if (_eeprom.isPresent()) {
        bool ok = tryInitAdapter();
        if (ok && _eepromData.eolReached == EepromData::EOL_REACHED) {
            LOG_W("adapter: EOL — refusing operation");
            _adapter->setEolLed(true);
            ok = false;
        }
        if (ok) _dutDetector.poll();  // prime detector state — no event fired, no counter increment
        _state = ok ? State::ADAPTER_DETECTED : State::FAULT;
        LOG_I("adapter init %s -> %s", ok ? "ok" : "FAILED", stateName(_state));
    }
    updateLeds();
}

void StateMachine::update() {
    _buttons.poll();
    uint32_t now = millis();

    // Host commands
    HostCommand cmd = _hostProtocol.poll();
    if (cmd != HostCommand::NONE)
        handleCommand(cmd);

    // Adapter liveness (runtime — removal triggers reset)
    if (_state != State::NO_ADAPTER && _state != State::FAULT) {
        if (now - _lastAdapterPoll >= ADAPTER_POLL_INTERVAL_MS) {
            checkAdapterAlive();
            _lastAdapterPoll = now;
        }
    }

    // DUT polling — all states except TESTING, NO_ADAPTER, FAULT
    if (_state != State::TESTING &&
        _state != State::NO_ADAPTER &&
        _state != State::FAULT) {
        if (now >= _dutSettleUntil && now - _lastDutPoll >= DUT_POLL_INTERVAL_MS) {
            handleDutEvent(_dutDetector.poll());
            _lastDutPoll = now;
        }
    }

    if (_state == State::READY && _buttons.startPressed())
        startTest();
    else if ((_state == State::PASS || _state == State::FAIL) &&
             _buttons.startPressed() && _dutDetector.checkNow())
        startTest();

    // Poll for adapter in NO_ADAPTER state
    if (_state == State::NO_ADAPTER) {
        if (now - _lastAdapterPoll >= ADAPTER_POLL_INTERVAL_FAST_MS) {
            if (_eeprom.isPresent())
                transition(tryInitAdapter() ? State::ADAPTER_DETECTED : State::FAULT);
            _lastAdapterPoll = now;
        }
    }

    updateLeds();
}

// ——————————————————————————————————————————————————————————————————————————

void StateMachine::transition(State next) {
    LOG_I("state %s -> %s", stateName(_state), stateName(next));
    _state = next;

    if (next == State::ADAPTER_DETECTED && _adapter) {
        _hostProtocol.sendAdapterDetected(
            (uint8_t)_adapter->getAdapterModel(),
            _adapter->getAdapterVersion(),
            _adapter->getSupportedPadmapIds());
    }

    updateLeds();
}

void StateMachine::handleDutEvent(DutEvent ev) {
    switch (ev) {
        case DutEvent::INSERTED:
            _hostProtocol.sendDutInserted();
            _eepromData.insertionCount++;
            flushEeprom();
            transition(State::READY);
            _dutSettleUntil = millis() + DUT_INSERT_SETTLE_MS;
            break;
        case DutEvent::REMOVED:
            _hostProtocol.sendDutRemoved();
            _buttons.startPressed();  // discard any press that arrived while DUT was in
            if (_state != State::PASS && _state != State::FAIL)
                transition(State::ADAPTER_DETECTED);
            if (_adapter && _padMap)
                _adapter->connectorIsolationSweep(_mux, _adc, *_padMap);
            break;
        case DutEvent::WRONG_ORIENTATION:
            transition(State::WRONG_ORIENTATION);
            break;
        case DutEvent::NONE:
            break;
    }
}

void StateMachine::handleCommand(HostCommand cmd) {
    switch (cmd) {
        case HostCommand::RUN:
            if (_state == State::READY) startTest();
            break;
        case HostCommand::GET_RESULTS:
            sendResults();
            break;
        case HostCommand::SET_PADMAP: {
            const PadMap* map = PadMapRegistry::find(_hostProtocol.setPadmapId());
            if (map) {
                _padMap = map;
                _dutDetector.setPadMap(_padMap);
            } else {
                _hostProtocol.sendError("unknown pad map id");
            }
            break;
        }
        case HostCommand::PROVISION:
            if (!_eeprom.isPresent()) {
                _hostProtocol.sendError("no adapter");
            } else if (!provisionEeprom(_hostProtocol.provisionPadmapId())) {
                _hostProtocol.sendError("provision write failed");
            } else {
                _adapter = nullptr; _padMap = nullptr;
                transition(tryInitAdapter() ? State::ADAPTER_DETECTED : State::FAULT);
                Serial.println("OK PROVISION");
            }
            break;
        case HostCommand::DISCOVER:
            _hostProtocol.sendError("DISCOVER not implemented");
            break;
        case HostCommand::GET_ADAPTER:
            if (!_adapter) {
                _hostProtocol.sendError("no adapter");
            } else {
                _hostProtocol.sendAdapterInfo(
                    (uint8_t)_eepromData.adapterModel,
                    _eepromData.adapterVersion,
                    _eepromData.supportedPadmapIds,
                    _eepromData.designedLifespan,
                    _eepromData.dateOfManufacture,
                    _eepromData.insertionCount,
                    _eepromData.testCount,
                    _eepromData.eolReached == EepromData::EOL_REACHED,
                    _padMap ? _padMap->name : nullptr);
            }
            break;
        case HostCommand::DEBUG_PWR_SWEEP:
            debugPwrSweep(_hostProtocol.debugPadMapId());
            break;
        case HostCommand::DEBUG_BIASED_SWEEP:
            debugBiasedSweep(_hostProtocol.debugPadMapId());
            break;
        case HostCommand::NONE:
            break;
    }
}

// ——————————————————————————————————————————————————————————————————————————

bool StateMachine::provisionEeprom(uint8_t padmapId) {
    EepromData d = {};
    d.adapterModel            = AdapterModel::Mezzanine70;
    d.adapterVersion          = 1;
    d.supportedPadmapIds[0]   = padmapId;
    d.supportedPadmapIds[1]   = EepromData::PADMAP_UNSET;
    d.supportedPadmapIds[2]   = EepromData::PADMAP_UNSET;
    d.supportedPadmapIds[3]   = EepromData::PADMAP_UNSET;
    d.designedLifespan        = 10000;
    d.dateOfManufacture = 0;
    d.insertionCount    = 0;
    d.testCount         = 0;
    d.eolReached        = 0x00;

    uint8_t buf[36];
    eepromSerialize(d, buf);
    if (!_eeprom.write(0, buf, 36)) { LOG_E("adapter: eeprom provision write failed"); return false; }
    LOG_I("adapter: eeprom provisioned (Mezzanine70 v1)");
    return true;
}

bool StateMachine::tryInitAdapter() {
    uint8_t buf[36];
    if (!_eeprom.read(0, buf, 36)) { LOG_E("adapter: eeprom read failed");         return false; }

    LOG_I("eeprom[0..7]: %02X %02X %02X %02X %02X %02X %02X %02X",
          buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

    // Blank EEPROM (never programmed) — provision with defaults and re-read.
    if (buf[0] == 0xFF && buf[1] == 0xFF) {
        LOG_W("adapter: eeprom blank, provisioning defaults");
        if (!provisionEeprom())              { return false; }
        if (!_eeprom.read(0, buf, 36))       { LOG_E("adapter: eeprom re-read after provision failed"); return false; }
    }

    if (!eepromDeserialize(buf, _eepromData)) { LOG_E("adapter: eeprom deserialize failed"); return false; }

    LOG_I("adapter: model=%u ver=%u padmaps=[%u,%u,%u,%u] lifespan=%lu ins=%lu tests=%lu eol=%s",
          (uint8_t)_eepromData.adapterModel, _eepromData.adapterVersion,
          _eepromData.supportedPadmapIds[0], _eepromData.supportedPadmapIds[1],
          _eepromData.supportedPadmapIds[2], _eepromData.supportedPadmapIds[3],
          _eepromData.designedLifespan, _eepromData.insertionCount, _eepromData.testCount,
          _eepromData.eolReached == EepromData::EOL_REACHED ? "YES" : "no");

    _adapter = AdapterRegistry::create(_eepromData);
    if (!_adapter) { LOG_E("adapter: unknown model %u", (uint8_t)_eepromData.adapterModel); return false; }

    adapterSelfTest(_adapter);
    _dutDetector.setAdapter(_adapter);
    selectPadMap();
    if (_padMap)
        _adapter->connectorIsolationSweep(_mux, _adc, *_padMap);
    LOG_I("adapter: init ok, padmap=%s", _padMap ? "set" : "null");
    return true;
}

void StateMachine::selectPadMap() {
    const uint8_t* ids = _adapter->getSupportedPadmapIds();
    for (uint8_t i = 0; i < 4 && ids[i] != EepromData::PADMAP_UNSET; i++) {
        _padMap = PadMapRegistry::find(ids[i]);
        if (_padMap) break;
    }
    if (!_padMap)
        _padMap = PadMapRegistry::all();
    _dutDetector.setPadMap(_padMap);
}

void StateMachine::flushEeprom() {
    // Only mutable fields are ever modified: insertionCount, testCount, eolReached.
    // Read-only fields (model, version, padmapId, lifespan, dateOfManufacture) are
    // loaded once in tryInitAdapter() and never changed, so a full write is safe.
    if (_eepromData.insertionCount >= _eepromData.designedLifespan &&
        _eepromData.eolReached != EepromData::EOL_REACHED) {
        _eepromData.eolReached = EepromData::EOL_REACHED;
        LOG_W("adapter: EOL reached (%lu insertions)", _eepromData.insertionCount);
    }
    if (_adapter && _eepromData.eolReached == EepromData::EOL_REACHED)
        _adapter->setEolLed(true);
    uint8_t buf[36];
    eepromSerialize(_eepromData, buf);
    if (!_eeprom.write(0, buf, 36))
        LOG_E("adapter: eeprom write failed");
}

bool StateMachine::checkAdapterAlive() {
    if (_eeprom.isPresent()) return true;
    LOG_W("adapter removed");
    _adapter = nullptr;
    _padMap  = nullptr;
    _dutDetector.setAdapter(nullptr);
    transition(State::NO_ADAPTER);
    return false;
}

void StateMachine::startTest() {
    if (!checkAdapterAlive()) return;
    LOG_I("test start: slots=%u pads=%u", _adapter->getDutCount(), _padMap->caseCount);

    _hostProtocol.sendTestStart(
        (uint8_t)_adapter->getAdapterModel(),
        _adapter->getAdapterVersion(),
        _adapter->getSupportedPadmapIds());

    transition(State::TESTING);
    updateLeds();

    _lastResult = _testRunner.run(*_adapter, *_padMap);
    _eepromData.testCount++;
    flushEeprom();

    static const char* const outcomeStr[] = { "PASS", "FAIL", "FAIL_DUT_REMOVED", "WRONG_ORIENTATION" };
    uint8_t oi = (uint8_t)_lastResult.outcome;
    LOG_I("test done: outcome=%s", oi < 4 ? outcomeStr[oi] : "?");
    for (uint8_t s = 0; s < _lastResult.slotCount; s++)
        LOG_I("  slot %u: %u/%u good", s, _lastResult.slots[s].goodCount, _lastResult.slots[s].testedCount);

    sendResults();
    transition(_lastResult.outcome == TestOutcome::PASS ? State::PASS : State::FAIL);
}

void StateMachine::sendResults() {
    if (!_padMap) return;
    for (uint8_t i = 0; i < _padMap->caseCount; i++) {
        uint8_t mezPin  = _padMap->cases[i].mezPin;
        uint8_t channel = _adapter->channelForPin(mezPin);
        for (uint8_t slot = 0; slot < _lastResult.slotCount; slot++)
            _hostProtocol.sendPadResult(slot, mezPin, _lastResult.slots[slot].byChannel[channel]);
    }
    _hostProtocol.sendSummary(_lastResult);
}

// ——————————————————————————————————————————————————————————————————————————

static const char* padTypeName(PadType t) {
    switch (t) {
        case PadType::VDDIO:    return "VDDIO";
        case PadType::VDD_CORE: return "VDD_CORE";
        case PadType::PWR_AUX:  return "PWR_AUX";
        default:                return "?";
    }
}

void StateMachine::debugPwrSweep(uint8_t padMapId) {
    const PadMap* pm = PadMapRegistry::find(padMapId);
    if (!pm) { LOG_E("padmap %u not found", padMapId); return; }

    LOG_I("=== pwr sweep: %s ===", pm->name);

    // Collect unique power mez pins (skip DISCHARGE duplicates)
    uint8_t pwrPins[16];
    PadType pwrTypes[16];
    uint8_t pwrCount = 0;
    uint8_t gndPin = pm->cases[0].gndPin;

    for (uint8_t i = 0; i < pm->caseCount; i++) {
        const TestCase& tc = pm->cases[i];
        if (tc.padType == PadType::IO || tc.padType == PadType::GND) continue;
        if (tc.strategy == TestStrategy::DISCHARGE) continue;
        bool seen = false;
        for (uint8_t j = 0; j < pwrCount; j++) {
            if (pwrPins[j] == tc.mezPin) { seen = true; break; }
        }
        if (!seen && pwrCount < 16) {
            pwrPins[pwrCount]  = tc.mezPin;
            pwrTypes[pwrCount] = tc.padType;
            pwrCount++;
        }
    }

    if (pwrCount == 0) { LOG_E("no power pads in pad map"); return; }
    LOG_I("  %u power pins, gnd=mez%02u", pwrCount, gndPin);

    for (uint8_t i = 0; i < pwrCount; i++) {
        uint8_t pwrPin = pwrPins[i];
        for (uint8_t dir = 0; dir < 2; dir++) {
            LOG_I("--- mez%02u (%s) %s, probing all ---",
                  pwrPin, padTypeName(pwrTypes[i]),
                  dir == 0 ? "->D" : "->B");

            // Discharge: sink all power pins to GND
            _mux.clearAll();
            for (uint8_t j = 0; j < pwrCount; j++)
                _mux.setChannel(pwrPins[j] - 1, Bus::B);
            delay(100);
            _mux.clearAll();

            for (uint8_t probe = 1; probe <= 70; probe++) {
                if (probe == pwrPin) continue;
                _mux.clearAll();
                if (dir == 0) {
                    _mux.setChannel(pwrPin - 1, Bus::D);
                    _mux.setChannel(probe  - 1, Bus::B);
                } else {
                    _mux.setChannel(probe  - 1, Bus::D);
                    _mux.setChannel(pwrPin - 1, Bus::B);
                }
                delay(5);
                float v = _adc.readVoltage(0);
                _mux.clearAll();
                if (v < 3.0f)
                    LOG_I("  probe mez%02u  sense=%.3fV", probe, v);
                delay(20);
            }
        }
    }

    LOG_I("=== pwr sweep done ===");
}

void StateMachine::debugBiasedSweep(uint8_t padMapId) {
    const PadMap* pm = PadMapRegistry::find(padMapId);
    if (!pm) { LOG_E("padmap %u not found", padMapId); return; }
    LOG_I("=== biased sweep: %s (%u cases) ===", pm->name, pm->caseCount);
    for (uint8_t i = 0; i < pm->caseCount; i++) {
        const TestCase& tc = pm->cases[i];
        uint8_t mezCh = tc.mezPin - 1;
        uint8_t gndCh = tc.gndPin - 1;

        _mux.clearAll();
        _mux.setChannel(mezCh, Bus::D);
        _mux.setChannel(gndCh, Bus::B);
        delay(1);
        float fwd = _adc.readVoltage(0);

        _mux.clearAll();
        _mux.setChannel(gndCh, Bus::D);
        _mux.setChannel(mezCh, Bus::B);
        delay(1);
        float rev = _adc.readVoltage(0);

        _mux.clearAll();
        LOG_I("mez%02u gnd%02u  fwd=%.3fV  rev=%.3fV  delta=%.3fV",
              tc.mezPin, tc.gndPin, fwd, rev, fwd - rev);
    }
    LOG_I("=== biased sweep done ===");
}

// ——————————————————————————————————————————————————————————————————————————

bool StateMachine::blinkOn(uint32_t periodMs) {
    return (millis() % periodMs) < (periodMs / 2);
}

void StateMachine::updateLeds() {
    _leds.clear();
    switch (_state) {
        case State::NO_ADAPTER:
            _leds.setAll(DIM_RED_R, DIM_RED_G, DIM_RED_B);
            break;
        case State::ADAPTER_DETECTED:
            if (blinkOn(BLINK_SLOW_MS)) _leds.setPixel(0, YELLOW_R, YELLOW_G, YELLOW_B);
            break;
        case State::READY:
            _leds.setPixel(0, YELLOW_R, YELLOW_G, YELLOW_B);
            break;
        case State::TESTING:
            _leds.setPixel(0, YELLOW_R, YELLOW_G, YELLOW_B);
            break;
        case State::WRONG_ORIENTATION:
            if (blinkOn(BLINK_SLOW_MS)) _leds.setPixel(2, RED_R, RED_G, RED_B);
            break;
        case State::PASS:
            _leds.setPixel(1, GREEN_R, GREEN_G, GREEN_B);
            break;
        case State::FAIL:
            _leds.setPixel(2, RED_R, RED_G, RED_B);
            break;
        case State::FAULT:
            if (blinkOn(BLINK_FAST_MS)) _leds.setPixel(2, RED_R, RED_G, RED_B);
            break;
    }
    _leds.show();
}
