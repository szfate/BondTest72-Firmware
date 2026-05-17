#include "mezzanine70.h"
#include "../hal/mux.h"
#include "../hal/adc.h"
#include "../debug/log.h"
#include <Arduino.h>

Mezzanine70::Mezzanine70(const EepromData& eeprom)
    : _version(eeprom.adapterVersion)
    , _padmapId(eeprom.supportedPadmapId)
{
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
