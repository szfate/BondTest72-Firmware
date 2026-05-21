#include "test_runner.h"
#include "../debug/log.h"
#include <Arduino.h>

TestRunner::TestRunner(MuxController& mux, AdcDriver& adc, DutDetector& dutDetector)
    : _mux(mux)
    , _adc(adc)
    , _dutDetector(dutDetector)
{
}

PadResult TestRunner::classify(const AdcReadings& r, const TestCase& tc) {
    const TestThresholds& th = *tc.thresholds;
    PadResult pr;

    if (tc.strategy == TestStrategy::SKIP_SENSE) {
        pr.bond = BondResult::GOOD;
    } else {
        if      (r.sense < th.senseGoodMin) pr.bond = BondResult::SHORT_GND;
        else if (r.sense > th.senseGoodMax) pr.bond = BondResult::OPEN;
        else                                pr.bond = BondResult::GOOD;
    }

    pr.prevShort = (tc.prevPin != NO_NEIGHBOUR) &&
                   (r.prevNeighbour < th.neighbourGoodMin ||
                    r.prevNeighbour > th.neighbourGoodMax);

    pr.nextShort = (tc.nextPin != NO_NEIGHBOUR) &&
                   (r.nextNeighbour < th.neighbourGoodMin ||
                    r.nextNeighbour > th.neighbourGoodMax);

    return pr;
}

TestResult TestRunner::run(AdapterBase& adapter, const PadMap& padMap) {
    TestResult result;
    result.slotCount = adapter.getDutCount();
    result.outcome   = TestOutcome::PASS;

    for (uint8_t slot = 0; slot < adapter.getDutCount(); slot++) {
        adapter.selectDut(slot);

        SlotResult& sr = result.slots[slot];
        sr.testedCount = 0;
        sr.goodCount   = 0;

        for (uint8_t i = 0; i < padMap.caseCount; i++) {
            const TestCase& tc = padMap.cases[i];

            _mux.clearAll();
            if (tc.strategy == TestStrategy::DISCHARGE) {
                // Short both cap terminals to tester GND — do not engage the Bus::D current source.
                _mux.setChannel(adapter.channelForPin(tc.gndPin), Bus::B);
                _mux.setChannel(adapter.channelForPin(tc.mezPin), Bus::B);
                delay(tc.settleMs);
                continue;
            }

            // Phase 1: neighbour short detection
            // DUT pad pre-grounded (no injection) so neighbours read clean mid-rail;
            // any short to DUT pulls them toward 0 V without injection crosstalk.
            _mux.setChannel(adapter.channelForPin(tc.mezPin), Bus::B);
            if (tc.prevPin != NO_NEIGHBOUR) _mux.setChannel(adapter.channelForPin(tc.prevPin), Bus::A);
            if (tc.nextPin != NO_NEIGHBOUR) _mux.setChannel(adapter.channelForPin(tc.nextPin), Bus::C);
            delay(tc.settleMs);
            AdcReadings r = {};
            if (tc.prevPin != NO_NEIGHBOUR) r.prevNeighbour = _adc.readVoltage(1);
            if (tc.nextPin != NO_NEIGHBOUR) r.nextNeighbour = _adc.readVoltage(2);

            // Phase 2: bond sense
            // Re-establish return path then connect injection — neighbours disconnected
            // so the sense bus has no additional load.
            _mux.clearAll();
            _mux.setChannel(adapter.channelForPin(tc.mezPin), Bus::B);
            _mux.setChannel(adapter.channelForPin(tc.gndPin), Bus::D);
            delay(tc.settleMs);
            r.sense = _adc.readVoltage(0);
            _mux.clearAll();

            PadResult   pr = classify(r, tc);

            LOG_I("slot%u amez%u die%u: sense=%.3f prev=%.3f next=%.3f",
                  slot, tc.mezPin, tc.diePad, r.sense, r.prevNeighbour, r.nextNeighbour);

            bool ok = pr.bond == BondResult::GOOD && !pr.prevShort && !pr.nextShort;
            if (!ok) {
                LOG_I("slot%u amez%u die%u FAIL: bond=%s%s%s sense=%.3f prev=%.3f next=%.3f",
                      slot, tc.mezPin, tc.diePad,
                      pr.bond == BondResult::SHORT_GND ? "SHORT_GND" :
                      pr.bond == BondResult::OPEN      ? "OPEN"      : "GOOD",
                      pr.prevShort ? " +PREV_SHORT" : "",
                      pr.nextShort ? " +NEXT_SHORT" : "",
                      r.sense, r.prevNeighbour, r.nextNeighbour);
            }

            sr.byChannel[adapter.channelForPin(tc.mezPin)] = pr;
            sr.testedCount++;
            if (ok) sr.goodCount++;
        }

        if (!_dutDetector.checkNow()) {
            result.outcome = TestOutcome::FAIL_DUT_REMOVED;
            break;
        }
    }

    _mux.clearAll();

    if (result.outcome == TestOutcome::PASS) {
        for (uint8_t slot = 0; slot < result.slotCount && result.outcome == TestOutcome::PASS; slot++) {
            const SlotResult& sr = result.slots[slot];

            // Ungrouped pads: every one must pass.
            for (uint8_t i = 0; i < padMap.caseCount && result.outcome == TestOutcome::PASS; i++) {
                const TestCase& tc = padMap.cases[i];
                if (tc.strategy == TestStrategy::DISCHARGE || tc.groupId != 0) continue;
                const PadResult& pr = sr.byChannel[adapter.channelForPin(tc.mezPin)];
                if (pr.bond != BondResult::GOOD || pr.prevShort || pr.nextShort)
                    result.outcome = TestOutcome::FAIL;
            }

            // Grouped pads: each group needs at least minPass members passing.
            for (uint8_t g = 0; g < padMap.padGroupCount && result.outcome == TestOutcome::PASS; g++) {
                const PadGroup& grp = padMap.padGroups[g];
                uint8_t passCount = 0, totalCount = 0;
                for (uint8_t i = 0; i < padMap.caseCount; i++) {
                    const TestCase& tc = padMap.cases[i];
                    if (tc.groupId != grp.id) continue;
                    totalCount++;
                    const PadResult& pr = sr.byChannel[adapter.channelForPin(tc.mezPin)];
                    if (pr.bond == BondResult::GOOD && !pr.prevShort && !pr.nextShort) passCount++;
                }
                LOG_I("slot%u group %s: %u/%u pass (need %u) -> %s",
                      slot, grp.name, passCount, totalCount, grp.minPass,
                      passCount >= grp.minPass ? "PASS" : "FAIL");
                if (passCount < grp.minPass)
                    result.outcome = TestOutcome::FAIL;
            }
        }
    }

    return result;
}
