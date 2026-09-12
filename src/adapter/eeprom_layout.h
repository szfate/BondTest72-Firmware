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

// True when buf's header looks like an erased (blank, unprovisioned) device:
// the AT21CS01 reads back all 0xFF when erased, and the first two bytes are
// the magic — so a header starting 0xFF 0xFF is a blank pending the re-read
// confirmation done by the caller (eeprom_manager).
inline bool eepromHeaderLooksBlank(const uint8_t buf[EepromData::WIRE_BYTES]) {
    return buf[0] == 0xFF && buf[1] == 0xFF;
}

// A PROVISION request as parsed off the wire (host_protocol fills it in).
// Fields the host omitted keep their unset sentinels; the required-field
// checks and the mapping onto EepromData both live here (markUnset /
// eepromFromProvision), so the sentinels and defaults have one home instead
// of being re-derived at every call site.
struct ProvisionRequest {
    uint8_t  hwId;
    uint8_t  padmapIds[AdapterBase::PADMAP_ID_COUNT];
    uint32_t designedLifespan;
    uint32_t dateOfManufacture;
    uint32_t insertionCount;
    uint32_t testCount;
    uint32_t eol;  // host sends 0|1; FIELD_UNSET if omitted

    // Reset to all-unset before parsing a new PROVISION line.
    void markUnset() {
        hwId = EepromData::HWID_UNSET;
        for (uint8_t i = 0; i < AdapterBase::PADMAP_ID_COUNT; i++)
            padmapIds[i] = EepromData::PADMAP_UNSET;
        designedLifespan  = EepromData::FIELD_UNSET;
        dateOfManufacture = EepromData::FIELD_UNSET;
        insertionCount    = EepromData::FIELD_UNSET;
        testCount         = EepromData::FIELD_UNSET;
        eol               = EepromData::FIELD_UNSET;
    }

    bool hasHw()       const { return hwId != EepromData::HWID_UNSET; }
    bool hasPadmap()   const { return padmapIds[0] != EepromData::PADMAP_UNSET; }
    bool hasLifespan() const { return designedLifespan != EepromData::FIELD_UNSET; }
    bool hasDate()     const { return dateOfManufacture != EepromData::FIELD_UNSET; }
};

// Build the EepromData a fresh PROVISION writes: required fields verbatim
// from the request; optional counters default to 0 when the host omitted
// them (FIELD_UNSET); eol=1 maps to the EOL_REACHED flag.
EepromData eepromFromProvision(const ProvisionRequest& req);
