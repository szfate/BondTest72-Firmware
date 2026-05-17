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
static constexpr uint8_t YELLOW_R = 200, YELLOW_G = 150, YELLOW_B = 0;
static constexpr uint8_t GREEN_R  =   0, GREEN_G  = 255, GREEN_B  = 0;
static constexpr uint8_t RED_R    = 255, RED_G    =   0, RED_B    = 0;

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
        if (now - _lastDutPoll >= DUT_POLL_INTERVAL_MS) {
            handleDutEvent(_dutDetector.poll());
            _lastDutPoll = now;
        }
    }

    if (_state == State::READY && _buttons.startPressed())
        startTest();

    // Poll for adapter in NO_ADAPTER state
    if (_state == State::NO_ADAPTER) {
        if (now - _lastAdapterPoll >= ADAPTER_POLL_INTERVAL_MS) {
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
            _adapter->getSupportedPadmapId());
    }

    updateLeds();
}

void StateMachine::handleDutEvent(DutEvent ev) {
    switch (ev) {
        case DutEvent::INSERTED:
            _hostProtocol.sendDutInserted();
            transition(State::READY);
            break;
        case DutEvent::REMOVED:
            _hostProtocol.sendDutRemoved();
            if (_state != State::PASS && _state != State::FAIL)
                transition(State::ADAPTER_DETECTED);
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
        case HostCommand::DISCOVER:
            _hostProtocol.sendError("DISCOVER not implemented");
            break;
        case HostCommand::NONE:
            break;
    }
}

// ——————————————————————————————————————————————————————————————————————————

bool StateMachine::provisionEeprom() {
    EepromData d = {};
    d.adapterModel      = AdapterModel::Mezzanine70;
    d.adapterVersion    = 1;
    d.supportedPadmapId = EepromData::PADMAP_UNSET;
    d.designedLifespan  = 10000;
    d.dateOfManufacture = 0;
    d.insertionCount    = 0;
    d.testCount         = 0;
    d.eolReached        = 0x00;

    uint8_t buf[24];
    eepromSerialize(d, buf);
    if (!_eeprom.write(0, buf, 24)) { LOG_E("adapter: eeprom provision write failed"); return false; }
    LOG_I("adapter: eeprom provisioned (Mezzanine70 v1)");
    return true;
}

bool StateMachine::tryInitAdapter() {
    uint8_t buf[24];
    if (!_eeprom.read(0, buf, 24)) { LOG_E("adapter: eeprom read failed");         return false; }

    LOG_I("eeprom[0..7]: %02X %02X %02X %02X %02X %02X %02X %02X",
          buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

    // Blank EEPROM (never programmed) — provision with defaults and re-read.
    if (buf[0] == 0xFF && buf[1] == 0xFF) {
        LOG_W("adapter: eeprom blank, provisioning defaults");
        if (!provisionEeprom())              { return false; }
        if (!_eeprom.read(0, buf, 24))       { LOG_E("adapter: eeprom re-read after provision failed"); return false; }
    }

    EepromData data;
    if (!eepromDeserialize(buf, data)) { LOG_E("adapter: eeprom deserialize failed"); return false; }

    LOG_I("adapter: model=%u ver=%u padmap=%u ins=%lu tests=%lu",
          (uint8_t)data.adapterModel, data.adapterVersion, data.supportedPadmapId,
          data.insertionCount, data.testCount);

    _adapter = AdapterRegistry::create(data);
    if (!_adapter) { LOG_E("adapter: unknown model %u", (uint8_t)data.adapterModel); return false; }

    adapterSelfTest(_adapter);
    _dutDetector.setAdapter(_adapter);
    selectPadMap();
    LOG_I("adapter: init ok, padmap=%s", _padMap ? "set" : "null");
    return true;
}

void StateMachine::selectPadMap() {
    uint8_t hintId = _adapter->getSupportedPadmapId();
    if (hintId != EepromData::PADMAP_UNSET)
        _padMap = PadMapRegistry::find(hintId);
    if (!_padMap)
        _padMap = PadMapRegistry::all();
    _dutDetector.setPadMap(_padMap);
}

void StateMachine::checkAdapterAlive() {
    if (!_eeprom.isPresent())
        rp2040.reboot();
}

void StateMachine::startTest() {
    checkAdapterAlive();
    LOG_I("test start: slots=%u pads=%u", _adapter->getDutCount(), _padMap->caseCount);

    _hostProtocol.sendTestStart(
        (uint8_t)_adapter->getAdapterModel(),
        _adapter->getAdapterVersion(),
        _adapter->getSupportedPadmapId());

    transition(State::TESTING);
    updateLeds();

    _lastResult = _testRunner.run(*_adapter, *_padMap);

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
        uint8_t mezPin  = _padMap->cases[i].pad;
        uint8_t channel = _adapter->channelForPin(mezPin);
        for (uint8_t slot = 0; slot < _lastResult.slotCount; slot++)
            _hostProtocol.sendPadResult(slot, mezPin, _lastResult.slots[slot].pads[channel]);
    }
    _hostProtocol.sendSummary(_lastResult);
}

// ——————————————————————————————————————————————————————————————————————————

bool StateMachine::blinkOn(uint32_t periodMs) {
    return (millis() % periodMs) < (periodMs / 2);
}

void StateMachine::updateLeds() {
    _leds.clear();
    switch (_state) {
        case State::NO_ADAPTER:
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
