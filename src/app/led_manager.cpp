#include "led_manager.h"
#include <Arduino.h>

static constexpr uint8_t YELLOW_R = 170, YELLOW_G =  30, YELLOW_B = 0;
static constexpr uint8_t GREEN_R  =   0, GREEN_G  = 255, GREEN_B  = 0;
static constexpr uint8_t RED_R    = 255, RED_G    =   0, RED_B    = 0;
static constexpr uint8_t DIM_RED_R =  50, DIM_RED_G =  0, DIM_RED_B = 0;

static constexpr uint32_t BLINK_SLOW_MS = 800;
static constexpr uint32_t BLINK_FAST_MS = 200;

LedManager::LedManager(SK6812Controller& leds)
    : _leds(leds)
{
}

bool LedManager::blinkOn(uint32_t periodMs) {
    return (millis() % periodMs) < (periodMs / 2);
}

void LedManager::update(State state) {
    _leds.clear();
    switch (state) {
        case State::NO_ADAPTER:
        case State::EOL_ADAPTER:
            _leds.setAll(DIM_RED_R, DIM_RED_G, DIM_RED_B);
            break;
        case State::ADAPTER_DETECTED:
            if (blinkOn(BLINK_SLOW_MS)) _leds.setPixel(0, YELLOW_R, YELLOW_G, YELLOW_B);
            break;
        case State::READY:
            _leds.setPixel(0, YELLOW_R, YELLOW_G, YELLOW_B);
            break;
        case State::TESTING:
            _leds.setPixel(0, YELLOW_R, YELLOW_G, YELLOW_B);
            break;
        case State::WRONG_ORIENTATION:
            if (blinkOn(BLINK_SLOW_MS)) _leds.setPixel(0, RED_R, RED_G, RED_B);
            break;
        case State::PASS:
            _leds.setPixel(1, GREEN_R, GREEN_G, GREEN_B);
            break;
        case State::FAIL:
            _leds.setPixel(2, RED_R, RED_G, RED_B);
            break;
        case State::FAULT:
            if (blinkOn(BLINK_FAST_MS)) _leds.setPixel(2, RED_R, RED_G, RED_B);
            break;
    }
    _leds.show();
}