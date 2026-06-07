#include <Arduino.h>
#include "hal/mux.h"
#include "hal/at21cs01.h"
#include "hal/adc.h"
#include "hal/buttons.h"
#include "hal/sk6812.h"
#include "test/dut_detector.h"
#include "test/test_runner.h"
#include "adapter/eeprom_manager.h"
#include "app/host_protocol.h"
#include "app/state_machine.h"
#include "debug/eeprom_test.h"

MuxController    mux;
AT21CS01Driver   eeprom;
AdcDriver        adc;
Buttons          buttons;
SK6812Controller leds;
HostProtocol     hostProtocol;

EepromManager    eepromMgr(eeprom);
DutDetector      dutDetector(mux, adc);
TestRunner       testRunner(mux, adc, dutDetector);
StateMachine     stateMachine(mux, adc, leds, buttons, eepromMgr, dutDetector, testRunner, hostProtocol);

void setup() {
    Serial.begin(115200);
#ifndef NDEBUG
    delay(1000);
#endif
    pinMode(LED_BUILTIN, OUTPUT);
    leds.begin();
    leds.bootShow();
    adc.begin();
    mux.begin();
    eeprom.begin();
    buttons.begin();
    hostProtocol.begin();
    stateMachine.begin();
}

static void heartbeat() {
    bool ledOn = (millis() % 2000) < 20;
    static bool lastLedOn = false;
    if (ledOn != lastLedOn) {
        digitalWrite(LED_BUILTIN, ledOn);
        lastLedOn = ledOn;
    }
}

void loop() {
    heartbeat();
    stateMachine.update();
}