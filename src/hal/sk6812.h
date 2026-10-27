#pragma once
#include <stdint.h>

// Two pixels. Pixel 0 = D1 (status, top),
// pixel 1 = D2 (result, next to the START button) — see app/led_manager.cpp.
constexpr uint8_t LED_PIXEL_COUNT = 2;

class SK6812Controller {
public:
    void begin();
    void bootShow();   // blocking ~2s LED self-check animation on startup
    void setPixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
    void setAll(uint8_t r, uint8_t g, uint8_t b);
    void clear();
    void show();
};
