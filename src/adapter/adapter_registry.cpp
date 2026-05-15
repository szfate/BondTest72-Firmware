#include "adapter_registry.h"
#include "mezzanine70.h"
#include <new>

static_assert(sizeof(Mezzanine70) >= sizeof(AdapterBase));

alignas(Mezzanine70) static uint8_t _buf[sizeof(Mezzanine70)];

AdapterBase* AdapterRegistry::create(const EepromData& eeprom) {
    switch (eeprom.adapterModel) {
        case AdapterModel::Mezzanine70:
            return new (_buf) Mezzanine70(eeprom);
        default:
            return nullptr;
    }
}
