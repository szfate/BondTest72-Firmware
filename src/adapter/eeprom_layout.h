#pragma once
#include "adapter_base.h"
#include <stdint.h>

struct EepromData {
    static constexpr uint8_t  PADMAP_UNSET = 0xFF;            // unused pad map slot
    static constexpr uint32_t EOL_REACHED  = 0xFFFFFFFFu;     // eolReached flag value
    static constexpr uint8_t  WIRE_BYTES   = 36;              // total on-wire size (32 header + 4 CRC)

    AdapterModel adapterModel;
    uint8_t      adapterVersion;
    uint8_t      supportedPadmapIds[4];  // PADMAP_UNSET-terminated list of supported pad map IDs
    uint32_t     designedLifespan;       // max insertions before EOL (set at manufacture)
    uint32_t     dateOfManufacture;      // YYYYMMDD
    uint32_t     insertionCount;         // absent→present transitions (wear metric)
    uint32_t     testCount;              // completed test runs
    uint32_t     eolReached;             // 0 = ok, EOL_REACHED = end-of-life
};

// Pack EepromData into EEPROM_WIRE_BYTES (32 header + 4 CRC-32). buf must be at least EEPROM_WIRE_BYTES.
// Layout: [0..1] magic  [2] model  [3] version  [4..7] padmapIds  [8..11] reserved
//         [12..15] designedLifespan  [16..19] dateOfManufacture
//         [20..23] insertionCount    [24..27] testCount  [28..31] eolReached  [32..35] CRC32
void eepromSerialize(const EepromData& data, uint8_t buf[EepromData::WIRE_BYTES]);

// Unpack EepromData::WIRE_BYTES into EepromData. Returns false if magic sentinel or CRC-32 is wrong.
bool eepromDeserialize(const uint8_t buf[EepromData::WIRE_BYTES], EepromData& out);
