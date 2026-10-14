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
#include <string.h>

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
        // A primed detector never reports that DUT as INSERTED (it already
        // counts as present), so move to READY here or the tester would sit
        // in ADAPTER_DETECTED, ignoring START, until the DUT is re-inserted.
        if (_state == State::ADAPTER_DETECTED)
            syncStateToDut();
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
            _eepromData.supportedPadmapIds,
            _padMap);
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
            // flushEeprom sets EOL_REACHED flag if the new count hit the lifespan limit;
            // on failure it escalates to FAULT — skip the READY/EOL transition then.
            if (!flushEeprom()) return;
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
// "State Machine"), enforced in handleCommand. HELLO, GET_ADAPTER and
// LIST_PADMAPS are pure queries — valid in every state (provision.py
// GET_ADAPTERs from FAULT/NO_ADAPTER; LIST_PADMAPS answers NO_ADAPTER itself
// when there is nothing to list). PROVISION is allowed everywhere except
// TESTING: a fresh blank adapter latches FAULT, and provision.py works from
// there. Pad map changes are allowed whenever a healthy adapter is in and no
// test is running — including WRONG_ORIENTATION, since picking the wrong map
// is itself a common cause of a "flipped" reading. Never during TESTING: a
// mid-test swap would invalidate the result's pad labels.
constexpr uint16_t cmdBit(HostCommand c) { return 1u << static_cast<uint8_t>(c); }

static constexpr uint16_t CMD_QUERIES  = cmdBit(HostCommand::HELLO) | cmdBit(HostCommand::GET_ADAPTER) |
                                         cmdBit(HostCommand::LIST_PADMAPS);
static constexpr uint16_t CMD_PADMAP   = cmdBit(HostCommand::SET_PADMAP) | cmdBit(HostCommand::SET_DEFAULT_PADMAP);

