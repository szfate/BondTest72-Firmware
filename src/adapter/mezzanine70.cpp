#include "mezzanine70.h"

Mezzanine70::Mezzanine70(const EepromData& eeprom)
    : _version(eeprom.adapterVersion)
    , _padmapId(eeprom.supportedPadmapId)
{
}
