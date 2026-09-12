#pragma once
#include "hal/sk6812.h"
#include "app/state.h"

class LedManager {
public:
    explicit LedManager(SK6812Controller& leds);
    void update(State state);

private:
    SK6812Controller& _leds;
    uint8_t _rendered[LED_PIXEL_COUNT][3];  // last pattern pushed to the strip
};