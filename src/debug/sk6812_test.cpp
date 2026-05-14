#include "sk6812_test.h"
#include "hal/sk6812.h"
#include <Arduino.h>

extern SK6812Controller leds;

// Cycles through LED behaviour states every 800 ms so all pixels and colours
// can be verified visually.
void sk6812Test() {
    static uint8_t  step   = 0;
    static uint32_t lastMs = 0;

    if (millis() - lastMs < 800) return;
    lastMs = millis();

    leds.clear();
    switch (step % 5) {
        case 0: leds.setPixel(0, 255, 180,   0); break;  // LED 0 yellow — READY
        case 1: leds.setPixel(1,   0, 255,   0); break;  // LED 1 green  — PASS
        case 2: leds.setPixel(2, 255,   0,   0); break;  // LED 2 red    — FAIL
        case 3: leds.setAll(255, 180, 0);         break;  // all yellow
        case 4: break;                                    // all off
    }
    leds.show();
    step++;
}
