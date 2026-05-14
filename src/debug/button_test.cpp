#include "button_test.h"
#include "hal/buttons.h"
#include <Arduino.h>

extern Buttons buttons;

// Call every loop — prints once per press.
void buttonTest() {
    buttons.poll();
    if (buttons.startPressed())
        Serial.println("START pressed");
}
