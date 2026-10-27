#include "kelvin.h"
#include <Arduino.h>

// Covers ~10τ of a 1µF net through two ~50Ω switches; raise it for bigger caps.
static constexpr uint32_t DRAIN_SETTLE_US = 1000;

// Chosen from bench IV data to drive ~10/100/1000 µA through a good bond.
const PullupLevel PULLUP_LEVELS[PULLUP_LEVEL_COUNT] = {
    { Bus::C, 280000.0f,   10.0f },
    { Bus::D,  27400.0f,  100.0f },
    { Bus::E,   2490.0f, 1000.0f },
};

// Front-loaded: for a ~1µF die-side net (τ≈2.5ms) in a 20ms window the
// samples land at ~0.06/0.13/0.5/4/8τ. Don't go much earlier than /128 —
// the ADC read itself takes tens of µs.
void curveSampleTimesUs(uint16_t totalSettleUs, uint16_t* out) {
    static_assert(CAP_SENSE_SAMPLE_COUNT == 5, "curveSampleTimesUs's schedule is hardcoded for 5 points");
    out[0] = totalSettleUs / 128;
    out[1] = totalSettleUs / 64;
    out[2] = totalSettleUs / 16;
    out[3] = totalSettleUs / 2;
    out[4] = totalSettleUs;
}

static PadReading classifyVoltage(float v, float pullupOhms, float maxResistanceOhms) {
    PadReading r;
    r.voltageV  = v;
    float denom = VCC - v;
    r.resistanceOhms = (denom > 0.001f) ? pullupOhms * v / denom : 1.0e9f;  // clamp near-VCC (open) case
    r.conducted = r.resistanceOhms < maxResistanceOhms;
    return r;
}

// Switching order is a latch-up guard. Grounding a charged cap plate while
// its return floats pushes the return to about −VCC; on the CH446X an X pin
// more than 0.3V below VEE fires a parasitic SCR (~0.6A, cleared only by a
// power cycle). VEE ≈ −VSYS on the board keeps that out of reach, so this is
// defence in depth. Rules:
//   - after clearAll(), ground the return (sinkCh) first;
//   - only drain forceCh while sinkCh is grounded;
//   - rebuild from clearAll() rather than clearChannel() — that one leaves the
//     drain crosspoint closed and every pad then reads GOOD.

// Leaves only sinkCh→Bus::B closed, with forceCh drained.
static void groundAndDischarge(MuxController& mux, uint8_t forceCh, uint8_t sinkCh) {
    mux.clearAll();
    mux.setChannel(sinkCh, Bus::B);
    mux.setChannel(forceCh, Bus::B);
    delayMicroseconds(DRAIN_SETTLE_US);

    mux.clearAll();
    mux.setChannel(sinkCh, Bus::B);
}

// Drains forceCh so no charge carries into the next measurement, then opens
// everything. Expects sinkCh→Bus::B still closed.
static void drainAndRelease(MuxController& mux, uint8_t forceCh) {
    mux.setChannel(forceCh, Bus::B);
    delayMicroseconds(DRAIN_SETTLE_US);
    mux.clearAll();
}

PadReading measureKelvin(MuxController& mux, AdcDriver& adc,
                          uint8_t forceCh, uint8_t sinkCh, Bus pullupBus, float pullupOhms,
                          uint16_t settleUs, float maxResistanceOhms) {
    groundAndDischarge(mux, forceCh, sinkCh);

    mux.setChannel(forceCh, pullupBus);
    mux.setChannel(forceCh, Bus::A);
    delayMicroseconds(settleUs);
    float v = adc.readVoltage(AdcChannel::KelvinSense);
    drainAndRelease(mux, forceCh);

    return classifyVoltage(v, pullupOhms, maxResistanceOhms);
}

void measureKelvinCurve(MuxController& mux, AdcDriver& adc,
                         uint8_t forceCh, uint8_t sinkCh, Bus pullupBus, float pullupOhms,
                         uint16_t totalSettleUs, float maxResistanceOhms,
                         PadReading* out) {
    groundAndDischarge(mux, forceCh, sinkCh);

    mux.setChannel(forceCh, pullupBus);
    mux.setChannel(forceCh, Bus::A);

    uint16_t times[CAP_SENSE_SAMPLE_COUNT];
    curveSampleTimesUs(totalSettleUs, times);

    uint16_t elapsed = 0;
    for (uint8_t i = 0; i < CAP_SENSE_SAMPLE_COUNT; i++) {
        delayMicroseconds(times[i] - elapsed);
        elapsed = times[i];
        out[i] = classifyVoltage(adc.readVoltage(AdcChannel::KelvinSense), pullupOhms, maxResistanceOhms);
    }
    drainAndRelease(mux, forceCh);
}

bool kelvinAnyLevelBelow(MuxController& mux, AdcDriver& adc,
                          uint8_t forceCh, uint8_t sinkCh, float thresholdV, uint16_t settleUs) {
    for (uint8_t i = 0; i < PULLUP_LEVEL_COUNT; i++) {
        PadReading r = measureKelvin(mux, adc, forceCh, sinkCh,
                                      PULLUP_LEVELS[i].bus, PULLUP_LEVELS[i].ohms, settleUs, 0.0f);
        if (r.voltageV < thresholdV) return true;
    }
    return false;
}
