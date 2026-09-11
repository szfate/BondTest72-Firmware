#pragma once
#include "adapter_base.h"
#include "eeprom_layout.h"

class AdapterRegistry {
public:
    // Constructs the adapter into a static buffer and returns a pointer to it.
    // Returns nullptr if the model is unknown. Called on every adapter re-detection
    // (not just boot); ends the previous occupant's lifetime before reusing the
    // buffer — see adapter_registry.cpp.
    static AdapterBase* create(const EepromData& eeprom);
};
