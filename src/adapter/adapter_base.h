#pragma once
#include <stdint.h>

class MuxController;
class AdcDriver;
struct PadMap;

enum class AdapterHardware : uint8_t {
    Mezzanine70   = 0x01,
    Mezzanine70r2 = 0x02,
};

// Physical DUT-connector pinout an adapter exposes. Pad maps declare the
// pinout they are written for (PadMap::pinout), and any pad map with a
// matching pinout may run on the adapter — independent of the padmap ID list
// in the adapter EEPROM, which only supplies the default. Hardware revisions
// that keep the same connector wiring share a pinout (Mezzanine70 r1/r2).
enum class AdapterPinout : uint8_t {
    Mezzanine70 = 1,
};

class AdapterBase {
public:
    // Length of the padmap-id list. Fixed by the EEPROM wire layout
    // (EepromData bytes [4..7]) — every adapter's list is exactly this long.
    static constexpr uint8_t PADMAP_ID_COUNT = 4;

    virtual ~AdapterBase() = default;

    virtual uint8_t      getDutCount()           const = 0;
    virtual void         selectDut(uint8_t index)      = 0;
    virtual uint8_t      getPadCount()           const = 0;
    virtual AdapterHardware getAdapterHardware()  const = 0;
    virtual AdapterPinout   getPinout()           const = 0;
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
