#include "mux.h"
#include "mux_map.h"
#include <Arduino.h>

// Pin assignments — from docs/RP2350 PINMAP.md
static constexpr uint8_t DAT_PINS[3] = {13, 10,  8};  // U2, U3, U4
static constexpr uint8_t CLK_PINS[3] = {14, 11,  9};
static constexpr uint8_t STB_PINS[3] = {12,  5,  4};
static constexpr uint8_t RST_PIN     = 7;

// CH446X serial address encoding.
// Y0–Y3: each Y occupies a contiguous block of 32 addresses (24 valid, 8 unused).
// Y4: its 24 X channels are split into four groups of 6, interleaved after Y0–Y3.
static uint8_t computeAddr(uint8_t y, uint8_t x) {
    if (y <= 3) return (y << 5) | x;
    return 0x18u + (x / 6u) * 0x20u + (x % 6u);
}

// Shift 7-bit address into chip, set switch data, then commit with STB.
// Speed note: replace digitalWrite with gpio_set_mask/gpio_clr_mask (hardware/gpio.h)
// for ~50x speedup (~200 ns/transaction vs ~10 µs). All CH446X setup/hold
// requirements (3–10 ns) are met by back-to-back SIO writes without added delays.
static void writeSwitch(uint8_t chip, uint8_t addr, bool on) {
    const uint8_t dat = DAT_PINS[chip];
    const uint8_t clk = CLK_PINS[chip];
    const uint8_t stb = STB_PINS[chip];

    for (int8_t i = 6; i >= 0; i--) {
        digitalWrite(dat, (addr >> i) & 1u);
        digitalWrite(clk, HIGH);
        digitalWrite(clk, LOW);
    }
    digitalWrite(dat, on ? HIGH : LOW);
    digitalWrite(stb, HIGH);
    digitalWrite(stb, LOW);
}

void MuxController::begin() {
    for (uint8_t i = 0; i < 3; i++) {
        pinMode(DAT_PINS[i], OUTPUT); digitalWrite(DAT_PINS[i], LOW);
        pinMode(CLK_PINS[i], OUTPUT); digitalWrite(CLK_PINS[i], LOW);
        pinMode(STB_PINS[i], OUTPUT); digitalWrite(STB_PINS[i], LOW);
    }
    pinMode(RST_PIN, OUTPUT);
    clearAll();
}

void MuxController::setChannel(uint8_t logicalPad, Bus bus) {
    if (logicalPad >= 72) return;
    const MuxEntry& e = MUX_MAP[logicalPad];
    writeSwitch(e.chip, computeAddr(static_cast<uint8_t>(bus), e.channel), true);
}

void MuxController::clearChannel(uint8_t logicalPad, Bus bus) {
    if (logicalPad >= 72) return;
    const MuxEntry& e = MUX_MAP[logicalPad];
    writeSwitch(e.chip, computeAddr(static_cast<uint8_t>(bus), e.channel), false);
}

void MuxController::clearAll() {
    digitalWrite(RST_PIN, HIGH);
    digitalWrite(RST_PIN, LOW);
}
