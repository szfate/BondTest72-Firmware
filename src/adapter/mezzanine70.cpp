#include "mezzanine70.h"
#include "hal/mux.h"
#include "hal/adc.h"
#include "hal/kelvin.h"
#include "test/pad_map.h"
#include "debug/log.h"
#include <Arduino.h>
#include <math.h>

static constexpr uint8_t EOL_LED_PIN = 18;  // CON5 = GP18
static constexpr uint8_t ADAPTER_PIN_COUNT_PLUS_1 = 71;  // mirror formula for flipped-DUT detection: flipped_pin = 71 - normal_pin

Mezzanine70::Mezzanine70(const EepromData& eeprom)
{
    for (uint8_t i = 0; i < 4; i++) _padmapIds[i] = eeprom.supportedPadmapIds[i];
    pinMode(EOL_LED_PIN, OUTPUT);
    digitalWrite(EOL_LED_PIN, LOW);
}

static constexpr float ISOLATION_SHORT_THRESHOLD_V = 1.5f;

bool Mezzanine70::connectorIsolationSweep(MuxController& mux, AdcDriver& adc,
                                           const PadMap& padMap) const {
    LOG_I("connector isolation sweep: %u pads", padMap.caseCount);
    bool ok = true;
    for (uint8_t i = 0; i < padMap.caseCount; i++) {
        const TestCase& tc = padMap.cases[i];
        PadReading r = measureKelvin(mux, adc, channelForPin(tc.gndPin), channelForPin(tc.adapterPin),
                                      PULLUP_LEVELS[1].bus, PULLUP_LEVELS[1].ohms, 200, 0.0f);
        if (r.voltageV < ISOLATION_SHORT_THRESHOLD_V) {
            LOG_W("connector isolation: apin%u sense=%.3fV SHORT?", tc.adapterPin, r.voltageV);
            ok = false;
        }
    }
    mux.clearAll();
    LOG_I("connector isolation sweep: %s", ok ? "clean" : "SHORTS DETECTED");
    return ok;
}

void Mezzanine70::setEolLed(bool on) {
    digitalWrite(EOL_LED_PIN, on ? HIGH : LOW);
}

void Mezzanine70::tickEolLed() {
    setEolLed((millis() / 500) % 2 == 0);  // 1 Hz blink
}

// Onboard diode between channels 70 (U2 X21) and 71 (U2 X20).
// Anode=70, cathode=71. Drive anode→D (27K pull-up), cathode→B (GND).
// Read COM_D (ch0) at the anode: forward → ~Vf, reverse → ~3.3V.
static constexpr uint8_t DIODE_ANODE   = 70;
static constexpr uint8_t DIODE_CATHODE = 71;
static constexpr float   DIODE_FWD_MIN = 0.3f;
static constexpr float   DIODE_FWD_MAX = 1.0f;
static constexpr float   DIODE_REV_MIN = 2.5f;

bool Mezzanine70::selfTest(MuxController& mux, AdcDriver& adc) const {
    PadReading fwd = measureKelvin(mux, adc, DIODE_ANODE, DIODE_CATHODE,
                                    PULLUP_LEVELS[1].bus, PULLUP_LEVELS[1].ohms, 200, 0.0f);  // anode = Vf
    PadReading rev = measureKelvin(mux, adc, DIODE_CATHODE, DIODE_ANODE,
                                    PULLUP_LEVELS[1].bus, PULLUP_LEVELS[1].ohms, 200, 0.0f);  // cathode ≈ 3.3V (blocking)

    LOG_I("adapter self-test: fwd=%.3fV rev=%.3fV", fwd.voltageV, rev.voltageV);
    return fwd.voltageV > DIODE_FWD_MIN && fwd.voltageV < DIODE_FWD_MAX && rev.voltageV > DIODE_REV_MIN;
}

// r2: same pins (70/71), but a 1k precision resistor instead of a diode, so
// forward/reverse are electrically symmetric — one reading is enough, and
// "pass" means "reads close to the known 1k value" rather than diode-style
// asymmetry. Logged deviation is diagnostic only for now (candidate future
// use: correcting ADC/pullup-chain error against this known-good reference)
// — it is NOT fed back into pad resistance measurements yet.
static constexpr float RESISTOR_EXPECTED_OHMS = 1000.0f;
static constexpr float RESISTOR_TOLERANCE     = 0.20f;  // ±20%, sanity check not precision spec

bool Mezzanine70r2::selfTest(MuxController& mux, AdcDriver& adc) const {
    PadReading r = measureKelvin(mux, adc, DIODE_ANODE, DIODE_CATHODE,
                                  PULLUP_LEVELS[1].bus, PULLUP_LEVELS[1].ohms, 200, 0.0f);

    float deviation = (r.resistanceOhms - RESISTOR_EXPECTED_OHMS) / RESISTOR_EXPECTED_OHMS;
    LOG_I("adapter self-test: measured=%.0fohm expected=%.0fohm dev=%.1f%%",
          r.resistanceOhms, RESISTOR_EXPECTED_OHMS, deviation * 100.0f);

    return fabsf(deviation) < RESISTOR_TOLERANCE;
}

// Sweeps all pullup levels and requires only ONE to show a connection —
// same "any signal at all" philosophy as pad bond detection. A single fixed
// drive strength isn't reliable here: the GND-plane bond path between these
// two "equivalent" pins goes through actual bond wires on the die, and can
// sit above what one drive strength alone can detect even when genuinely
// connected (mirrors why pad bond tests sweep multiple levels too).
bool Mezzanine70::senseDutPresent(MuxController& mux, AdcDriver& adc,
                                    const PadMap& padMap) const {
    bool present = kelvinAnyLevelBelow(mux, adc, channelForPin(padMap.presencePadA),
                                        channelForPin(padMap.presencePadB), padMap.presenceThresholdV, 200);
    LOG_D("dut present: apin%u<->apin%u: %s", padMap.presencePadA, padMap.presencePadB,
          present ? "yes" : "no");
    return present;
}

bool Mezzanine70::senseDutFlipped(MuxController& mux, AdcDriver& adc,
                                    const PadMap& padMap) const {
    uint8_t flippedA = ADAPTER_PIN_COUNT_PLUS_1 - padMap.presencePadA;
    uint8_t flippedB = ADAPTER_PIN_COUNT_PLUS_1 - padMap.presencePadB;
    bool flipped = kelvinAnyLevelBelow(mux, adc, channelForPin(flippedA),
                                        channelForPin(flippedB), padMap.presenceThresholdV, 200);
    LOG_D("dut flipped: apin%u<->apin%u: %s", flippedA, flippedB, flipped ? "yes" : "no");
    return flipped;
}

bool Mezzanine70::checkDutNow(MuxController& mux, AdcDriver& adc,
                                const PadMap& padMap) const {
    return senseDutPresent(mux, adc, padMap);
}
