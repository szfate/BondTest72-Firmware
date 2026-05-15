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

bool DutDetector::checkPresence(uint8_t pinA, uint8_t pinB) {
    if (!_adapter) return false;
    _mux.clearAll();
    _mux.setChannel(_adapter->channelForPin(pinA), Bus::A);
    _mux.setChannel(_adapter->channelForPin(pinB), Bus::D);
    float v = _adc.readVoltage(1);  // COM_A
    _mux.clearAll();
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
