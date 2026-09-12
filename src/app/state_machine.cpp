#include "state_machine.h"
#include "hal/mux.h"
#include "hal/adc.h"
#include "hal/buttons.h"
#include "adapter/adapter_base.h"
#include "adapter/adapter_registry.h"
#include "test/dut_detector.h"
#include "test/test_runner.h"
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
{
}

void StateMachine::begin() {
    LOG_I("begin, eeprom present=%d", _eepromMgr.isPresent());
    if (_eepromMgr.isPresent()) {
        bool ok = tryInitAdapter();
        if (ok && _eepromData.eolReached != EepromData::EOL_REACHED)
            _dutDetector.prime();  // sync detector with a DUT already seated at power-on
        handleAdapterArrival(ok);
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

    pollAdapterLiveness(now);
    pollDut(now);
    // Consumed unconditionally every cycle (NOT inside pollDut — startPressed()
    // is a consume-on-read latch, and gating it would let a press during a test
    // linger and fire a surprise test right after PASS). tryStartTest() no-ops
    // outside READY/PASS/FAIL on its own.
    if (_buttons.startPressed())
        tryStartTest();
    pollAdapterInsertion(now);

    _ledManager.update(_state);
}

void StateMachine::pollAdapterLiveness(uint32_t now) {
    if (_state != State::NO_ADAPTER) {
        if (now - _lastAdapterLivePoll >= ADAPTER_POLL_INTERVAL_MS) {
            checkAdapterAlive();
            _lastAdapterLivePoll = now;
        }
    }

    // Keep the EOL LED blinking even after the adapter is rejected so the operator sees the warning
    if (_state == State::EOL_ADAPTER && _adapter)
        _adapter->tickEolLed();
}

void StateMachine::pollDut(uint32_t now) {
    // DUT polling — all states except TESTING, NO_ADAPTER, EOL_ADAPTER, FAULT
    if (_state == State::TESTING ||
        _state == State::NO_ADAPTER ||
        _state == State::EOL_ADAPTER ||
        _state == State::FAULT) {
        return;
    }
    // _dutSettleUntil suppresses re-polling for a short window after insertion to absorb connector bounce
    if (now >= _dutSettleUntil && now - _lastDutPoll >= DUT_POLL_INTERVAL_MS) {
        handleDutEvent(_dutDetector.poll());
        _lastDutPoll = now;
    }
}

void StateMachine::pollAdapterInsertion(uint32_t now) {
    // Poll for adapter in NO_ADAPTER state. On first detection, wait out
    // ADAPTER_INSERT_SETTLE_MS before the first EEPROM read: a half-seated
    // connector makes early reads fail or return garbage (a floating SWI line
    // reads as 0xFF, which looks like a blank header → spurious
    // ADAPTER_NOT_PROVISIONED). Init failure retries on the next poll instead of
    // latching FAULT — insertion bounce must not wedge the tester. Only a
    // genuinely blank adapter (read succeeded, header really is blank) latches.
    if (_state != State::NO_ADAPTER) return;
    if (now - _lastAdapterInsertPoll >= ADAPTER_POLL_INTERVAL_FAST_MS) {
        _lastAdapterInsertPoll = now;
        if (_eepromMgr.isPresent()) {
            if (_adapterSettleUntil == 0)
                _adapterSettleUntil = now + ADAPTER_INSERT_SETTLE_MS;  // just detected — let it seat first
            else if (now >= _adapterSettleUntil) {
                if (tryInitAdapter()) {
                    _adapterSettleUntil = 0;
                    handleAdapterArrival(true);
                } else if (_lastEepromResult == EepromManager::ReadResult::Blank) {
                    _adapterSettleUntil = 0;
                    handleAdapterArrival(false);  // fully seated but unprovisioned — permanent
                } else {
                    _adapterSettleUntil = 0;  // transient read failure — retry after another settle
                    LOG_W("adapter: init failed during detection, retrying");
                }
            }
        } else {
            _adapterSettleUntil = 0;  // contact lost before settle elapsed (bounce) — start over
        }
    }
}

// ——————————————————————————————————————————————————————————————————————————

void StateMachine::transition(State next) {
    LOG_I("state %s -> %s", stateName(_state), stateName(next));

    if (next == State::EOL_ADAPTER) {
        _hostProtocol.sendEolWarning(_eepromData.insertionCount);
    }

    _state = next;

    // Announce adapter details on ADAPTER_DETECTED rather than on READY, so the host
    // knows the model before the DUT is inserted and can validate the pad map selection.
    if (next == State::ADAPTER_DETECTED && _adapter) {
        _hostProtocol.sendAdapterDetected(
            (uint8_t)_adapter->getAdapterHardware(),
            _adapter->getSupportedPadmapIds());
    }

    _ledManager.update(_state);
}

void StateMachine::handleAdapterArrival(bool initOk) {
    // Shared tail of adapter arrival — used by boot (begin) and the runtime
    // insertion poll. Healthy arrivals go to ADAPTER_DETECTED; EOL adapters are
    // rejected with an EVENT EOL_WARNING; init failure is fatal (FAULT). Callers
    // that want to retry transient failures (the runtime insertion path) must
    // handle those before calling this with false.
    if (!initOk) {
        _hostProtocol.sendFault("ADAPTER_INIT_FAILED");
        transition(State::FAULT);
        return;
    }
    if (_eepromData.eolReached == EepromData::EOL_REACHED) {
        LOG_W("adapter: EOL — rejecting");
        transition(State::EOL_ADAPTER);  // sends EVENT EOL_WARNING to the host
    } else {
        transition(State::ADAPTER_DETECTED);
    }
}

void StateMachine::handleDutEvent(DutEvent ev) {
    switch (ev) {
        case DutEvent::INSERTED:
            _hostProtocol.sendDutInserted();
            _eepromData.insertionCount++;
            flushEeprom();  // flushEeprom sets EOL_REACHED flag if the new count hit the lifespan limit
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
            _hostProtocol.sendWrongOrientation();
            transition(State::WRONG_ORIENTATION);
            break;
        case DutEvent::NONE:
            break;
    }
}

// Per-state command permission table (docs/BONDTEST72_HOST_PROTOCOL.md
// "State Machine"), enforced in handleCommand. HELLO and GET_ADAPTER are pure
// queries — valid in every state (provision.py and live_serial_viewer.py both
// GET_ADAPTER from FAULT/NO_ADAPTER). PROVISION is allowed everywhere except
// TESTING: a fresh blank adapter latches FAULT, and provision.py works from
// there — the old spec table's "EOL_ADAPTER only" row was wrong. The
// SET_PADMAP gate closes B3's root cause (mid-test padmap swap invalidating
// the result's pad labels).
constexpr uint16_t cmdBit(HostCommand c) { return 1u << static_cast<uint8_t>(c); }

static constexpr uint16_t CMD_MASK[static_cast<uint8_t>(State::COUNT)] = {
    /* NO_ADAPTER        */ cmdBit(HostCommand::HELLO) | cmdBit(HostCommand::GET_ADAPTER) | cmdBit(HostCommand::PROVISION),
    /* EOL_ADAPTER       */ cmdBit(HostCommand::HELLO) | cmdBit(HostCommand::GET_ADAPTER) | cmdBit(HostCommand::PROVISION),
    /* ADAPTER_DETECTED  */ cmdBit(HostCommand::HELLO) | cmdBit(HostCommand::GET_ADAPTER) | cmdBit(HostCommand::SET_PADMAP) |
                            cmdBit(HostCommand::GET_RESULTS) | cmdBit(HostCommand::PROVISION),
    /* READY             */ cmdBit(HostCommand::HELLO) | cmdBit(HostCommand::GET_ADAPTER) | cmdBit(HostCommand::RUN) |
                            cmdBit(HostCommand::SET_PADMAP) | cmdBit(HostCommand::GET_RESULTS) |
                            cmdBit(HostCommand::PROVISION),
    /* WRONG_ORIENTATION */ cmdBit(HostCommand::HELLO) | cmdBit(HostCommand::GET_ADAPTER),
    /* TESTING           */ cmdBit(HostCommand::HELLO) | cmdBit(HostCommand::GET_ADAPTER),
    /* PASS              */ cmdBit(HostCommand::HELLO) | cmdBit(HostCommand::GET_ADAPTER) | cmdBit(HostCommand::RUN) | cmdBit(HostCommand::GET_RESULTS),
    /* FAIL              */ cmdBit(HostCommand::HELLO) | cmdBit(HostCommand::GET_ADAPTER) | cmdBit(HostCommand::RUN) | cmdBit(HostCommand::GET_RESULTS),
    /* FAULT             */ cmdBit(HostCommand::HELLO) | cmdBit(HostCommand::GET_ADAPTER) | cmdBit(HostCommand::PROVISION),
};

void StateMachine::handleCommand(HostCommand cmd) {
    // PROVISION_INVALID only reports a malformed PROVISION line — same gate as PROVISION.
    HostCommand gated = cmd == HostCommand::PROVISION_INVALID ? HostCommand::PROVISION : cmd;
    if (!(CMD_MASK[static_cast<uint8_t>(_state)] & cmdBit(gated))) {
        _hostProtocol.sendError(ErrorCode::WRONG_STATE, stateName(_state));
        return;
    }

    switch (cmd) {
        case HostCommand::RUN:
            tryStartTest();
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
                _hostProtocol.sendError(ErrorCode::UNKNOWN_PADMAP, "UNKNOWN_PADMAP");
            }
            break;
        }
        case HostCommand::PROVISION:
            if (!_eepromMgr.isPresent()) {
                _hostProtocol.sendError(ErrorCode::NO_ADAPTER, "NO_ADAPTER");
            } else if (_hostProtocol.provisionHwId() == EepromData::HWID_UNSET) {
                _hostProtocol.sendError(ErrorCode::MISSING_FIELD, "MISSING_HW");
            } else if (_hostProtocol.provisionPadmapIds()[0] == EepromData::PADMAP_UNSET) {
                _hostProtocol.sendError(ErrorCode::MISSING_FIELD, "MISSING_PADMAP");
            } else if (_hostProtocol.provisionLifespan() == EepromData::FIELD_UNSET) {
                _hostProtocol.sendError(ErrorCode::MISSING_FIELD, "MISSING_LIFESPAN");
            } else if (_hostProtocol.provisionMfgDate() == EepromData::FIELD_UNSET) {
                _hostProtocol.sendError(ErrorCode::MISSING_FIELD, "MISSING_DATE");
            } else if (!provisionEeprom(_hostProtocol.provisionHwId(),
                                        _hostProtocol.provisionPadmapIds(),
                                        _hostProtocol.provisionLifespan(),
                                        _hostProtocol.provisionMfgDate(),
                                        _hostProtocol.provisionIns(),
                                        _hostProtocol.provisionTests(),
                                        _hostProtocol.provisionEol())) {
                _hostProtocol.sendError(ErrorCode::PROVISION_FAILED, "PROVISION_FAILED");
            } else {
                _adapter = nullptr; _padMap = nullptr;
                transition(tryInitAdapter() ? State::ADAPTER_DETECTED : State::FAULT);
                _hostProtocol.sendOk("PROVISION");
            }
            break;
        case HostCommand::PROVISION_INVALID:
            _hostProtocol.sendError(ErrorCode::MISSING_FIELD, "MISSING_PADMAP");
            break;
        case HostCommand::GET_ADAPTER:
            if (!_adapter) {
                if (_eepromMgr.isPresent() && _lastEepromResult == EepromManager::ReadResult::Blank)
                    _hostProtocol.sendError(ErrorCode::ADAPTER_NOT_PROVISIONED, "ADAPTER_NOT_PROVISIONED");
                else
                    _hostProtocol.sendError(ErrorCode::NO_ADAPTER, "NO_ADAPTER");
            } else {
                _hostProtocol.sendAdapterInfo(
                    (uint8_t)_eepromData.adapterHardware,
                    _eepromData.supportedPadmapIds,
                    _eepromData.designedLifespan,
                    _eepromData.dateOfManufacture,
                    _eepromData.insertionCount,
                    _eepromData.testCount,
                    _eepromData.eolReached == EepromData::EOL_REACHED,
                    _dutDetector.dutPresent());
            }
            break;
        case HostCommand::HELLO:
            _hostProtocol.sendHello();
            break;
        case HostCommand::NONE:
            break;
    }
}

