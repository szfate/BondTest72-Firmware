#include "adapter_registry.h"
#include "adapter_base.h"
#include "eeprom_layout.h"
#include "mezzanine70.h"
#include <new>
#include <algorithm>
#include <type_traits>

static_assert(sizeof(Mezzanine70) >= sizeof(AdapterBase));
static_assert(std::is_base_of_v<AdapterBase, Mezzanine70>);
static_assert(std::is_base_of_v<AdapterBase, Mezzanine70r2>);

constexpr size_t BUF_SIZE  = std::max(sizeof(Mezzanine70), sizeof(Mezzanine70r2));
constexpr size_t BUF_ALIGN = std::max(alignof(Mezzanine70), alignof(Mezzanine70r2));

alignas(BUF_ALIGN) static uint8_t _buf[BUF_SIZE];
static bool _bufOccupied = false;

AdapterBase* AdapterRegistry::create(const EepromData& eeprom) {
    // create() runs on every re-detection, so the buffer may still hold a live
    // adapter. The concrete types are NOT trivially destructible (AdapterBase has
    // a virtual dtor), so reusing the buffer without ending the previous object's
    // lifetime would be UB — end it explicitly first.
    if (_bufOccupied) {
        reinterpret_cast<AdapterBase*>(_buf)->~AdapterBase();
        _bufOccupied = false;
    }
    AdapterBase* a = nullptr;
    switch (eeprom.adapterHardware) {
        case AdapterHardware::Mezzanine70:
            a = new (_buf) Mezzanine70(eeprom);
            break;
        case AdapterHardware::Mezzanine70r2:
            a = new (_buf) Mezzanine70r2(eeprom);
            break;
        default:
            return nullptr;
    }
    _bufOccupied = true;
    return a;
}
