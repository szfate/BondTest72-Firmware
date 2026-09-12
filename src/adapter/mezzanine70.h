#pragma once
#include "adapter_base.h"

struct EepromData;

static constexpr uint8_t ADAPTER_PIN_COUNT = 70;  // mezzanine die pads on this adapter model

class Mezzanine70 : public AdapterBase {
public:
    explicit Mezzanine70(const EepromData& eeprom);

    uint8_t      getDutCount()           const override { return 1; }
    void         selectDut(uint8_t)            override { }  // single-DUT, no-op
    uint8_t      getPadCount()           const override { return ADAPTER_PIN_COUNT; }
    AdapterHardware getAdapterHardware() const override { return AdapterHardware::Mezzanine70; }
    auto getSupportedPadmapIds() const -> const uint8_t (&)[PADMAP_ID_COUNT] override { return _padmapIds; }
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
    bool kelvinPresence(MuxController& mux, AdcDriver& adc,
                        uint8_t pinA, uint8_t pinB, float thresholdV, const char* tag) const;
    uint8_t _padmapIds[PADMAP_ID_COUNT];
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