// ——————————————————————————————————————————————————————————————————————————

bool StateMachine::provisionEeprom(uint8_t hwId, const uint8_t padmapIds[4], uint32_t lifespan, uint32_t mfgDate,
                                   uint32_t ins, uint32_t tests, uint32_t eol) {
    EepromData d = {};
    d.adapterHardware          = (AdapterHardware)hwId;
    d.rfu        = 0xFF;  // reserved byte: erased-flash convention
    for (uint8_t i = 0; i < 4; i++) d.supportedPadmapIds[i] = padmapIds[i];
    d.designedLifespan        = lifespan;
    d.dateOfManufacture       = mfgDate;
    d.insertionCount          = (ins    == EepromData::FIELD_UNSET) ? 0 : ins;
    d.testCount               = (tests  == EepromData::FIELD_UNSET) ? 0 : tests;
    d.eolReached              = (eol == EepromData::FIELD_UNSET) ? 0u : (eol ? EepromData::EOL_REACHED : 0u);

    if (!_eepromMgr.write(d)) { LOG_E("adapter: eeprom provision write failed"); return false; }
    LOG_I("adapter: eeprom provisioned (Mezzanine70 v1)");
    return true;
}

bool StateMachine::tryInitAdapter() {
    auto result = _eepromMgr.read(_eepromData);
    _lastEepromResult = result;

    if (result == EepromManager::ReadResult::Blank) {
        LOG_E("adapter: eeprom blank — adapter must be provisioned before use");
        _hostProtocol.sendFault("ADAPTER_NOT_PROVISIONED");
        return false;
    }

    if (result != EepromManager::ReadResult::Ok) {
        LOG_E("adapter: eeprom read failed");
        return false;
    }

    LOG_I("adapter: hw=%u padmaps=[%u,%u,%u,%u] dom=%lu lifespan=%lu ins=%lu tests=%lu eol=%s",
          (uint8_t)_eepromData.adapterHardware,
          _eepromData.supportedPadmapIds[0], _eepromData.supportedPadmapIds[1],
          _eepromData.supportedPadmapIds[2], _eepromData.supportedPadmapIds[3],
          _eepromData.dateOfManufacture,
          _eepromData.designedLifespan, _eepromData.insertionCount, _eepromData.testCount,
          _eepromData.eolReached == EepromData::EOL_REACHED ? "YES" : "no");

    _adapter = AdapterRegistry::create(_eepromData);
    if (!_adapter) { LOG_E("adapter: unknown hw %u", (uint8_t)_eepromData.adapterHardware); return false; }

    // aid = the adapter's unique 64-bit serial burned into the AT21CS01. On read
    // failure pass nullptr — setAdapterUid falls back to sixteen '0's so the host
    // always sees a valid-length aid rather than garbage.
    char uidBuf[17];
    _hostProtocol.setAdapterUid(_eepromMgr.readSerialUid(uidBuf, sizeof(uidBuf)) ? uidBuf : nullptr);

    adapterSelfTest(_adapter, _mux, _adc);
    _dutDetector.setAdapter(_adapter);
    selectPadMap();
    // Discharge any residual charge on the connector pins before the first test to avoid false readings
    if (_padMap)
        _adapter->connectorIsolationSweep(_mux, _adc, *_padMap);
    LOG_I("adapter: init ok, padmap=%s", _padMap ? "set" : "null");
    return true;
}

