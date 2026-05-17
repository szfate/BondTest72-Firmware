#include "sk6812.h"
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

static constexpr uint8_t DIN_PIN    = 1;  // GP1 — docs/RP2350 PINMAP.md
static constexpr uint8_t NUM_PIXELS = 3;

static Adafruit_NeoPixel strip(NUM_PIXELS, DIN_PIN, NEO_GRB + NEO_KHZ800);

void SK6812Controller::begin() {
    strip.begin();
    strip.setBrightness(80);
    strip.clear();
    strip.show();
}

void SK6812Controller::bootShow() {
    // Chase each colour (R→G→B) across all 3 pixels: 9 × 150 ms = 1350 ms
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
            delay(150);
        }
    }
    // All white for 350 ms, then off
    for (uint8_t i = 0; i < NUM_PIXELS; i++)
        strip.setPixelColor(i, strip.Color(200, 200, 200));
    strip.show();
    delay(350);
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
