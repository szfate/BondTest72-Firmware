#include "eeprom_test.h"
#include "hal/at21cs01.h"
#include <Arduino.h>

extern AT21CS01Driver eeprom;

// GP17 — matches SWI_PIN in at21cs01.cpp. Needed here to simulate the adapter's
// pull-up resistor (normally provided by the adapter board hardware).
static constexpr uint8_t SWI_PIN = 17;

// Scratch address in the reserved area (bytes 24–127) — safe to overwrite.
static constexpr uint8_t SCRATCH_ADDR = 0x7F;

void eepromTest() {
    // Enable internal pull-up to simulate the adapter board's pull-up resistor.
    // Remove once real hardware is available — the adapter provides this in production.
    pinMode(SWI_PIN, INPUT_PULLUP);

    Serial.println("\n--- EEPROM test ---");

    // 1. Presence check
    bool present = eeprom.ping();
    Serial.printf("ping:         %s\n", present ? "OK" : "FAIL — no device");
    if (!present) return;

    // 2. Hex-dump first 24 bytes (the header fields per ARCHITECTURE.md)
    uint8_t buf[24];
    if (eeprom.read(0x00, buf, sizeof(buf))) {
        Serial.println("read 0x00–0x17:");
        for (uint8_t i = 0; i < sizeof(buf); i++) {
            Serial.printf("%02X%c", buf[i], (i & 7) == 7 ? '\n' : ' ');
        }
    } else {
        Serial.println("read:         FAIL");
        return;
    }

    // 3. Write a known pattern then read back to verify round-trip
    const uint8_t pattern[4] = {0xA5, 0x5A, 0xDE, 0xAD};
    uint8_t verify[4]         = {};

    if (!eeprom.write(SCRATCH_ADDR - 3, pattern, sizeof(pattern))) {
        Serial.println("write:        FAIL");
        return;
    }
    Serial.printf("write 0x%02X:  OK\n", SCRATCH_ADDR - 3);

    if (!eeprom.read(SCRATCH_ADDR - 3, verify, sizeof(verify))) {
        Serial.println("readback:     FAIL");
        return;
    }

    bool match = memcmp(pattern, verify, sizeof(pattern)) == 0;
    Serial.printf("readback:     %s", match ? "OK" : "MISMATCH — got");
    if (!match) {
        for (uint8_t i = 0; i < sizeof(verify); i++)
            Serial.printf(" %02X", verify[i]);
    }
    Serial.println();

    // Restore plain INPUT so isPresent() works correctly after the test
    eeprom.begin();
}
