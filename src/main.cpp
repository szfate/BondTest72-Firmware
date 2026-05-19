#include <Arduino.h>
#include "hal/mux.h"
#include "hal/at21cs01.h"
#include "hal/adc.h"
#include "hal/buttons.h"
#include "hal/sk6812.h"
#include "test/dut_detector.h"
#include "test/test_runner.h"
#include "test/pad_map_registry.h"
#include "app/host_protocol.h"
#include "app/state_machine.h"
#include "debug/eeprom_test.h"
#include "debug/log.h"

MuxController    mux;
AT21CS01Driver   eeprom;
AdcDriver        adc;
Buttons          buttons;
SK6812Controller leds;
HostProtocol     hostProtocol;

DutDetector  dutDetector(mux, adc);
TestRunner   testRunner(mux, adc, dutDetector);
StateMachine stateMachine(mux, adc, leds, buttons, eeprom, dutDetector, testRunner, hostProtocol);

// Debug: sweep all test cases with DUT, measure fwd (mezPin→D, gndPin→B)
// and rev (gndPin→D, mezPin→B) bias, then hang.
static void debugBiasedSweep(uint8_t padMapId) {
    const PadMap* pm = PadMapRegistry::find(padMapId);
    if (!pm) { LOG_E("padmap %u not found", padMapId); return; }
    LOG_I("=== biased sweep: %s (%u cases) ===", pm->name, pm->caseCount);
    for (uint8_t i = 0; i < pm->caseCount; i++) {
        const TestCase& tc = pm->cases[i];
        uint8_t mezCh = tc.mezPin - 1;
        uint8_t gndCh = tc.gndPin - 1;

        mux.clearAll();
        mux.setChannel(mezCh, Bus::D);
        mux.setChannel(gndCh, Bus::B);
        delay(1);
        float fwd = adc.readVoltage(0);  // mezPin on pull-up rail

        mux.clearAll();
        mux.setChannel(gndCh, Bus::D);
        mux.setChannel(mezCh, Bus::B);
        delay(1);
        float rev = adc.readVoltage(0);  // gndPin on pull-up rail

        mux.clearAll();
        LOG_I("mez%02u gnd%02u  fwd=%.3fV  rev=%.3fV  delta=%.3fV",
              tc.mezPin, tc.gndPin, fwd, rev, fwd - rev);
    }
    LOG_I("=== sweep done, halting ===");
    while (true) delay(1000);
}

void setup() {
    Serial.begin(115200);
#ifndef NDEBUG
    delay(000);
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
    bool ledOn = (millis() % 2000) < 1;
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
