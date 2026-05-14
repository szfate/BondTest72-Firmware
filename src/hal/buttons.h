#pragma once
#include <stdint.h>

class Buttons {
public:
    void begin();
    void poll();          // call every loop iteration
    bool startPressed();  // true once per falling edge (active-low button)
private:
    bool     _lastRaw    = true;
    bool     _stable     = true;
    bool     _pending    = false;
    uint32_t _lastChange = 0;
};
