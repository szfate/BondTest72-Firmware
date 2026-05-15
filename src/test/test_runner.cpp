#include "test_runner.h"

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

            sr.pads[adapter.channelForPin(tc.pad)] = pr;
            sr.testedCount++;
            if (pr.bond == BondResult::GOOD && !pr.leftShort && !pr.rightShort)
                sr.goodCount++;
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
