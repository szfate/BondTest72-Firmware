#include "eeprom_layout.h"
#include <string.h>

static constexpr uint8_t MAGIC[2]     = {0xB7, 0x72};
static constexpr uint8_t HEADER_BYTES = 32;  // bytes [0..31] covered by CRC32
static constexpr uint32_t CRC32_POLY     = 0xEDB88320u;  // CRC-32/ISO-HDLC reflected polynomial
static constexpr uint32_t CRC32_INIT_XOR = 0xFFFFFFFFu;

static uint32_t crc32(const uint8_t* data, uint8_t len) {
    uint32_t crc = CRC32_INIT_XOR;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++)
            crc = (crc & 1u) ? (crc >> 1) ^ CRC32_POLY : (crc >> 1);
    }
    return crc ^ CRC32_INIT_XOR;
}

static void pack32(uint8_t* buf, uint32_t v) {
    buf[0] =  v        & 0xFF;
    buf[1] = (v >>  8) & 0xFF;
    buf[2] = (v >> 16) & 0xFF;
    buf[3] = (v >> 24) & 0xFF;
}

static uint32_t unpack32(const uint8_t* buf) {
    return static_cast<uint32_t>(buf[0])
         | (static_cast<uint32_t>(buf[1]) <<  8)
         | (static_cast<uint32_t>(buf[2]) << 16)
         | (static_cast<uint32_t>(buf[3]) << 24);
}

void eepromSerialize(const EepromData& d, uint8_t buf[EepromData::WIRE_BYTES]) {
    buf[0] = MAGIC[0];
    buf[1] = MAGIC[1];
    buf[2] = static_cast<uint8_t>(d.adapterModel);
    buf[3] = d.adapterVersion;
    buf[4] = d.supportedPadmapIds[0];
    buf[5] = d.supportedPadmapIds[1];
    buf[6] = d.supportedPadmapIds[2];
    buf[7] = d.supportedPadmapIds[3];
    memset(buf + 8, 0x00, 4);  // reserved
    pack32(buf + 12, d.designedLifespan);
    pack32(buf + 16, d.dateOfManufacture);
    pack32(buf + 20, d.insertionCount);
    pack32(buf + 24, d.testCount);
    pack32(buf + 28, d.eolReached);
    pack32(buf + 32, crc32(buf, HEADER_BYTES));
}

bool eepromDeserialize(const uint8_t buf[EepromData::WIRE_BYTES], EepromData& out) {
    if (buf[0] != MAGIC[0] || buf[1] != MAGIC[1])
        return false;
    if (crc32(buf, HEADER_BYTES) != unpack32(buf + 32))
        return false;

    out.adapterModel          = static_cast<AdapterModel>(buf[2]);
    out.adapterVersion        = buf[3];
    out.supportedPadmapIds[0] = buf[4];
    out.supportedPadmapIds[1] = buf[5];
    out.supportedPadmapIds[2] = buf[6];
    out.supportedPadmapIds[3] = buf[7];
    // buf[8..11] reserved
    out.designedLifespan  = unpack32(buf + 12);
    out.dateOfManufacture = unpack32(buf + 16);
    out.insertionCount    = unpack32(buf + 20);
    out.testCount         = unpack32(buf + 24);
    out.eolReached        = unpack32(buf + 28);
    return true;
}
