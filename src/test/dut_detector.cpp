#include <Arduino.h>
#include "dut_detector.h"
#include "debug/log.h"

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

DutEvent DutDetector::poll() {
    if (!_adapter || !_padMap) return DutEvent::NONE;

    bool present = _adapter->senseDutPresent(_mux, _adc, *_padMap);
    bool flipped = !present && _adapter->senseDutFlipped(_mux, _adc, *_padMap);

    DutState next = present ? DutState::PRESENT           :
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
    if (!_adapter || !_padMap) return false;
    return _adapter->checkDutNow(_mux, _adc, *_padMap);
}
