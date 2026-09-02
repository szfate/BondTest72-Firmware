#include "test_runner.h"
#include "hal/kelvin.h"
#include "debug/log.h"
#include <Arduino.h>

TestRunner::TestRunner(MuxController& mux, AdcDriver& adc, DutDetector& dutDetector)
    : _mux(mux)
    , _adc(adc)
    , _dutDetector(dutDetector)
{
}

PadResult TestRunner::sweepPad(AdapterBase& adapter, const TestCase& tc) {
    PadResult pr = {};

    if (tc.strategy == TestStrategy::CAP_SENSE) {
        // Only the strongest (3.3k) pullup is fast enough to fully settle a
        // real cap in practical time — τ = R·C, so at 1µF the 330k/33k
        // levels would need hundreds of ms, impractical per-pad. In the
        // reverse-only sweep (MEASURE_DIRECTIONS, result.h) the DUT bypass
        // cap sits on the grounded sink side and never charges; the node
        // being charged is the die-side net, whose capacitance is
        // DUT-dependent (~0.7µF / τ≈2.3ms on the 1x1 die). tc.settleUs is
        // the total elapsed time for the last sample and must cover ~3τ of
        // that net — OPEN has to charge past the resistance-ceiling voltage
        // (~95% of VCC) to classify OPEN, while GOOD only errs safe (set per
        // pad map, NOT the short STANDARD default). Instead of one
        // point-in-time reading, measureKelvinCurve takes
        // CAP_SENSE_SAMPLE_COUNT samples spread across that window without
        // releasing the connection in between, so the readings[] array holds
        // a charging curve (die net rising on OPEN pads, flat at the bond
        // divider on GOOD pads) rather than discrete current levels — see
        // result.h and measureKelvinCurve's doc comment.
        const float maxOhms = tc.thresholds->maxBondResistanceOhms;
        uint8_t adapterCh = adapter.channelForPin(tc.adapterPin);
        uint8_t gndCh     = adapter.channelForPin(tc.gndPin);
        const PullupLevel& lvl = PULLUP_LEVELS[PULLUP_LEVEL_COUNT - 1];  // 3.3k — weakest R, strongest current

        if (measuresForward(MEASURE_DIRECTIONS)) {
            // forward: adapterPin driven + Kelvin-sensed, gndPin sinks
            measureKelvinCurve(_mux, _adc, adapterCh, gndCh, lvl.bus, lvl.ohms, tc.settleUs, maxOhms, &pr.readings[0]);
        }
        if (measuresReverse(MEASURE_DIRECTIONS)) {
            // reverse: gndPin driven + Kelvin-sensed, adapterPin sinks
            measureKelvinCurve(_mux, _adc, gndCh, adapterCh, lvl.bus, lvl.ohms, tc.settleUs, maxOhms,
                                &pr.readings[CAP_SENSE_SAMPLE_COUNT]);
        }

        // Classify on the last (most-settled) sample of each direction — the earlier samples are curve-shape only.
        bool fwdConducted = measuresForward(MEASURE_DIRECTIONS) && pr.readings[CAP_SENSE_SAMPLE_COUNT - 1].conducted;
        bool revConducted = measuresReverse(MEASURE_DIRECTIONS) && pr.readings[2 * CAP_SENSE_SAMPLE_COUNT - 1].conducted;
        pr.bond = (fwdConducted || revConducted) ? BondResult::GOOD : BondResult::OPEN;
        return pr;
    }

    const float maxOhms = tc.thresholds->maxBondResistanceOhms;
    uint8_t adapterCh = adapter.channelForPin(tc.adapterPin);
    uint8_t gndCh     = adapter.channelForPin(tc.gndPin);

    bool anyConducted = false;
    for (uint8_t i = 0; i < PULLUP_LEVEL_COUNT; i++) {
        if (measuresForward(MEASURE_DIRECTIONS)) {
            // forward: adapterPin driven + Kelvin-sensed, gndPin sinks
            PadReading fwd = measureKelvin(_mux, _adc, adapterCh, gndCh,
                                            PULLUP_LEVELS[i].bus, PULLUP_LEVELS[i].ohms, tc.settleUs, maxOhms);
            pr.readings[i] = fwd;
            anyConducted |= fwd.conducted;
        }
        if (measuresReverse(MEASURE_DIRECTIONS)) {
            // reverse: gndPin driven + Kelvin-sensed, adapterPin sinks
            PadReading rev = measureKelvin(_mux, _adc, gndCh, adapterCh,
                                            PULLUP_LEVELS[i].bus, PULLUP_LEVELS[i].ohms, tc.settleUs, maxOhms);
            pr.readings[PULLUP_LEVEL_COUNT + i] = rev;
            anyConducted |= rev.conducted;
        }
    }

    pr.bond = anyConducted ? BondResult::GOOD : BondResult::OPEN;
    return pr;
}

