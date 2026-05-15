#pragma once
#include "pad_map.h"

class PadMapRegistry {
public:
    // Find a pad map by ID. Returns nullptr if not found.
    static const PadMap* find(uint8_t id);

    // Iterate all pad maps (used for auto-detect).
    static const PadMap* all();
    static uint8_t       count();
};
