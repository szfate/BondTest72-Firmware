#pragma once
#include <stdint.h>

struct EepromData;
class AT21CS01Driver;

class EepromManager {
public:
    enum class ReadResult { Ok, Blank, CrcError, IoError };

    // Write attempts: initial try + one retry after a failed read-back verify.
    static constexpr uint8_t MAX_WRITE_ATTEMPTS = 2;

    explicit EepromManager(AT21CS01Driver& eeprom);

    bool isPresent();
    ReadResult read(EepromData& out);
    bool write(const EepromData& data);

    bool readSerialUid(char* buf, uint8_t bufLen);

private:
    AT21CS01Driver& _eeprom;
};