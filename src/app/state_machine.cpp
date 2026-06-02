#include "state_machine.h"
#include "adapter/adapter_registry.h"
#include "test/pad_map_registry.h"
#include "debug/log.h"
#include "debug/adapter_self_test.h"
#include <Arduino.h>

static const char* stateName(State s) {
    switch (s) {
        case State::NO_ADAPTER:        return "NO_ADAPTER";
        case State::ADAPTER_DETECTED:  return "ADAPTER_DETECTED";
        case State::READY:             return "READY";
        case State::EOL_ADAPTER:        return "EOL_ADAPTER";
        case State::WRONG_ORIENTATION: return "WRONG_ORIENTATION";
        case State::TESTING:           return "TESTING";
        case State::PASS:              return "PASS";
        case State::FAIL:              return "FAIL";
        case State::FAULT:             return "FAULT";
        default:                        return "?";
    }
}

StateMachine::StateMachine(MuxController&    mux,
                           AdcDriver&        adc,
                           SK6812Controller& leds,
                           Buttons&          buttons,
                           EepromManager&    eepromMgr,
                           DutDetector&      dutDetector,
                           TestRunner&       testRunner,
                           HostProtocol&     hostProtocol)
    : _mux(mux)
    , _adc(adc)
    , _ledManager(leds)
    , _buttons(buttons)
    , _eepromMgr(eepromMgr)
    , _dutDetector(dutDetector)
    , _testRunner(testRunner)
    , _hostProtocol(hostProtocol)
    , _discoveryScanner(mux, adc, hostProtocol)
{
}

void StateMachine::begin() {
    LOG_I("begin, eeprom present=%d", _eepromMgr.isPresent());
    if (_eepromMgr.isPresent()) {
        bool ok = tryInitAdapter();
        if (ok && _eepromData.eolReached == EepromData::EOL_REACHED) {
            LOG_W("adapter: EOL — rejecting");
            _state = State::EOL_ADAPTER;
        } else {
            if (ok) _dutDetector.poll();  // prime detector state — no event fired, no counter increment
            _state = ok ? State::ADAPTER_DETECTED : State::FAULT;
        }
        LOG_I("adapter init %s -> %s", ok ? "ok" : "FAILED", stateName(_state));
    }
    _ledManager.update(_state);
}

void StateMachine::update() {
    _buttons.poll();
    uint32_t now = millis();

    // Host commands
    HostCommand cmd = _hostProtocol.poll();
    if (cmd != HostCommand::NONE)
        handleCommand(cmd);

    // Adapter liveness (runtime — removal triggers reset)
    if (_state != State::NO_ADAPTER) {
        if (now - _lastAdapterPoll >= ADAPTER_POLL_INTERVAL_MS) {
            checkAdapterAlive();
            _lastAdapterPoll = now;
        }
    }

    if (_state == State::EOL_ADAPTER && _adapter)
        _adapter->tickEolLed();

    // DUT polling — all states except TESTING, NO_ADAPTER, EOL_ADAPTER, FAULT
    if (_state != State::TESTING &&
        _state != State::NO_ADAPTER &&
        _state != State::EOL_ADAPTER &&
        _state != State::FAULT) {
        if (now >= _dutSettleUntil && now - _lastDutPoll >= DUT_POLL_INTERVAL_MS) {
            handleDutEvent(_dutDetector.poll());
            _lastDutPoll = now;
        }
    }

    bool startReq = _buttons.startPressed();
    if (_state == State::READY && startReq)
        startTest();
    else if ((_state == State::PASS || _state == State::FAIL) &&
             startReq && _dutDetector.checkNow())
        startTest();

    // Poll for adapter in NO_ADAPTER state
    if (_state == State::NO_ADAPTER) {
        if (now - _lastAdapterPoll >= ADAPTER_POLL_INTERVAL_FAST_MS) {
            if (_eepromMgr.isPresent()) {
                if (tryInitAdapter()) {
                    if (_eepromData.eolReached == EepromData::EOL_REACHED) {
                        LOG_W("adapter: EOL — rejecting");
                        transition(State::EOL_ADAPTER);
                    } else {
                        transition(State::ADAPTER_DETECTED);
                    }
                } else {
                    transition(State::FAULT);
                }
            }
            _lastAdapterPoll = now;
        }
    }

    _ledManager.update(_state);
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

    _ledManager.update(_state);
}