TestResult TestRunner::run(AdapterBase& adapter, const PadMap& padMap) {
    TestResult result = {};
    result.slotCount = adapter.getDutCount();
    result.outcome   = TestOutcome::PASS;

    if (result.slotCount == 0 || result.slotCount > MAX_DUT_SLOTS) {
        LOG_E("test: invalid slot count %u", result.slotCount);
        result.outcome = TestOutcome::FAIL;
        result.slotCount = 0;
        return result;
    }

    for (uint8_t slot = 0; slot < result.slotCount; slot++) {
        adapter.selectDut(slot);

        SlotResult& sr = result.slots[slot];
        sr.testedCount = 0;
        sr.goodCount   = 0;
        sr.present      = false;
        sr.tested       = false;

        for (uint8_t i = 0; i < padMap.caseCount; i++) {
            const TestCase& tc = padMap.cases[i];

            _mux.clearAll();
            if (tc.strategy == TestStrategy::DISCHARGE) {
                // Short both cap terminals to tester GND — do not engage a current source.
                _mux.setChannel(adapter.channelForPin(tc.gndPin), Bus::B);
                _mux.setChannel(adapter.channelForPin(tc.adapterPin), Bus::B);
                delayMicroseconds(tc.settleUs);
                _mux.clearAll();
                continue;
            }
            PadResult pr = sweepPad(adapter, tc);

            // Full per-reading detail goes out via sendPadResult; keep this to a summary.
            LOG_I("slot%u apin%u die%u: result=%s", slot, tc.adapterPin, tc.diePad,
                  pr.bond == BondResult::GOOD ? "GOOD" : "OPEN");

            sr.byChannel[adapter.channelForPin(tc.adapterPin)] = pr;
            sr.testedCount++;
            if (pr.bond == BondResult::GOOD) sr.goodCount++;
        }

        if (!_dutDetector.checkNow()) {
            result.outcome = TestOutcome::FAIL_DUT_REMOVED;
            sr.present = true;   // was present when slot started
            sr.tested = false;   // removed mid-test
            break;
        }
        sr.present = true;
        sr.tested = true;
    }

    _mux.clearAll();

    if (result.outcome == TestOutcome::PASS) {
        for (uint8_t slot = 0; slot < result.slotCount && result.outcome == TestOutcome::PASS; slot++) {
            const SlotResult& sr = result.slots[slot];
            if (!sr.tested) continue;

            // Every pad must pass individually.
            for (uint8_t i = 0; i < padMap.caseCount && result.outcome == TestOutcome::PASS; i++) {
                const TestCase& tc = padMap.cases[i];
                if (tc.strategy == TestStrategy::DISCHARGE) continue;
                const PadResult& pr = sr.byChannel[adapter.channelForPin(tc.adapterPin)];
                if (pr.bond != BondResult::GOOD)
                    result.outcome = TestOutcome::FAIL;
            }
        }
    }

    return result;
}
