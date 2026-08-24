#pragma once
#include "adapter_base.h"
#include "eeprom_layout.h"

class Mezzanine70 : public AdapterBase {
public:
    explicit Mezzanine70(const EepromData& eeprom);

    uint8_t      getDutCount()           const override { return 1; }
    void         selectDut(uint8_t)            override { }  // single-DUT, no-op
    uint8_t      getPadCount()           const override { return 70; }
    AdapterHardware getAdapterHardware() const override { return AdapterHardware::Mezzanine70; }
    const uint8_t* getSupportedPadmapIds() const override { return _padmapIds; }
    uint8_t      channelForPin(uint8_t adapterPin) const override { return adapterPin - 1; }
    bool         selfTest(MuxController& mux, AdcDriver& adc) const override;
    bool         connectorIsolationSweep(MuxController& mux, AdcDriver& adc,
                                         const PadMap& padMap) const override;
    bool         senseDutPresent(MuxController& mux, AdcDriver& adc,
                                 const PadMap& padMap) const override;
    bool         senseDutFlipped(MuxController& mux, AdcDriver& adc,
                                  const PadMap& padMap) const override;
    bool         checkDutNow(MuxController& mux, AdcDriver& adc,
                              const PadMap& padMap) const override;
    void         setEolLed(bool on) override;
    void         tickEolLed()       override;

private:
    uint8_t _padmapIds[4];
};

// r2: onboard diode (pins 70/71) replaced with a 1k precision resistor.
// Everything else about the board is unchanged, so this only overrides the
// self-test — see mezzanine70.cpp for what changed and why.
class Mezzanine70r2 : public Mezzanine70 {
public:
    explicit Mezzanine70r2(const EepromData& eeprom) : Mezzanine70(eeprom) {}

    AdapterHardware getAdapterHardware() const override { return AdapterHardware::Mezzanine70r2; }
    bool            selfTest(MuxController& mux, AdcDriver& adc) const override;
};