void StateMachine::handleDutEvent(DutEvent ev) {
    switch (ev) {
        case DutEvent::INSERTED:
            _hostProtocol.sendDutInserted();
            _eepromData.insertionCount++;
            flushEeprom();
            if (_eepromData.eolReached == EepromData::EOL_REACHED)
                transition(State::EOL_ADAPTER);
            else
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
            if (!_eepromMgr.isPresent()) {
                _hostProtocol.sendError("no adapter");
            } else if (!provisionEeprom(_hostProtocol.provisionPadmapId(),
                                        _hostProtocol.provisionMfgDate())) {
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
        case HostCommand::DISCOVERY_SCAN:
            if (_state == State::TESTING) {
                _hostProtocol.sendError("busy");
            } else if (!_adapter || !_padMap) {
                _hostProtocol.sendError("no adapter");
            } else {
                _discoveryScanner.run();
            }
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
        case HostCommand::NONE:
            break;
    }
}

// ——————————————————————————————————————————————————————————————————————————

bool StateMachine::provisionEeprom(uint8_t padmapId, uint32_t timestamp) {
    EepromData d = {};
    d.adapterModel            = AdapterModel::Mezzanine70;
    d.adapterVersion          = 1;
    d.supportedPadmapIds[0]   = padmapId;
    d.supportedPadmapIds[1]   = EepromData::PADMAP_UNSET;
    d.supportedPadmapIds[2]   = EepromData::PADMAP_UNSET;
    d.supportedPadmapIds[3]   = EepromData::PADMAP_UNSET;
    d.designedLifespan        = 100;
    d.dateOfManufacture       = timestamp;
    d.insertionCount          = 0;
    d.testCount               = 0;
    d.eolReached              = 0x00;

    if (!_eepromMgr.write(d)) { LOG_E("adapter: eeprom provision write failed"); return false; }
    LOG_I("adapter: eeprom provisioned (Mezzanine70 v1)");
    return true;
}

bool StateMachine::tryInitAdapter() {
    auto result = _eepromMgr.read(_eepromData);

    if (result == EepromManager::ReadResult::Blank) {
        LOG_W("adapter: eeprom blank, provisioning defaults");
        if (!provisionEeprom())
            return false;
        result = _eepromMgr.read(_eepromData);
    }

    if (result != EepromManager::ReadResult::Ok) {
        LOG_E("adapter: eeprom read failed");
        return false;
    }

    LOG_I("adapter: model=%u ver=%u padmaps=[%u,%u,%u,%u] dom=%lu lifespan=%lu ins=%lu tests=%lu eol=%s",
          (uint8_t)_eepromData.adapterModel, _eepromData.adapterVersion,
          _eepromData.supportedPadmapIds[0], _eepromData.supportedPadmapIds[1],
          _eepromData.supportedPadmapIds[2], _eepromData.supportedPadmapIds[3],
          _eepromData.dateOfManufacture,
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
    if (_adapter && _eepromData.eolReached == EepromData::EOL_REACHED) {
        _adapter->setEolLed(true);
    }
    if (!_eepromMgr.write(_eepromData)) {
        _hostProtocol.sendError("eeprom write failed");
    }
}

bool StateMachine::checkAdapterAlive() {
    if (_eepromMgr.isPresent()) return true;
    LOG_W("adapter removed");
    if (_adapter) _adapter->setEolLed(false);
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
    _ledManager.update(_state);

    _lastResult = _testRunner.run(*_adapter, *_padMap);
    _eepromData.testCount++;
    flushEeprom();

    static const char* const outcomeStr[] = { "PASS", "FAIL", "FAIL_DUT_REMOVED", "WRONG_ORIENTATION" };
    uint8_t oi = (uint8_t)_lastResult.outcome;
    LOG_I("test done: outcome=%s", oi < 4 ? outcomeStr[oi] : "?");
    for (uint8_t s = 0; s < _lastResult.slotCount; s++)
        LOG_I("  slot %u: %u/%u good", s, _lastResult.slots[s].goodCount, _lastResult.slots[s].testedCount);

    sendResults();
    if (_eepromData.eolReached == EepromData::EOL_REACHED)
        transition(State::EOL_ADAPTER);
    else
        transition(_lastResult.outcome == TestOutcome::PASS ? State::PASS : State::FAIL);
}

void StateMachine::sendResults() {
    if (!_adapter || !_padMap) return;
    for (uint8_t slot = 0; slot < _lastResult.slotCount; slot++) {
        const SlotResult& sr = _lastResult.slots[slot];
        _hostProtocol.sendSlotStatus(slot, sr.present, sr.tested);
        if (!sr.tested) continue;
        for (uint8_t i = 0; i < _padMap->caseCount; i++) {
            const TestCase& tc = _padMap->cases[i];
            if (tc.strategy == TestStrategy::DISCHARGE ||
                tc.strategy == TestStrategy::PRECHARGE) continue;
            uint8_t channel = _adapter->channelForPin(tc.mezPin);
            _hostProtocol.sendPadResult(slot, tc.mezPin, tc.diePad, sr.byChannel[channel]);
        }
    }
    _hostProtocol.sendSummary(_lastResult);
}
