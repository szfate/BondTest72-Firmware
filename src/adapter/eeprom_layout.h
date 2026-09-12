#pragma once
#include "adapter_base.h"
#include <stdint.h>

struct EepromData {
    static constexpr uint8_t  PADMAP_UNSET = 0xFF;            // unused pad map slot
    static constexpr uint8_t  HWID_UNSET   = 0xFF;            // no adapter hardware provisioned
    static constexpr uint32_t FIELD_UNSET  = 0xFFFFFFFFu;     // unset 32-bit field (lifespan, mfg_date, ins, tests)
    static constexpr uint32_t EOL_REACHED  = 0xFFFFFFFFu;     // eolReached flag value
    static constexpr uint8_t  WIRE_BYTES   = 36;              // total on-wire size (32 header + 4 CRC)

    AdapterHardware adapterHardware;
    uint8_t         rfu;  // reserved, written 0xFF — earmarked as layout_version if a second header layout ever exists
    uint8_t         supportedPadmapIds[AdapterBase::PADMAP_ID_COUNT];  // PADMAP_UNSET-terminated list of supported pad map IDs
    uint32_t     designedLifespan;       // max insertions before EOL (set at manufacture)
    uint32_t     dateOfManufacture;      // YYYYMMDD
    uint32_t     insertionCount;         // absent→present transitions (wear metric)
    uint32_t     testCount;              // completed test runs
    uint32_t     eolReached;             // 0 = ok, EOL_REACHED = end-of-life
};

// Pack EepromData into EepromData::WIRE_BYTES (32 header + 4 CRC-32). buf must be at least EepromData::WIRE_BYTES long.
// Layout: [0..1] magic  [2] hwId  [3] rfu (candidate: layout_version)  [4..7] padmapIds  [8..11] reserved
//         [12..15] designedLifespan  [16..19] dateOfManufacture
//         [20..23] insertionCount    [24..27] testCount  [28..31] eolReached  [32..35] CRC32
void eepromSerialize(const EepromData& data, uint8_t buf[EepromData::WIRE_BYTES]);

// Unpack EepromData::WIRE_BYTES into EepromData. Returns false if magic sentinel or CRC-32 is wrong.
bool eepromDeserialize(const uint8_t buf[EepromData::WIRE_BYTES], EepromData& out);
