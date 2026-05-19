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

    pr.prevShort = (tc.prevPin != NO_NEIGHBOUR) &&
                   (r.prevNeighbour < padMap.neighbourGoodMin ||
                    r.prevNeighbour > padMap.neighbourGoodMax);

    pr.nextShort = (tc.nextPin != NO_NEIGHBOUR) &&
                   (r.nextNeighbour < padMap.neighbourGoodMin ||
                    r.nextNeighbour > padMap.neighbourGoodMax);

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
            _mux.setChannel(adapter.channelForPin(tc.gndPin),   Bus::D);
            _mux.setChannel(adapter.channelForPin(tc.mezPin),   Bus::B);
            if (tc.prevPin != NO_NEIGHBOUR) _mux.setChannel(adapter.channelForPin(tc.prevPin), Bus::A);
            if (tc.nextPin != NO_NEIGHBOUR) _mux.setChannel(adapter.channelForPin(tc.nextPin), Bus::C);
            delay(2);  // allow signals to settle

            AdcReadings r  = _adc.readAll();
            PadResult   pr = classify(r, tc, padMap);

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
        for (uint8_t slot = 0; slot < result.slotCount; slot++) {
            if (result.slots[slot].goodCount != result.slots[slot].testedCount) {
                result.outcome = TestOutcome::FAIL;
                break;
            }
        }
    }

    return result;
}
