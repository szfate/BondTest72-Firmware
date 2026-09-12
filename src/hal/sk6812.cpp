#include "sk6812.h"
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

static constexpr uint8_t DIN_PIN    = 1;  // GP1 — docs/RP2350 PINMAP.md
static constexpr uint8_t NUM_PIXELS = 3;

// Boot animation look-and-feel (cosmetic; timing not functional)
static constexpr uint8_t  BOOT_BRIGHTNESS  = 80;
static constexpr uint32_t BOOT_STEP_MS     = 150;  // per-pixel chase step
static constexpr uint32_t BOOT_WHITE_MS    = 350;
static constexpr uint8_t  BOOT_WHITE_LEVEL = 200;  // per-channel white intensity

static Adafruit_NeoPixel strip(NUM_PIXELS, DIN_PIN, NEO_GRB + NEO_KHZ800);

void SK6812Controller::begin() {
    strip.begin();
    strip.setBrightness(BOOT_BRIGHTNESS);
    strip.clear();
    strip.show();
}

void SK6812Controller::bootShow() {
    // Chase each colour (R→G→B) across all 3 pixels: 9 × BOOT_STEP_MS = 1350 ms
    static const uint8_t COLORS[3][3] = {
        {255,   0,   0},  // red
        {  0, 255,   0},  // green
        {  0,   0, 255},  // blue
    };
    for (uint8_t c = 0; c < 3; c++) {
        for (uint8_t i = 0; i < NUM_PIXELS; i++) {
            strip.clear();
            strip.setPixelColor(i, strip.Color(COLORS[c][0], COLORS[c][1], COLORS[c][2]));
            strip.show();
            delay(BOOT_STEP_MS);
        }
    }
    // All white for BOOT_WHITE_MS, then off
    for (uint8_t i = 0; i < NUM_PIXELS; i++)
        strip.setPixelColor(i, strip.Color(BOOT_WHITE_LEVEL, BOOT_WHITE_LEVEL, BOOT_WHITE_LEVEL));
    strip.show();
    delay(BOOT_WHITE_MS);
    strip.clear();
    strip.show();
}

void SK6812Controller::setPixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= NUM_PIXELS) return;
    strip.setPixelColor(index, strip.Color(r, g, b));
}

void SK6812Controller::setAll(uint8_t r, uint8_t g, uint8_t b) {
    for (uint8_t i = 0; i < NUM_PIXELS; i++)
        strip.setPixelColor(i, strip.Color(r, g, b));
}

void SK6812Controller::clear() {
    strip.clear();
}

void SK6812Controller::show() {
    strip.show();
}