static constexpr uint16_t CMD_MASK[static_cast<uint8_t>(State::COUNT)] = {
    /* NO_ADAPTER        */ CMD_QUERIES | cmdBit(HostCommand::PROVISION),
    /* EOL_ADAPTER       */ CMD_QUERIES | cmdBit(HostCommand::PROVISION),
    /* ADAPTER_DETECTED  */ CMD_QUERIES | CMD_PADMAP | cmdBit(HostCommand::GET_RESULTS) | cmdBit(HostCommand::PROVISION),
    /* READY             */ CMD_QUERIES | CMD_PADMAP | cmdBit(HostCommand::RUN) | cmdBit(HostCommand::GET_RESULTS) |
                            cmdBit(HostCommand::PROVISION),
    /* WRONG_ORIENTATION */ CMD_QUERIES | CMD_PADMAP,
    /* TESTING           */ CMD_QUERIES,
    /* PASS              */ CMD_QUERIES | CMD_PADMAP | cmdBit(HostCommand::RUN) | cmdBit(HostCommand::GET_RESULTS),
    /* FAIL              */ CMD_QUERIES | CMD_PADMAP | cmdBit(HostCommand::RUN) | cmdBit(HostCommand::GET_RESULTS),
    /* FAULT             */ CMD_QUERIES | cmdBit(HostCommand::PROVISION),
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
        case HostCommand::SET_PADMAP:
            handleSetPadMap();
            break;
        case HostCommand::SET_DEFAULT_PADMAP:
            handleSetDefaultPadMap();
            break;
        case HostCommand::LIST_PADMAPS:
            handleListPadMaps();
            break;
        case HostCommand::PROVISION: {
            const ProvisionRequest& req = _hostProtocol.provisionRequest();
            if (!_eepromMgr.isPresent()) {
                _hostProtocol.sendError(ErrorCode::NO_ADAPTER, "NO_ADAPTER");
            } else if (!req.hasHw()) {
                _hostProtocol.sendError(ErrorCode::MISSING_FIELD, "MISSING_HW");
            } else if (!req.hasPadmap()) {
                _hostProtocol.sendError(ErrorCode::MISSING_FIELD, "MISSING_PADMAP");
            } else if (!req.hasLifespan()) {
                _hostProtocol.sendError(ErrorCode::MISSING_FIELD, "MISSING_LIFESPAN");
            } else if (!req.hasDate()) {
                _hostProtocol.sendError(ErrorCode::MISSING_FIELD, "MISSING_DATE");
            } else if (!provisionEeprom(req)) {
                _hostProtocol.sendError(ErrorCode::PROVISION_FAILED, "PROVISION_FAILED");
            } else {
                _adapter = nullptr; _padMap = nullptr;
                if (tryInitAdapter()) {
                    transition(State::ADAPTER_DETECTED);
                    // Re-provisioning with a DUT seated: same situation as boot.
                    _dutDetector.prime();
                    syncStateToDut();
                } else {
                    transition(State::FAULT);
                }
                _hostProtocol.sendOk("PROVISION");
            }
            break;
        }
        case HostCommand::PROVISION_INVALID:
            _hostProtocol.sendError(ErrorCode::MISSING_FIELD, "MISSING_PADMAP");
            break;
        case HostCommand::GET_ADAPTER:
            if (!_adapter) {
                sendNoAdapterError();
            } else {
                _hostProtocol.sendAdapterInfo(
                    (uint8_t)_eepromData.adapterHardware,
                    _eepromData.supportedPadmapIds,
                    _padMap,
                    _eepromData.designedLifespan,
                    _eepromData.dateOfManufacture,
                    _eepromData.insertionCount,
                    _eepromData.testCount,
                    _eepromData.eolReached == EepromData::EOL_REACHED,
                    _dutDetector.dutPresent(),
                    stateName(_state));
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

// Why there is no usable adapter: a present-but-blank EEPROM needs PROVISION,
// which is a different fix for the user than plugging an adapter in.
void StateMachine::sendNoAdapterError() {
    if (_eepromMgr.isPresent() && _lastEepromResult == EepromManager::ReadResult::Blank)
        _hostProtocol.sendError(ErrorCode::ADAPTER_NOT_PROVISIONED, "ADAPTER_NOT_PROVISIONED");
    else
        _hostProtocol.sendError(ErrorCode::NO_ADAPTER, "NO_ADAPTER");
}

bool StateMachine::provisionEeprom(const ProvisionRequest& req) {
    EepromData d = eepromFromProvision(req);  // sentinel→default mapping lives in eeprom_layout
    if (!_eepromMgr.write(d)) { LOG_E("adapter: eeprom provision write failed"); return false; }
    LOG_I("adapter: eeprom provisioned (hw=%u)", req.hwId);
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
    if (!_padMap) {
        // Only possible if no built-in pad map targets this adapter's pinout —
        // a firmware build problem, not a contact problem.
        LOG_E("adapter: no built-in pad map fits this adapter");
        return false;
    }
    // Discharge any residual charge on the connector pins before the first test to avoid false readings
    _adapter->connectorIsolationSweep(_mux, _adc, *_padMap);
    LOG_I("adapter: init ok, padmap=%u (%s)", _padMap->id, _padMap->name);
    return true;
}

// Picks the pad map for a freshly detected adapter: the first entry of the
// EEPROM padmap list that this firmware knows and that fits the adapter's
// pinout. An entry can be skipped when the adapter was provisioned for a map
// this build doesn't have; then the first compatible built-in map is used so
// the tester stays usable. Any host selection from a previous adapter session
// is deliberately dropped here — SET_DEFAULT_PADMAP is how a choice persists.
void StateMachine::selectPadMap() {
    _padMap = nullptr;
    const uint8_t (&ids)[AdapterBase::PADMAP_ID_COUNT] = _eepromData.supportedPadmapIds;
    for (uint8_t i = 0; i < AdapterBase::PADMAP_ID_COUNT && ids[i] != EepromData::PADMAP_UNSET; i++) {
        const PadMap* map = PadMapRegistry::find(ids[i]);
        if (padMapFitsAdapter(map)) { _padMap = map; break; }
        LOG_W("adapter: padmap %u from eeprom is unknown or incompatible — skipped", ids[i]);
    }
    if (!_padMap) {
        const PadMap* maps = PadMapRegistry::all();
        for (uint8_t i = 0; i < PadMapRegistry::count(); i++) {
            if (padMapFitsAdapter(&maps[i])) { _padMap = &maps[i]; break; }
        }
        if (_padMap) LOG_W("adapter: falling back to padmap %u", _padMap->id);
    }
    _dutDetector.setPadMap(_padMap);
}

bool StateMachine::padMapFitsAdapter(const PadMap* map) const {
    return map && _adapter && map->pinout == _adapter->getPinout();
}

// Resolves a host-supplied pad map ID, reporting the reason to the host when it
// can't be used. Caller has already been state-gated, so an adapter is present.
const PadMap* StateMachine::findCompatiblePadMap(uint8_t id) {
    const PadMap* map = PadMapRegistry::find(id);
    if (!map) {
        _hostProtocol.sendError(ErrorCode::UNKNOWN_PADMAP, "UNKNOWN_PADMAP");
        return nullptr;
    }
    if (!padMapFitsAdapter(map)) {
        _hostProtocol.sendError(ErrorCode::PADMAP_INCOMPATIBLE, "PADMAP_INCOMPATIBLE");
        return nullptr;
    }
    return map;
}

// Makes `map` the active pad map and re-senses the DUT with it. Presence and
// orientation are pad-map specific (presencePadA/B), so the DUT state under the
// old map says nothing about the new one; prime() takes a fresh reading.
// Landing in READY here is not a new insertion — the insertion counter only
// counts physical insertions (handleDutEvent). A PASS/FAIL result is cleared
// on purpose: it was measured under the previous map. _lastResult itself is
// kept (with _lastResultPadMap) so GET_RESULTS still returns it correctly.
void StateMachine::activatePadMap(const PadMap* map) {
    _padMap = map;
    _dutDetector.setPadMap(map);
    _dutDetector.prime();
    LOG_I("padmap: active=%u (%s)", map->id, map->name);
    syncStateToDut();
}

// Moves to the state matching the detector's current (primed) DUT reading:
// READY, WRONG_ORIENTATION or ADAPTER_DETECTED. Used where the DUT reading was
// taken directly rather than arriving as a DutEvent — boot, re-provisioning
// and pad map switches — so it never counts an insertion. Only transitions on
// an actual change: transition() re-announces EVENT ADAPTER_DETECTED, which a
// host would take for a re-inserted adapter.
void StateMachine::syncStateToDut() {
    State next = _dutDetector.dutWrongOrientation() ? State::WRONG_ORIENTATION
               : _dutDetector.dutPresent()          ? State::READY
                                                    : State::ADAPTER_DETECTED;
    if (next == _state) return;
    if (next == State::WRONG_ORIENTATION)
        _hostProtocol.sendWrongOrientation();
    transition(next);
}

void StateMachine::handleSetPadMap() {
    const PadMap* map = findCompatiblePadMap(_hostProtocol.padmapIdArg());
    if (!map) return;
    activatePadMap(map);
    _hostProtocol.sendOkPadmap("SET_PADMAP", map->id);
}

// Moves `id` to the front of the adapter EEPROM's padmap list — the first entry
// is what selectPadMap() activates on every future detection — and activates it
// now. The rest of the list keeps its order (minus the moved id), dropping the
// last entry if the list was full. Skips the EEPROM write when `id` is already
// first, to spare write cycles.
void StateMachine::handleSetDefaultPadMap() {
    const PadMap* map = findCompatiblePadMap(_hostProtocol.padmapIdArg());
    if (!map) return;

    uint8_t (&ids)[AdapterBase::PADMAP_ID_COUNT] = _eepromData.supportedPadmapIds;
    if (ids[0] != map->id) {
        uint8_t reordered[AdapterBase::PADMAP_ID_COUNT];
        uint8_t n = 0;
        reordered[n++] = map->id;
        for (uint8_t i = 0; i < AdapterBase::PADMAP_ID_COUNT && n < AdapterBase::PADMAP_ID_COUNT; i++) {
            if (ids[i] == EepromData::PADMAP_UNSET) break;
            if (ids[i] != map->id) reordered[n++] = ids[i];
        }
        while (n < AdapterBase::PADMAP_ID_COUNT) reordered[n++] = EepromData::PADMAP_UNSET;
        memcpy(ids, reordered, sizeof(reordered));

        // flushEeprom() reports EVENT FAULT and enters FAULT itself on failure.
        if (!flushEeprom()) return;
        LOG_I("padmap: default=%u written to adapter", map->id);
    }
    activatePadMap(map);
    _hostProtocol.sendOkPadmap("SET_DEFAULT_PADMAP", map->id);
}

// Lists every built-in pad map that fits the connected adapter, then OK.
// Maps for other pinouts are left out: they could never be selected here.
void StateMachine::handleListPadMaps() {
    if (!_adapter) {
        sendNoAdapterError();
        return;
    }
    const uint8_t defaultId = _eepromData.supportedPadmapIds[0];
    const PadMap* maps = PadMapRegistry::all();
    for (uint8_t i = 0; i < PadMapRegistry::count(); i++) {
        const PadMap& m = maps[i];
        if (!padMapFitsAdapter(&m)) continue;
        _hostProtocol.sendPadmapEntry(m, m.id == defaultId, &m == _padMap);
    }
    _hostProtocol.sendOk("LIST_PADMAPS");
}

bool StateMachine::flushEeprom() {
    // Only mutable fields are ever modified: insertionCount, testCount,
    // eolReached, and the padmap list order (SET_DEFAULT_PADMAP). The rest
    // (hwId, lifespan, dateOfManufacture) is loaded once in tryInitAdapter()
    // and never changed, so writing back the whole header is safe.
    if (_eepromData.insertionCount >= _eepromData.designedLifespan &&
        _eepromData.eolReached != EepromData::EOL_REACHED) {
        _eepromData.eolReached = EepromData::EOL_REACHED;
        LOG_W("adapter: EOL reached (%lu insertions)", _eepromData.insertionCount);
    }
    // Light the EOL LED on the adapter itself so the operator gets a physical indicator
    if (_adapter && _eepromData.eolReached == EepromData::EOL_REACHED) {
        _adapter->setEolLed(true);
    }
    if (_eepromMgr.write(_eepromData))
        return true;
    // EepromManager already retried once with read-back verify. A flush that
    // still fails means the counters on the wire disagree with what the host
    // was told (and possibly a torn image) — escalate for operator attention
    // instead of reporting per-flush errors forever (R2).
    _hostProtocol.sendFault("EEPROM_WRITE_FAILED");
    transition(State::FAULT);
    return false;
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
        _eepromData.supportedPadmapIds,
        _padMap,
        _eepromData.insertionCount,
        _eepromData.testCount);

    transition(State::TESTING);
    _ledManager.update(_state);

    _lastResultPadMap = _padMap;
    _testRunner.run(*_adapter, *_padMap, _lastResult);
    _eepromData.testCount++;
    // On flush failure the machine is in FAULT — skip results/host reporting.
    if (!flushEeprom()) return;

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
            _hostProtocol.sendPadResult(slot, tc.adapterPin, tc.diePad, tc.strategy, tc.padType,
                                        _lastResultPadMap->directions, sr.byChannel[channel]);
        }
    }
    _hostProtocol.sendSummary(_lastResult);
}
