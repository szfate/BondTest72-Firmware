#include <Arduino.h>
#include "hal/mux.h"
#include "debug/mux_waveform_test.h"

MuxController mux;

void setup() {
    Serial.begin(115200);
    mux.begin();
}

void loop() {
    muxWaveformTest();
    delay(50);
}
