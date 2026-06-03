#pragma once
#include "adapter_base.h"
#include "eeprom_layout.h"

class Mezzanine70 : public AdapterBase {
public:
    explicit Mezzanine70(const EepromData& eeprom);

    uint8_t      getDutCount()           const override { return 1; }
    void         selectDut(uint8_t)            override { }  // single-DUT, no-op
    uint8_t      getPadCount()           const override { return 70; }
    AdapterModel getAdapterModel()       const override { return AdapterModel::Mezzanine70; }
    uint8_t      getAdapterVersion()     const override { return _version; }
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
    uint8_t _version;
    uint8_t _padmapIds[4];
};
