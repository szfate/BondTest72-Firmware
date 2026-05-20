#include "mezzanine70.h"
#include "../hal/mux.h"
#include "../hal/adc.h"
#include "../test/pad_map.h"
#include "../debug/log.h"
#include <Arduino.h>

static constexpr uint8_t EOL_LED_PIN = 18;  // CON5 = GP18

Mezzanine70::Mezzanine70(const EepromData& eeprom)
    : _version(eeprom.adapterVersion)
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
        mux.clearAll();
        mux.setChannel(channelForPin(tc.gndPin), Bus::D);
        mux.setChannel(channelForPin(tc.mezPin), Bus::B);
        delay(1);
        float v = adc.readVoltage(0);  // COM_D
        if (v < ISOLATION_SHORT_THRESHOLD_V) {
            LOG_W("connector isolation: amez%u sense=%.3fV SHORT?", tc.mezPin, v);
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
    mux.clearAll();
    mux.setChannel(DIODE_ANODE,   Bus::D);
    mux.setChannel(DIODE_CATHODE, Bus::B);
    delay(1);
    float fwd = adc.readVoltage(0);  // COM_D = anode = Vf

    mux.clearAll();
    mux.setChannel(DIODE_CATHODE, Bus::D);
    mux.setChannel(DIODE_ANODE,   Bus::B);
    delay(1);
    float rev = adc.readVoltage(0);  // COM_D = cathode ≈ 3.3V (blocking)

    mux.clearAll();
    LOG_I("adapter self-test: fwd=%.3fV rev=%.3fV", fwd, rev);
    return fwd > DIODE_FWD_MIN && fwd < DIODE_FWD_MAX && rev > DIODE_REV_MIN;
}
