#include "pad_map_registry.h"

// — Pad map 1: placeholder —————————————————————————————————————————————————
// TODO: fill ring order from MEZ_CONNECTOR_MAP.md and set real presence pads.
// Testable pads: 56 IO + 4 VDDIO + 2 VDD_CORE + 2 PWR_AUX = 64 total.
// GND pads are excluded — all shorted together on the adapter, bonds not individually detectable.

static TestCase _pm1Cases[64];

static void buildPm1() {
    // Mez pins in physical ring order, GND mez pin
    // static const uint8_t ring[] = { ... };  // TODO
    // static const uint8_t gnd   = ...;       // TODO
    // buildRingCases(_pm1Cases, ring, sizeof(ring), gnd);
}

static const PadMap _maps[] = {
    {
        .id                 = 1,
        .name               = "Placeholder",
        .cases              = _pm1Cases,
        .caseCount          = 0,            // TODO: set to ring length once filled
        .presencePadA       = 0,            // TODO: mezzanine GND pin
        .presencePadB       = 0,            // TODO: second mezzanine GND pin
        .presenceThresholdV = 0.3f,
        .senseGoodMin       = 0.5f,
        .senseGoodMax       = 0.7f,
        .neighbourGoodMin   = 0.3f,
        .neighbourGoodMax   = 2.0f,
    },
};

static constexpr uint8_t MAP_COUNT = sizeof(_maps) / sizeof(_maps[0]);

// Called once from setup() before any pad map is used.
static bool _built = false;
static void ensureBuilt() {
    if (_built) return;
    buildPm1();
    _built = true;
}

// ——————————————————————————————————————————————————————————————————————————

const PadMap* PadMapRegistry::find(uint8_t id) {
    ensureBuilt();
    for (uint8_t i = 0; i < MAP_COUNT; i++) {
        if (_maps[i].id == id) return &_maps[i];
    }
    return nullptr;
}

const PadMap* PadMapRegistry::all() {
    ensureBuilt();
    return _maps;
}

uint8_t PadMapRegistry::count() {
    return MAP_COUNT;
}
