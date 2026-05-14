#include "sk6812.h"
#include <Adafruit_NeoPixel.h>

static constexpr uint8_t DIN_PIN    = 1;  // GP1 — docs/RP2350 PINMAP.md
static constexpr uint8_t NUM_PIXELS = 3;

// SK6812 is an RGBW device — use NEO_GRBW. If your board uses the RGB-only
// SK6812 variant, change to NEO_GRB and remove the w=0 arguments.
static Adafruit_NeoPixel strip(NUM_PIXELS, DIN_PIN, NEO_GRBW + NEO_KHZ800);

void SK6812Controller::begin() {
    strip.begin();
    strip.setBrightness(80);
    strip.clear();
    strip.show();
}

void SK6812Controller::setPixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= NUM_PIXELS) return;
    strip.setPixelColor(index, strip.Color(r, g, b, 0));
}

void SK6812Controller::setAll(uint8_t r, uint8_t g, uint8_t b) {
    for (uint8_t i = 0; i < NUM_PIXELS; i++)
        strip.setPixelColor(i, strip.Color(r, g, b, 0));
}

void SK6812Controller::clear() {
    strip.clear();
}

void SK6812Controller::show() {
    strip.show();
}
