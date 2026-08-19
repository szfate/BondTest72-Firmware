#pragma once
#include <stdint.h>
#include "mux.h"
#include "adc.h"
#include "test/pad_map.h"
#include "test/result.h"

struct PullupLevel { Bus bus; float ohms; };

// Lowest current (highest resistance) first — sweeping in this order avoids
// hitting bypass caps with strong current before weak. Exactly
// PULLUP_LEVEL_COUNT entries (see pad_map.h).
extern const PullupLevel PULLUP_LEVELS[PULLUP_LEVEL_COUNT];

// Approximate steady-state drive current for a pullup value (I = VCC/R),
// in microamps. Reporting-only — the actual current a real bond sees is
// lower since it divides against the bond resistance too. Used by the host
// protocol to tell the GUI what current=drive each PULLUP_LEVELS entry
// corresponds to, without duplicating VCC here.
float pullupCurrentUA(float pullupOhms);

// The timepoints (elapsed since drive start, in µs) that measureKelvinCurve
// samples at for a given totalSettleUs. Exposed so the host protocol can
// report the same schedule it actually used, instead of re-deriving it.
void curveSampleTimesUs(uint16_t totalSettleUs, uint16_t* out /* [CAP_SENSE_SAMPLE_COUNT] */);

// Drives forceCh through pullupBus while also Kelvin-tapping it on Bus::A
// (same channel, two Y buses closed at once); sinks the other end on Bus::B.
// Reports voltage / apparent resistance / activity for that one reading.
// `conducted` is true when the computed resistance is below maxResistanceOhms
// (pass a value <= 0 if the caller doesn't care about `conducted`, e.g. when
// checking `voltageV` directly against its own criterion).
PadReading measureKelvin(MuxController& mux, AdcDriver& adc,
                          uint8_t forceCh, uint8_t sinkCh, Bus pullupBus, float pullupOhms,
                          uint16_t settleUs, float maxResistanceOhms);

// Sweeps all PULLUP_LEVEL_COUNT levels forward-only (forceCh driven +
// Kelvin-sensed, sinkCh grounded) and reports true if ANY level reads below
// thresholdV — same "any signal at all" philosophy as pad bond detection,
// so a bond path that's weak at one drive strength but not another still
// registers. Used for DUT presence/orientation checks, which need a quick
// present/absent answer rather than the full 6-reading PadResult.
bool kelvinAnyLevelBelow(MuxController& mux, AdcDriver& adc,
                          uint8_t forceCh, uint8_t sinkCh, float thresholdV, uint16_t settleUs);

// Discharges, then drives forceCh through pullupBus (Kelvin-sensed on
// Bus::A), sinkCh sunk to Bus::B, and takes exactly CAP_SENSE_SAMPLE_COUNT
// voltage samples at the schedule from curveSampleTimesUs — WITHOUT
// releasing the connection between samples, so this is one continuous
// charging event, not independent settled readings. Used by CAP_SENSE to
// make the charging curve itself visible (still rising vs already
// plateaued) instead of only reporting a single final value. Each sample's
// `conducted` reflects only that timepoint's resistance vs
// maxResistanceOhms; classification should use
// out[CAP_SENSE_SAMPLE_COUNT-1] (the last/most-settled sample) — the
// earlier ones are for curve-shape visibility, not decision-making.
void measureKelvinCurve(MuxController& mux, AdcDriver& adc,
                         uint8_t forceCh, uint8_t sinkCh, Bus pullupBus, float pullupOhms,
                         uint16_t totalSettleUs, float maxResistanceOhms,
                         PadReading* out /* [CAP_SENSE_SAMPLE_COUNT] */);
