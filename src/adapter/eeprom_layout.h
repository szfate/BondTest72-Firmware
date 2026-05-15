#pragma once
#include "adapter_base.h"
#include <stdint.h>

struct EepromData {
    static constexpr uint8_t PADMAP_UNSET = 0xFF;  // supportedPadmapId → auto-detect
    static constexpr uint8_t EOL_REACHED  = 0xFF;  // eolReached flag value

    AdapterModel adapterModel;
    uint8_t      adapterVersion;
    uint8_t      supportedPadmapId;  // PADMAP_UNSET = auto-detect
    uint16_t     designedLifespan;   // max insertions before EOL (set at manufacture)
    uint32_t     dateOfManufacture;  // Unix timestamp, little-endian
    uint32_t     insertionCount;     // absent→present transitions (wear metric)
    uint32_t     testCount;          // completed test runs
    uint8_t      eolReached;         // 0x00 = ok, EOL_REACHED = end-of-life
};

// Pack EepromData into 24 bytes (22 header + 2 CRC-16). buf must be at least 24 bytes.
void eepromSerialize(const EepromData& data, uint8_t buf[24]);

// Unpack 24 bytes into EepromData. Returns false if magic sentinel or CRC-16 is wrong.
bool eepromDeserialize(const uint8_t buf[24], EepromData& out);
