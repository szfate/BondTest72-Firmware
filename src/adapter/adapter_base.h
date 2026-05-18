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
    virtual uint8_t      channelForPin(uint8_t mezPin) const = 0;

    // Adapter-specific hardware self-test (e.g. onboard diode, loopback).
    // Returns true if the adapter hardware checks out.
    virtual bool         selfTest(MuxController& mux, AdcDriver& adc) const = 0;

    // Drive the adapter's EOL indicator (e.g. onboard LED). Default no-op.
    virtual void         setEolLed(bool on) {}

    // Sweep all IO pads for connector-level shorts to GND (solder bridges etc.).
    // Must only be called when no DUT is present. Returns false if any pad reads
    // suspiciously low. Default no-op returns true.
    virtual bool         connectorIsolationSweep(MuxController& mux, AdcDriver& adc,
                                                 const PadMap& padMap) const { return true; }
};
