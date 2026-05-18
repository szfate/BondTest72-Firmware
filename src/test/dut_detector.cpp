#include <Arduino.h>
#include "dut_detector.h"
#include "../debug/log.h"

DutDetector::DutDetector(MuxController& mux, AdcDriver& adc)
    : _mux(mux)
    , _adc(adc)
    , _adapter(nullptr)
    , _padMap(nullptr)
    , _state(DutState::ABSENT)
{
}

void DutDetector::setAdapter(const AdapterBase* adapter) {
    _adapter = adapter;
}

void DutDetector::setPadMap(const PadMap* padMap) {
    _padMap = padMap;
}

bool DutDetector::checkPresence(uint8_t padA, uint8_t padB) {
    if (!_adapter || !_padMap) return false;

    // padA → Bus::D (27K pull-up), padB → Bus::B (GND return).
    // DUT inserted → GND pads shorted through DUT → COM_D pulled low ≈ 0V.
    // DUT absent   → open circuit → COM_D ≈ 3.3V.
    _mux.clearAll();
    _mux.setChannel(_adapter->channelForPin(padA), Bus::D);
    _mux.setChannel(_adapter->channelForPin(padB), Bus::B);
    float v = _adc.readVoltage(0);  // COM_D
    _mux.clearAll();
    LOG_D("dut presence: amez%u→D amez%u→B: %.3fV", padA, padB, v);
    return v < _padMap->presenceThresholdV;
}

DutEvent DutDetector::poll() {
    if (!_padMap) return DutEvent::NONE;

    bool normal  = checkPresence(_padMap->presencePadA, _padMap->presencePadB);
    bool flipped = !normal && checkPresence(71 - _padMap->presencePadA,
                                            71 - _padMap->presencePadB);

    DutState next = normal  ? DutState::PRESENT           :
                    flipped ? DutState::WRONG_ORIENTATION :
                              DutState::ABSENT;

    if (next == _state) return DutEvent::NONE;

    DutEvent ev = DutEvent::NONE;
    if      (next == DutState::PRESENT)            ev = DutEvent::INSERTED;
    else if (next == DutState::WRONG_ORIENTATION)  ev = DutEvent::WRONG_ORIENTATION;
    else if (_state != DutState::ABSENT)           ev = DutEvent::REMOVED;

    _state = next;
    return ev;
}

bool DutDetector::checkNow() {
    if (!_padMap) return false;
    return checkPresence(_padMap->presencePadA, _padMap->presencePadB);
}
