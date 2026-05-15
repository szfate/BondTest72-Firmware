#pragma once
#include <stdint.h>
#include "pad_map.h"
#include "../hal/mux.h"
#include "../hal/adc.h"
#include "../adapter/adapter_base.h"

constexpr uint32_t DUT_POLL_INTERVAL_MS = 250;

enum class DutEvent : uint8_t {
    NONE,
    INSERTED,
    REMOVED,
    WRONG_ORIENTATION,
};

class DutDetector {
public:
    DutDetector(MuxController& mux, AdcDriver& adc);

    void     setAdapter(const AdapterBase* adapter);
    void     setPadMap(const PadMap* padMap);
    DutEvent poll();

    // Presence check without state update — call from TestRunner at end of scan.
    bool checkNow();

private:
    bool checkPresence(uint8_t pinA, uint8_t pinB);

    MuxController&      _mux;
    AdcDriver&          _adc;
    const AdapterBase*  _adapter;
    const PadMap*       _padMap;

    enum class DutState : uint8_t { ABSENT, PRESENT, WRONG_ORIENTATION };
    DutState _state = DutState::ABSENT;
};
