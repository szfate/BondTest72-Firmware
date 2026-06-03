#pragma once
#include <stdint.h>

class MuxController;
class AdcDriver;
struct PadMap;

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
    virtual const uint8_t* getSupportedPadmapIds() const = 0;  // 4-entry array, PADMAP_UNSET terminated
    virtual uint8_t      channelForPin(uint8_t adapterPin) const = 0;

    virtual bool         selfTest(MuxController& mux, AdcDriver& adc) const = 0;

    virtual void         setEolLed(bool on) {}
    virtual void         tickEolLed()       {}

    virtual bool         connectorIsolationSweep(MuxController& mux, AdcDriver& adc,
                                                 const PadMap& padMap) const { return true; }

    virtual bool         senseDutPresent(MuxController& mux, AdcDriver& adc,
                                          const PadMap& padMap) const = 0;
    virtual bool         senseDutFlipped(MuxController& mux, AdcDriver& adc,
                                          const PadMap& padMap) const { return false; }
    virtual bool         checkDutNow(MuxController& mux, AdcDriver& adc,
                                      const PadMap& padMap) const = 0;
};
