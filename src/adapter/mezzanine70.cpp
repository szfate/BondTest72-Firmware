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
    LOG_I("connector isolation sweep: %u test steps", padMap.caseCount);
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

// Onboard diode between tester channels 70 (U2 X21) and 71 (U2 X20) — these are
// tester channels, NOT adapter pins; don't run them through channelForPin().
// Anode=70, cathode=71. Drive anode→D (27.4k pull-up), cathode→B (GND).
// Read COM_A (Kelvin sense) at the anode: forward → ~Vf, reverse → ~3.3V.
static constexpr uint8_t DIODE_ANODE_CH   = 70;
static constexpr uint8_t DIODE_CATHODE_CH = 71;
static constexpr float   DIODE_FWD_MIN = 0.3f;
static constexpr float   DIODE_FWD_MAX = 1.0f;
static constexpr float   DIODE_REV_MIN = 2.5f;

bool Mezzanine70::selfTest(MuxController& mux, AdcDriver& adc) const {
    PadReading fwd = measureKelvin(mux, adc, DIODE_ANODE_CH, DIODE_CATHODE_CH,
                                    PULLUP_LEVELS[1].bus, PULLUP_LEVELS[1].ohms, 200, 0.0f);  // anode = Vf
    PadReading rev = measureKelvin(mux, adc, DIODE_CATHODE_CH, DIODE_ANODE_CH,
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
    PadReading r = measureKelvin(mux, adc, DIODE_ANODE_CH, DIODE_CATHODE_CH,
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
// Shared core of senseDutPresent/senseDutFlipped.
bool Mezzanine70::kelvinPresence(MuxController& mux, AdcDriver& adc,
                                  uint8_t pinA, uint8_t pinB, float thresholdV, const char* tag) const {
    bool connected = kelvinAnyLevelBelow(mux, adc, channelForPin(pinA),
                                         channelForPin(pinB), thresholdV, 200);
    LOG_D("%s: apin%u<->apin%u: %s", tag, pinA, pinB, connected ? "yes" : "no");
    return connected;
}

bool Mezzanine70::senseDutPresent(MuxController& mux, AdcDriver& adc,
                                    const PadMap& padMap) const {
    return kelvinPresence(mux, adc, padMap.presencePadA, padMap.presencePadB,
                          padMap.presenceThresholdV, "dut present");
}

bool Mezzanine70::senseDutFlipped(MuxController& mux, AdcDriver& adc,
                                    const PadMap& padMap) const {
    return kelvinPresence(mux, adc,
                          ADAPTER_PIN_COUNT_PLUS_1 - padMap.presencePadA,
                          ADAPTER_PIN_COUNT_PLUS_1 - padMap.presencePadB,
                          padMap.presenceThresholdV, "dut flipped");
}

bool Mezzanine70::checkDutNow(MuxController& mux, AdcDriver& adc,
                                const PadMap& padMap) const {
    return senseDutPresent(mux, adc, padMap);
}
