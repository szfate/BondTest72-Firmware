#include "eeprom_test.h"
#include "hal/at21cs01.h"
#include "adapter/eeprom_layout.h"
#include "adapter/adapter_base.h"
#include <Arduino.h>

extern AT21CS01Driver eeprom;

// Scratch address in the reserved area (bytes 24–127) — safe to overwrite.
static constexpr uint8_t SCRATCH_ADDR = 0x7F;

void eepromErase() {
    Serial.println("\n--- EEPROM erase (fill 0xFF) ---");
    uint8_t blank[128];
    memset(blank, 0xFF, sizeof(blank));
    if (eeprom.write(0x00, blank, sizeof(blank)))
        Serial.println("erase: OK");
    else
        Serial.println("erase: FAIL");
}

void eepromTest() {
    Serial.println("\n--- EEPROM test ---");

    // 1. Presence check
    bool present = eeprom.ping();
    Serial.printf("ping:         %s\n", present ? "OK" : "FAIL — no device");
    if (!present) return;

    // 2. Hex-dump first 36 bytes (header)
    uint8_t buf[36];
    if (eeprom.read(0x00, buf, sizeof(buf))) {
        Serial.println("read 0x00–0x21:");
        for (uint8_t i = 0; i < sizeof(buf); i++) {
            Serial.printf("%02X%c", buf[i], (i & 7) == 7 ? '\n' : ' ');
        }
    } else {
        Serial.println("read:         FAIL");
        return;
    }

    // 3. Provision if blank
    if (buf[0] == 0xFF && buf[1] == 0xFF) {
        Serial.println("header blank — provisioning defaults");
        EepromData d = {};
        d.adapterModel            = AdapterModel::Mezzanine70;
        d.adapterVersion          = 1;
        d.supportedPadmapIds[0]   = EepromData::PADMAP_UNSET;
        d.supportedPadmapIds[1]   = EepromData::PADMAP_UNSET;
        d.supportedPadmapIds[2]   = EepromData::PADMAP_UNSET;
        d.supportedPadmapIds[3]   = EepromData::PADMAP_UNSET;
        d.designedLifespan        = 10000;
        eepromSerialize(d, buf);
        if (eeprom.write(0x00, buf, sizeof(buf)))
            Serial.println("provision:    OK");
        else
            Serial.println("provision:    FAIL");
    }

    // 4. Write a known pattern then read back to verify round-trip
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
