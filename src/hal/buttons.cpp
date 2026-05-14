#include "buttons.h"
#include <Arduino.h>

static constexpr uint8_t  START_PIN   = 0;   // GP0 — active low, internal pullup
static constexpr uint32_t DEBOUNCE_MS = 20;

void Buttons::begin() {
    pinMode(START_PIN, INPUT_PULLUP);
}

void Buttons::poll() {
    bool raw = digitalRead(START_PIN);
    if (raw != _lastRaw) {
        _lastChange = millis();
        _lastRaw    = raw;
    }
    if ((millis() - _lastChange) >= DEBOUNCE_MS && raw != _stable) {
        _stable = raw;
        if (!_stable)   // falling edge = button pressed
            _pending = true;
    }
}

bool Buttons::startPressed() {
    if (!_pending) return false;
    _pending = false;
    return true;
}
