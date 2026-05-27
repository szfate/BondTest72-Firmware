#pragma once
#include "eeprom_layout.h"
#include "hal/at21cs01.h"

class EepromManager {
public:
    enum class ReadResult { Ok, Blank, CrcError };

    explicit EepromManager(AT21CS01Driver& eeprom);

    bool isPresent();
    ReadResult read(EepromData& out);
    bool write(const EepromData& data);

private:
    AT21CS01Driver& _eeprom;
};