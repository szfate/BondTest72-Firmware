#pragma once
#include <stdint.h>

class SK6812Controller {
public:
    void begin();
    void setPixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
    void setAll(uint8_t r, uint8_t g, uint8_t b);
    void clear();
    void show();
};
