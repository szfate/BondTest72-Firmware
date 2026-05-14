#include "mux_waveform_test.h"
#include "hal/mux.h"
#include <Arduino.h>

extern MuxController mux;

// Exercises representative switch operations at logic-analyzer-friendly pace.
// Expected addresses on DAT (7-bit, MSB-first):
//   setChannel( 0, A): chip U2, X0,  Y0 → 0x00
//   setChannel( 1, D): chip U2, X16, Y3 → 0x70
//   setChannel(15, B): chip U3, X0,  Y1 → 0x20
//   setChannel(30, C): chip U4, X0,  Y2 → 0x40
//   setChannel( 0, E): chip U2, X0,  Y4 → 0x18  (Y4 group 0)
//   setChannel(54, E): chip U3, X7,  Y4 → 0x39  (Y4 group 1)
//   setChannel(40, E): chip U4, X12, Y4 → 0x58  (Y4 group 2)
//   setChannel(38, E): chip U4, X18, Y4 → 0x78  (Y4 group 3)
void muxWaveformTest() {
    const auto set = [](uint8_t pad, Bus bus) {
        mux.setChannel(pad, bus);
        delay(1);
        mux.clearAll();
        delay(1);
    };

    set(0,  Bus::A);   // U2 X0  Y0 → 0x00
    set(1,  Bus::D);   // U2 X16 Y3 → 0x70
    set(15, Bus::B);   // U3 X0  Y1 → 0x20
    set(30, Bus::C);   // U4 X0  Y2 → 0x40
    set(0,  Bus::E);   // U2 X0  Y4 → 0x18
    set(54, Bus::E);   // U3 X7  Y4 → 0x39
    set(40, Bus::E);   // U4 X12 Y4 → 0x58
    set(38, Bus::E);   // U4 X18 Y4 → 0x78

    // Multi-channel: verify latches hold while setting three chips in sequence
    mux.setChannel(0,  Bus::D);
    mux.setChannel(15, Bus::A);
    mux.setChannel(30, Bus::C);
    delay(1);
    mux.clearAll();
    delay(5);
}
