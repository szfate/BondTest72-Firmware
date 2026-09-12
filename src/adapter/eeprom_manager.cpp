#include "eeprom_manager.h"
#include "eeprom_layout.h"
#include "hal/at21cs01.h"
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
        LOG_E("eeprom: read failed (transport, not CRC)");
        return ReadResult::IoError;
    }
    LOG_D("eeprom[0..7]: %02X %02X %02X %02X %02X %02X %02X %02X",
      buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
    if (eepromHeaderLooksBlank(buf)) {
        // A floating SWI line (marginal contact) also reads as 0xFF and can't be
        // told apart from a genuinely blank header within one read — the device
        // ACKed discovery, so the transport "succeeded". Require the blank to
        // reproduce after a fresh ping before declaring it; a bounce-induced
        // blank won't. Anything else is treated as a transient transport failure.
        uint8_t confirm[EepromData::WIRE_BYTES];
        if (_eeprom.ping() && _eeprom.read(0, confirm, EepromData::WIRE_BYTES) &&
            eepromHeaderLooksBlank(confirm)) {
            return ReadResult::Blank;
        }
        LOG_W("eeprom: blank header did not reproduce — treating as transport failure");
        return ReadResult::IoError;
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

bool EepromManager::readSerialUid(char* buf, uint8_t bufLen) {
    if (bufLen < 17) return false;
    uint8_t serial[8];
    if (!_eeprom.readSerial(serial)) return false;
    static const char hex[] = "0123456789ABCDEF";
    for (uint8_t i = 0; i < 8; i++) {
        buf[i * 2]     = hex[serial[i] >> 4];
        buf[i * 2 + 1] = hex[serial[i] & 0x0F];
    }
    buf[16] = '\0';
    return true;
}