void StateMachine::selectPadMap() {
    const uint8_t (&ids)[AdapterBase::PADMAP_ID_COUNT] = _adapter->getSupportedPadmapIds();
    for (uint8_t i = 0; i < AdapterBase::PADMAP_ID_COUNT && ids[i] != EepromData::PADMAP_UNSET; i++) {
        _padMap = PadMapRegistry::find(ids[i]);
        if (_padMap) break;
    }
    // Fall back to the universal pad map if no adapter-specific one is registered yet
    if (!_padMap)
        _padMap = PadMapRegistry::all();
    _dutDetector.setPadMap(_padMap);
}

void StateMachine::flushEeprom() {
    // Only mutable fields are ever modified: insertionCount, testCount, eolReached.
    // Read-only fields (hwId, padmapId, lifespan, dateOfManufacture) are
    // loaded once in tryInitAdapter() and never changed, so a full write is safe.
    if (_eepromData.insertionCount >= _eepromData.designedLifespan &&
        _eepromData.eolReached != EepromData::EOL_REACHED) {
        _eepromData.eolReached = EepromData::EOL_REACHED;
        LOG_W("adapter: EOL reached (%lu insertions)", _eepromData.insertionCount);
    }
    // Light the EOL LED on the adapter itself so the operator gets a physical indicator
    if (_adapter && _eepromData.eolReached == EepromData::EOL_REACHED) {
        _adapter->setEolLed(true);
    }
    if (!_eepromMgr.write(_eepromData)) {
        _hostProtocol.sendError(ErrorCode::PROVISION_FAILED, "EEPROM_WRITE_FAILED");
    }
}

