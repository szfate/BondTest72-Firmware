#include <Arduino.h>
#include "dut_detector.h"
#include "debug/log.h"

DutDetector::DutDetector(MuxController& mux, AdcDriver& adc)
    : _mux(mux)
    , _adc(adc)
    , _adapter(nullptr)
    , _padMap(nullptr)
    , _state(DutState::ABSENT)
    , _pendingState(DutState::ABSENT)
    , _confirmCount(0)
{
}

void DutDetector::setAdapter(const AdapterBase* adapter) {
    _adapter = adapter;
    _confirmCount = 0;
}

void DutDetector::setPadMap(const PadMap* padMap) {
    _padMap = padMap;
    _confirmCount = 0;
}

// Shared sensing block of prime() and poll(): sweep presence, then flipped
// orientation only if not present; classify to a DutState.
DutDetector::DutState DutDetector::senseCandidate() {
    bool present = _adapter->senseDutPresent(_mux, _adc, *_padMap);
    bool flipped = !present && _padMap->checkOrientation
                        && _adapter->senseDutFlipped(_mux, _adc, *_padMap);
    return present ? DutState::PRESENT           :
           flipped ? DutState::WRONG_ORIENTATION :
                     DutState::ABSENT;
}

void DutDetector::prime() {
    if (!_adapter || !_padMap) return;
    _state = senseCandidate();
    _pendingState = _state;
    _confirmCount = DUT_CONFIRM_COUNT;
}

bool DutDetector::dutPresent() const {
    return _state == DutState::PRESENT || _state == DutState::WRONG_ORIENTATION;
}

DutEvent DutDetector::poll() {
    if (!_adapter || !_padMap) return DutEvent::NONE;

    DutState candidate = senseCandidate();

    if (candidate == _state) {
        _pendingState = candidate;
        _confirmCount = DUT_CONFIRM_COUNT;
        return DutEvent::NONE;
    }

    if (candidate == _pendingState) {
        _confirmCount++;
    } else {
        _pendingState = candidate;
        _confirmCount = 1;
    }

    if (_confirmCount < DUT_CONFIRM_COUNT) return DutEvent::NONE;

    DutEvent ev = DutEvent::NONE;
    if      (candidate == DutState::PRESENT)            ev = DutEvent::INSERTED;
    else if (candidate == DutState::WRONG_ORIENTATION)  ev = DutEvent::WRONG_ORIENTATION;
    else if (_state != DutState::ABSENT)                ev = DutEvent::REMOVED;

    _state = candidate;
    _pendingState = candidate;
    _confirmCount = DUT_CONFIRM_COUNT;
    return ev;
}

bool DutDetector::checkNow() {
    if (!_adapter || !_padMap) return false;
    return _adapter->checkDutNow(_mux, _adc, *_padMap);
}