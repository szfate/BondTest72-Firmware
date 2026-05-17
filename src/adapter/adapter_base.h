#pragma once
#include <stdint.h>

class MuxController;
class AdcDriver;

enum class AdapterModel : uint8_t {
    Mezzanine70   = 0x01,
    Mezzanine70x5 = 0x02,
};

class AdapterBase {
public:
    virtual ~AdapterBase() = default;

    virtual uint8_t      getDutCount()           const = 0;
    virtual void         selectDut(uint8_t index)      = 0;  // no-op for single-DUT adapters
    virtual uint8_t      getPadCount()           const = 0;
    virtual AdapterModel getAdapterModel()       const = 0;
    virtual uint8_t      getAdapterVersion()     const = 0;
    virtual uint8_t      getSupportedPadmapId()  const = 0;  // 0xFF = unset
    virtual uint8_t      channelForPin(uint8_t mezPin) const = 0;

    // Adapter-specific hardware self-test (e.g. onboard diode, loopback).
    // Returns true if the adapter hardware checks out.
    virtual bool         selfTest(MuxController& mux, AdcDriver& adc) const = 0;
};
