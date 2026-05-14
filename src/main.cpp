#include <Arduino.h>
#include "hal/mux.h"
#include "hal/at21cs01.h"
#include "debug/mux_waveform_test.h"
#include "debug/eeprom_test.h"

MuxController    mux;
AT21CS01Driver   eeprom;

void setup() {
    Serial.begin(115200);
    mux.begin();
    eeprom.begin();
}

void loop() {
    muxWaveformTest();
    eepromTest();
    delay(2000);
}
