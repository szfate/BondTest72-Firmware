#include "eeprom_manager.h"
#include <Arduino.h>
#include "debug/log.h"

EepromManager::EepromManager(AT21CS01Driver& eeprom)
    : _eeprom(eeprom)
{
}

bool EepromManager::isPresent() {
    return _eeprom.isPresent();
}

EepromManager::ReadResult EepromManager::read(EepromData& out) {
    uint8_t buf[EepromData::WIRE_BYTES];
    if (!_eeprom.read(0, buf, EepromData::WIRE_BYTES)) {
        LOG_E("eeprom: read failed");
        return ReadResult::CrcError;
    }
    LOG_I("eeprom[0..7]: %02X %02X %02X %02X %02X %02X %02X %02X",
          buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
    if (buf[0] == 0xFF && buf[1] == 0xFF) {
        return ReadResult::Blank;
    }
    if (!eepromDeserialize(buf, out)) {
        LOG_E("eeprom: deserialize failed (CRC mismatch)");
        return ReadResult::CrcError;
    }
    return ReadResult::Ok;
}

bool EepromManager::write(const EepromData& data) {
    uint8_t buf[EepromData::WIRE_BYTES];
    eepromSerialize(data, buf);
    if (!_eeprom.write(0, buf, EepromData::WIRE_BYTES)) {
        LOG_E("eeprom: write failed");
        return false;
    }
    return true;
}