#include "eeprom_layout.h"

static constexpr uint8_t  MAGIC[2]       = {0xB7, 0x72};
static constexpr uint8_t  HEADER_BYTES   = 22;  // bytes covered by CRC
static constexpr uint16_t CRC16_POLY     = 0x1021;
static constexpr uint16_t CRC16_INIT     = 0xFFFF;

static uint16_t crc16(const uint8_t* data, uint8_t len) {
    uint16_t crc = CRC16_INIT;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (uint8_t b = 0; b < 8; b++)
            crc = (crc & 0x8000u) ? (crc << 1) ^ CRC16_POLY : (crc << 1);
    }
    return crc;
}

void eepromSerialize(const EepromData& d, uint8_t buf[24]) {
    buf[0]  = MAGIC[0];
    buf[1]  = MAGIC[1];
    buf[2]  = static_cast<uint8_t>(d.adapterModel);
    buf[3]  = d.adapterVersion;
    buf[4]  = d.supportedPadmapId;
    buf[5]  = 0x00;  // reserved
    buf[6]  =  d.designedLifespan        & 0xFF;
    buf[7]  = (d.designedLifespan >> 8)  & 0xFF;
    buf[8]  =  d.dateOfManufacture        & 0xFF;
    buf[9]  = (d.dateOfManufacture >>  8) & 0xFF;
    buf[10] = (d.dateOfManufacture >> 16) & 0xFF;
    buf[11] = (d.dateOfManufacture >> 24) & 0xFF;
    buf[12] =  d.insertionCount        & 0xFF;
    buf[13] = (d.insertionCount >>  8) & 0xFF;
    buf[14] = (d.insertionCount >> 16) & 0xFF;
    buf[15] = (d.insertionCount >> 24) & 0xFF;
    buf[16] =  d.testCount        & 0xFF;
    buf[17] = (d.testCount >>  8) & 0xFF;
    buf[18] = (d.testCount >> 16) & 0xFF;
    buf[19] = (d.testCount >> 24) & 0xFF;
    buf[20] = d.eolReached;
    buf[21] = 0x00;  // reserved

    uint16_t crc = crc16(buf, HEADER_BYTES);
    buf[22] =  crc        & 0xFF;
    buf[23] = (crc >> 8)  & 0xFF;
}

bool eepromDeserialize(const uint8_t buf[24], EepromData& out) {
    if (buf[0] != MAGIC[0] || buf[1] != MAGIC[1])
        return false;

    uint16_t stored = static_cast<uint16_t>(buf[22])
                    | (static_cast<uint16_t>(buf[23]) << 8);
    if (crc16(buf, HEADER_BYTES) != stored)
        return false;

    out.adapterModel      = static_cast<AdapterModel>(buf[2]);
    out.adapterVersion    = buf[3];
    out.supportedPadmapId = buf[4];
    // buf[5] reserved
    out.designedLifespan  = static_cast<uint16_t>(buf[6])
                          | (static_cast<uint16_t>(buf[7]) << 8);
    out.dateOfManufacture = static_cast<uint32_t>(buf[8])
                          | (static_cast<uint32_t>(buf[9])  <<  8)
                          | (static_cast<uint32_t>(buf[10]) << 16)
                          | (static_cast<uint32_t>(buf[11]) << 24);
    out.insertionCount    = static_cast<uint32_t>(buf[12])
                          | (static_cast<uint32_t>(buf[13]) <<  8)
                          | (static_cast<uint32_t>(buf[14]) << 16)
                          | (static_cast<uint32_t>(buf[15]) << 24);
    out.testCount         = static_cast<uint32_t>(buf[16])
                          | (static_cast<uint32_t>(buf[17]) <<  8)
                          | (static_cast<uint32_t>(buf[18]) << 16)
                          | (static_cast<uint32_t>(buf[19]) << 24);
    out.eolReached        = buf[20];
    // buf[21] reserved
    return true;
}
