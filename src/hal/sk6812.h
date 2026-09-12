#pragma once
#include <stdint.h>

constexpr uint8_t LED_PIXEL_COUNT = 3;

class SK6812Controller {
public:
    void begin();
    void bootShow();   // blocking ~2s LED self-check animation on startup
    void setPixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
    void setAll(uint8_t r, uint8_t g, uint8_t b);
    void clear();
    void show();
};
