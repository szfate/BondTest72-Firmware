#include "adapter_self_test.h"
#include "hal/mux.h"
#include "hal/adc.h"
#include "adapter/adapter_base.h"
#include <Arduino.h>

extern MuxController mux;
extern AdcDriver     adc;

void adapterSelfTest(const AdapterBase* adapter) {
    Serial.println("\n--- adapter self-test ---");
    if (!adapter) { Serial.println("no adapter"); return; }
    bool ok = adapter->selfTest(mux, adc);
    Serial.printf("result: %s\n", ok ? "PASS" : "FAIL");
}
