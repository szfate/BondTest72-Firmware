#include "adapter_registry.h"
#include "mezzanine70.h"
#include <new>
#include <algorithm>

static_assert(sizeof(Mezzanine70) >= sizeof(AdapterBase));

constexpr size_t BUF_SIZE  = std::max(sizeof(Mezzanine70), sizeof(Mezzanine70r2));
constexpr size_t BUF_ALIGN = std::max(alignof(Mezzanine70), alignof(Mezzanine70r2));

alignas(BUF_ALIGN) static uint8_t _buf[BUF_SIZE];

AdapterBase* AdapterRegistry::create(const EepromData& eeprom) {
    switch (eeprom.adapterHardware) {
        case AdapterHardware::Mezzanine70:
            return new (_buf) Mezzanine70(eeprom);
        case AdapterHardware::Mezzanine70r2:
            return new (_buf) Mezzanine70r2(eeprom);
        default:
            return nullptr;
    }
}
