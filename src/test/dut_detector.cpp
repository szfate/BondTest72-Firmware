#include "dut_detector.h"

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

    // Each pad is a known-GND mez pin. Connect to Bus::D (27K pull-up to 3.3V):
    // DUT inserted → pad shorted to GND → COM_D ≈ 0V.
    // DUT absent   → floating           → COM_D ≈ 3.3V.
    // Both pads must be low to confirm presence.
    _mux.clearAll();
    _mux.setChannel(_adapter->channelForPin(padA), Bus::D);
    float vA = _adc.readVoltage(0);  // COM_D

    _mux.clearAll();
    _mux.setChannel(_adapter->channelForPin(padB), Bus::D);
    float vB = _adc.readVoltage(0);  // COM_D

    _mux.clearAll();
    return vA < _padMap->presenceThresholdV && vB < _padMap->presenceThresholdV;
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
