#include "led_manager.h"
#include <Arduino.h>
#include <string.h>

static constexpr uint8_t YELLOW_R = 170, YELLOW_G =  30, YELLOW_B = 0;
static constexpr uint8_t GREEN_R  =   0, GREEN_G  = 255, GREEN_B  = 0;
static constexpr uint8_t RED_R    = 255, RED_G    =   0, RED_B    = 0;
static constexpr uint8_t DIM_RED_R =  50, DIM_RED_G =  0, DIM_RED_B = 0;

static constexpr uint32_t BLINK_SLOW_MS = 800;
static constexpr uint32_t BLINK_FAST_MS = 200;

// r2 board: two pixels. PASS and FAIL are mutually exclusive, so they share
// one result pixel (r1's separate pass/fail pixels were merged).
static constexpr uint8_t STATUS_LED = 0;  // D1 — top
static constexpr uint8_t RESULT_LED = 1;  // D2 — next to the START button
static_assert(RESULT_LED < LED_PIXEL_COUNT, "LED roles exceed the pixel chain");

LedManager::LedManager(SK6812Controller& leds)
    : _leds(leds)
{
    for (uint8_t i = 0; i < LED_PIXEL_COUNT; i++) {
        _rendered[i][0] = 0xFF;  // sentinel that never matches a real pattern — forces first render
        _rendered[i][1] = 0xFF;
        _rendered[i][2] = 0xFF;
    }
}

// Free function — touches no members. Static so the name can't collide at
// link time.
static bool blinkOn(uint32_t periodMs) {
    return (millis() % periodMs) < (periodMs / 2);
}

// show() shuts interrupts off for ~340 µs per call; the main loop calls
// update() every iteration, so we only push a redraw when the desired pattern
// actually differs from what's on the wire. Blink phases derive from millis(),
// so blinking still redraws exactly twice per period (at the on/off edges)
// while steady states cost nothing.
void LedManager::update(State state) {
    uint8_t desired[LED_PIXEL_COUNT][3] = {};
    switch (state) {
        case State::NO_ADAPTER:
        case State::EOL_ADAPTER:
            for (uint8_t i = 0; i < LED_PIXEL_COUNT; i++) {
                desired[i][0] = DIM_RED_R; desired[i][1] = DIM_RED_G; desired[i][2] = DIM_RED_B;
            }
            break;
        case State::ADAPTER_DETECTED:
            if (blinkOn(BLINK_SLOW_MS)) { desired[STATUS_LED][0] = YELLOW_R; desired[STATUS_LED][1] = YELLOW_G; desired[STATUS_LED][2] = YELLOW_B; }
            break;
        case State::READY:
        case State::TESTING:
            desired[STATUS_LED][0] = YELLOW_R; desired[STATUS_LED][1] = YELLOW_G; desired[STATUS_LED][2] = YELLOW_B;
            break;
        case State::WRONG_ORIENTATION:
            if (blinkOn(BLINK_SLOW_MS)) { desired[STATUS_LED][0] = RED_R; desired[STATUS_LED][1] = RED_G; desired[STATUS_LED][2] = RED_B; }
            break;
        case State::PASS:
            desired[RESULT_LED][0] = GREEN_R; desired[RESULT_LED][1] = GREEN_G; desired[RESULT_LED][2] = GREEN_B;
            break;
        case State::FAIL:
            desired[RESULT_LED][0] = RED_R; desired[RESULT_LED][1] = RED_G; desired[RESULT_LED][2] = RED_B;
            break;
        case State::FAULT:
            // Adapter/tester fault (e.g. blank EEPROM), not a test outcome — the
            // result LED is reserved for PASS/FAIL.
            if (blinkOn(BLINK_FAST_MS)) { desired[STATUS_LED][0] = RED_R; desired[STATUS_LED][1] = RED_G; desired[STATUS_LED][2] = RED_B; }
            break;
    }

    if (memcmp(desired, _rendered, sizeof(desired)) == 0) return;

    _leds.clear();
    for (uint8_t i = 0; i < LED_PIXEL_COUNT; i++) {
        if (desired[i][0] | desired[i][1] | desired[i][2])
            _leds.setPixel(i, desired[i][0], desired[i][1], desired[i][2]);
    }
    _leds.show();
    memcpy(_rendered, desired, sizeof(desired));
}