#include <Arduino.h>
#include "hal/mux.h"
#include "hal/at21cs01.h"
#include "hal/adc.h"
#include "hal/buttons.h"
#include "hal/sk6812.h"
#include "debug/mux_waveform_test.h"
#include "debug/eeprom_test.h"
#include "debug/adc_test.h"
#include "debug/button_test.h"
#include "debug/sk6812_test.h"

MuxController    mux;
AT21CS01Driver   eeprom;
AdcDriver        adc;
Buttons          buttons;
SK6812Controller leds;

void setup() {
    Serial.begin(115200);
    mux.begin();
    eeprom.begin();
    adc.begin();
    buttons.begin();
    leds.begin();
}

void loop() {
    muxWaveformTest();
    eepromTest();
    adcTest();
    buttonTest();
    sk6812Test();
    delay(2000);
}
