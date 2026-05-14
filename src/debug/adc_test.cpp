#include "adc_test.h"
#include "hal/adc.h"
#include <Arduino.h>

extern AdcDriver adc;

void adcTest() {
    AdcReadings r = adc.readAll();
    Serial.printf("sense: %.3f V  left: %.3f V  right: %.3f V\n",
                  r.sense, r.leftNeighbour, r.rightNeighbour);
}
