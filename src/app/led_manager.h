#pragma once
#include "hal/sk6812.h"
#include "app/state.h"

class LedManager {
public:
    explicit LedManager(SK6812Controller& leds);
    void update(State state);

private:
    SK6812Controller& _leds;
    bool blinkOn(uint32_t periodMs);
};