bool StateMachine::checkAdapterAlive() {
    if (_eepromMgr.isPresent()) return true;
    LOG_W("adapter removed");
    if (_adapter) _adapter->setEolLed(false);  // adapter is going away; clear its LED state first
    _hostProtocol.sendAdapterRemoved();
    _adapter = nullptr;
    _padMap  = nullptr;
    _dutDetector.setAdapter(nullptr);
    transition(State::NO_ADAPTER);
    return false;
}

void StateMachine::tryStartTest() {
    // Starts a run when the machine is in a startable state: READY always starts;
    // PASS/FAIL re-check DUT presence first — the DUT may have been removed while
    // the result was displayed. No-op in every other state.
    if (_state == State::READY) {
        startTest();
        return;
    }
    if (_state == State::PASS || _state == State::FAIL) {
        if (_dutDetector.checkNow())
            startTest();
        else
            transition(State::ADAPTER_DETECTED);
    }
}

void StateMachine::startTest() {
    if (!checkAdapterAlive()) return;
    LOG_I("test start: slots=%u pads=%u", _adapter->getDutCount(), _padMap->caseCount);

    _hostProtocol.sendTestStart(
        (uint8_t)_adapter->getAdapterHardware(),
        _adapter->getSupportedPadmapIds(),
        _padMap,
        _eepromData.insertionCount,
        _eepromData.testCount);

    transition(State::TESTING);
    _ledManager.update(_state);

    _lastResultPadMap = _padMap;
    _testRunner.run(*_adapter, *_padMap, _lastResult);
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
    // Iterate the pad map that was active when the test ran, not the live one —
    // a SET_PADMAP between the run and GET_RESULTS would otherwise mislabel pad numbers.
    if (!_adapter || !_lastResultPadMap) return;
    for (uint8_t slot = 0; slot < _lastResult.slotCount; slot++) {
        const SlotResult& sr = _lastResult.slots[slot];
        _hostProtocol.sendSlotStatus(slot, sr.present, sr.tested);
        if (!sr.tested) continue;
        for (uint8_t i = 0; i < _lastResultPadMap->caseCount; i++) {
            const TestCase& tc = _lastResultPadMap->cases[i];
            if (tc.strategy == TestStrategy::DISCHARGE) continue;
            uint8_t channel = _adapter->channelForPin(tc.adapterPin);
            _hostProtocol.sendPadResult(slot, tc.adapterPin, tc.diePad, tc.strategy, sr.byChannel[channel]);
        }
    }
    _hostProtocol.sendSummary(_lastResult);
}
