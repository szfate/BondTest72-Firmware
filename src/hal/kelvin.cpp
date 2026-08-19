#include "kelvin.h"
#include <Arduino.h>

static constexpr float VCC = 3.3f;

const PullupLevel PULLUP_LEVELS[PULLUP_LEVEL_COUNT] = {
    { Bus::C, 320000.0f },
    { Bus::D,  32850.0f },
    { Bus::E,   3285.0f },
};

float pullupCurrentUA(float pullupOhms) {
    return VCC / pullupOhms * 1.0e6f;
}

void curveSampleTimesUs(uint16_t totalSettleUs, uint16_t* out) {
    static_assert(CAP_SENSE_SAMPLE_COUNT == 5, "curveSampleTimesUs's schedule is hardcoded for 5 points");
    out[0] = totalSettleUs / 16;
    out[1] = totalSettleUs / 8;
    out[2] = totalSettleUs / 2;
    out[3] = totalSettleUs * 3 / 4;
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

// Drains any charge sitting on forceCh, then leaves the mux with ONLY
// sinkCh→Bus::B closed, ready for the caller to attach the pullup and sense
// legs to forceCh.
//
// ORDER IS SAFETY-CRITICAL. Each setChannel is a separate ~10µs serial
// transaction (often to a different chip), so whichever pin is grounded
// first spends that window alone. If forceCh (which may hold a 1µF bypass
// cap charged to 3.3V by a preceding CAP_SENSE) is clamped to ground while
// its return still floats, the floating side is driven to about −3.3V —
// roughly 10x past the mux's −0.3V latch-up threshold. That fires the
// parasitic SCR, which then draws ~0.6A from the chip's own rail until
// power is physically removed (an MCU reset will NOT clear it) and heats
// the package to ~100°C in seconds.
//
// So the rule is: after any clearAll(), ground the reference FIRST. A full
// clearAll() is itself safe — when everything opens, both plates of a cap
// float together and nothing is clamped — which is why the second phase
// below rebuilds from a clean reset rather than trying to open just the
// drain leg with clearChannel(). (Don't "optimize" that back: clearChannel
// leaves the crosspoint closed, which shorts forceCh to ground through the
// switch Ron during the measurement and makes every pad read ~150Ω / GOOD.)
//
// The drain itself is a hard short via Bus::B — there is no current-limited
// path, since COM_C/D/E are pullups to VCC and Bus::B is the only ground.
// That's ~33mA through two ~50Ω switches, above the continuous rating but
// entirely within the rails, so it's switch stress rather than a latch-up
// risk. τ≈100µs into 1µF; 500µs is ~5τ, >99% drained.
static void groundAndDischarge(MuxController& mux, uint8_t forceCh, uint8_t sinkCh) {
    mux.clearAll();
    mux.setChannel(sinkCh, Bus::B);   // reference first — never leave it floating
    mux.setChannel(forceCh, Bus::B);  // then drain into a solid ground
    delayMicroseconds(500);

    mux.clearAll();
    mux.setChannel(sinkCh, Bus::B);   // reference first again, before the caller drives forceCh
}

PadReading measureKelvin(MuxController& mux, AdcDriver& adc,
                          uint8_t forceCh, uint8_t sinkCh, Bus pullupBus, float pullupOhms,
                          uint16_t settleUs, float maxResistanceOhms) {
    groundAndDischarge(mux, forceCh, sinkCh);  // leaves sinkCh→Bus::B closed

    mux.setChannel(forceCh, pullupBus);
    mux.setChannel(forceCh, Bus::A);
    delayMicroseconds(settleUs);
    float v = adc.readVoltage(1);  // COM_A
    mux.clearAll();

    return classifyVoltage(v, pullupOhms, maxResistanceOhms);
}

void measureKelvinCurve(MuxController& mux, AdcDriver& adc,
                         uint8_t forceCh, uint8_t sinkCh, Bus pullupBus, float pullupOhms,
                         uint16_t totalSettleUs, float maxResistanceOhms,
                         PadReading* out) {
    groundAndDischarge(mux, forceCh, sinkCh);  // leaves sinkCh→Bus::B closed

    mux.setChannel(forceCh, pullupBus);
    mux.setChannel(forceCh, Bus::A);

    uint16_t times[CAP_SENSE_SAMPLE_COUNT];
    curveSampleTimesUs(totalSettleUs, times);

    uint16_t elapsed = 0;
    for (uint8_t i = 0; i < CAP_SENSE_SAMPLE_COUNT; i++) {
        delayMicroseconds(times[i] - elapsed);
        elapsed = times[i];
        out[i] = classifyVoltage(adc.readVoltage(1), pullupOhms, maxResistanceOhms);
    }
    mux.clearAll();
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
