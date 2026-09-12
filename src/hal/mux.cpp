#include "mux.h"
#include "mux_map.h"
#include "debug/log.h"
#include <Arduino.h>

// Pin assignments — from docs/RP2350 PINMAP.md
static constexpr uint8_t DAT_PINS[3] = {13, 10,  8};  // U2, U3, U4
static constexpr uint8_t CLK_PINS[3] = {14, 11,  9};
static constexpr uint8_t STB_PINS[3] = {12,  5,  4};
static constexpr uint8_t RST_PIN     = 7;

// CH446X serial address encoding.
// Y0–Y3: each Y occupies a contiguous block of 32 addresses (24 valid, 8 unused).
// Y4: its 24 X channels are split into four groups of 6, interleaved after Y0–Y3.
static constexpr uint8_t Y4_GROUP_SIZE   = 6;     // X channels per interleaved group
static constexpr uint8_t Y4_GROUP_STRIDE = 0x20;  // 32 addresses per group block
static constexpr uint8_t Y4_BASE         = 0x18;  // start of the Y4 region

static uint8_t computeAddr(uint8_t y, uint8_t x) {
    if (y <= 3) return (y << 5) | x;
    return Y4_BASE + (x / Y4_GROUP_SIZE) * Y4_GROUP_STRIDE + (x % Y4_GROUP_SIZE);
}

// Shift 7-bit address into chip, set switch data, then commit with STB.
// Do NOT swap digitalWrite for gpio_set_mask/gpio_clr_mask as a "50x speedup"
// (~200 ns/transaction vs ~10 µs): the ~10 µs per-switch serial window is part
// of the latchup exposure-window analysis in kelvin.cpp — the ground-reference-
// first sequencing assumes each switch commit takes this long. Optimizing this
// away would invalidate that analysis.
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

void MuxController::setChannel(uint8_t ch, Bus bus) {
    if (ch >= MUX_CHANNEL_COUNT) { LOG_E("mux: invalid channel %u", ch); return; }
    const MuxEntry& e = MUX_MAP[ch];
    LOG_D("mux set  ch=%u chip=%u x=%u bus=%u", ch, e.chip, e.channel, (uint8_t)bus);
    writeSwitch(e.chip, computeAddr(static_cast<uint8_t>(bus), e.channel), true);
}

void MuxController::clearChannel(uint8_t ch, Bus bus) {
    if (ch >= MUX_CHANNEL_COUNT) { LOG_E("mux: invalid channel %u", ch); return; }
    const MuxEntry& e = MUX_MAP[ch];
    LOG_D("mux clr  ch=%u chip=%u x=%u bus=%u", ch, e.chip, e.channel, (uint8_t)bus);
    writeSwitch(e.chip, computeAddr(static_cast<uint8_t>(bus), e.channel), false);
}

void MuxController::clearAll() {
    LOG_D("mux clear all");
    digitalWrite(RST_PIN, HIGH);
    digitalWrite(RST_PIN, LOW);
}
