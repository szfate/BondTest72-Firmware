#pragma once
#include <stdint.h>

class AdapterBase;
struct EepromData;

class AdapterRegistry {
public:
    // Constructs the adapter into a static buffer and returns a pointer to it.
    // Returns nullptr if the model is unknown. Called on every adapter re-detection
    // (not just boot); ends the previous occupant's lifetime before reusing the
    // buffer — see adapter_registry.cpp.
    static AdapterBase* create(const EepromData& eeprom);
    // True if create() can build this hardware ID. PROVISION checks it before
    // writing a record the tester could never use.
    static bool isKnown(uint8_t hwId);
};
