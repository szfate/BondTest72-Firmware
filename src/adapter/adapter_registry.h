#pragma once
#include "adapter_base.h"
#include "eeprom_layout.h"

class AdapterRegistry {
public:
    // Constructs the adapter into a static buffer and returns a pointer to it.
    // Returns nullptr if the model is unknown. Call once at boot.
    static AdapterBase* create(const EepromData& eeprom);
};
