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
    uint8_t      getSupportedPadmapId()  const override { return _padmapId; }
    uint8_t      channelForPin(uint8_t mezPin) const override { return mezPin - 1; }
    bool         selfTest(MuxController& mux, AdcDriver& adc) const override;

private:
    uint8_t _version;
    uint8_t _padmapId;
};
