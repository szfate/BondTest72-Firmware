#include "test_runner.h"
#include "../debug/log.h"
#include <Arduino.h>

TestRunner::TestRunner(MuxController& mux, AdcDriver& adc, DutDetector& dutDetector)
    : _mux(mux)
    , _adc(adc)
    , _dutDetector(dutDetector)
{
}

PadResult TestRunner::classify(const AdcReadings& r, const TestCase& tc, const PadMap& padMap) {
    PadResult pr;

    if      (r.sense < padMap.senseGoodMin) pr.bond = BondResult::SHORT_GND;
    else if (r.sense > padMap.senseGoodMax) pr.bond = BondResult::OPEN;
    else                                    pr.bond = BondResult::GOOD;

    pr.leftShort  = (tc.left  != NO_NEIGHBOUR) &&
                    (r.leftNeighbour  < padMap.neighbourGoodMin ||
                     r.leftNeighbour  > padMap.neighbourGoodMax);

    pr.rightShort = (tc.right != NO_NEIGHBOUR) &&
                    (r.rightNeighbour < padMap.neighbourGoodMin ||
                     r.rightNeighbour > padMap.neighbourGoodMax);

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
            _mux.setChannel(adapter.channelForPin(tc.gnd), Bus::D);
            _mux.setChannel(adapter.channelForPin(tc.pad), Bus::B);
            if (tc.left  != NO_NEIGHBOUR) _mux.setChannel(adapter.channelForPin(tc.left),  Bus::A);
            if (tc.right != NO_NEIGHBOUR) _mux.setChannel(adapter.channelForPin(tc.right), Bus::C);

            AdcReadings r  = _adc.readAll();
            PadResult   pr = classify(r, tc, padMap);

            LOG_D("slot%u pad%u: sense=%.3f left=%.3f right=%.3f",
                  slot, tc.pad, r.sense, r.leftNeighbour, r.rightNeighbour);

            bool ok = pr.bond == BondResult::GOOD && !pr.leftShort && !pr.rightShort;
            if (!ok) {
                LOG_I("slot%u pad%u FAIL: bond=%s%s%s sense=%.3f",
                      slot, tc.pad,
                      pr.bond == BondResult::SHORT_GND ? "SHORT_GND" :
                      pr.bond == BondResult::OPEN      ? "OPEN"      : "GOOD",
                      pr.leftShort  ? " +LEFT_SHORT"  : "",
                      pr.rightShort ? " +RIGHT_SHORT" : "",
                      r.sense);
            }

            sr.pads[adapter.channelForPin(tc.pad)] = pr;
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
        for (uint8_t slot = 0; slot < result.slotCount; slot++) {
            if (result.slots[slot].goodCount != result.slots[slot].testedCount) {
                result.outcome = TestOutcome::FAIL;
                break;
            }
        }
    }

    return result;
}
