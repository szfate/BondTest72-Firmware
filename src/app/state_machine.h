#pragma once
#include <stdint.h>
#include "../hal/mux.h"
#include "../hal/adc.h"
#include "../hal/sk6812.h"
#include "../hal/buttons.h"
#include "../hal/at21cs01.h"
#include "../adapter/adapter_base.h"
#include "../adapter/eeprom_layout.h"
#include "../test/dut_detector.h"
#include "../test/test_runner.h"
#include "../test/result.h"
#include "host_protocol.h"

constexpr uint32_t ADAPTER_POLL_INTERVAL_MS  = 2000;
constexpr uint32_t DUT_INSERT_SETTLE_MS      = 1000;  // debounce: ignore poll after insertion until connector is seated

class StateMachine {
public:
    StateMachine(MuxController&    mux,
                 AdcDriver&        adc,
                 SK6812Controller& leds,
                 Buttons&          buttons,
                 AT21CS01Driver&   eeprom,
                 DutDetector&      dutDetector,
                 TestRunner&       testRunner,
                 HostProtocol&     hostProtocol);

    void begin();   // call once from setup()
    void update();  // call every loop()

    enum class State : uint8_t {
        NO_ADAPTER,
        ADAPTER_DETECTED,
        READY,
        WRONG_ORIENTATION,
        TESTING,
        PASS,
        FAIL,
        FAULT,
    };

private:
    void transition(State next);
    void handleDutEvent(DutEvent ev);
    void handleCommand(HostCommand cmd);
    void updateLeds();
    bool blinkOn(uint32_t periodMs);
    bool tryInitAdapter();
    bool provisionEeprom(uint8_t padmapId = EepromData::PADMAP_UNSET);
    void selectPadMap();
    bool checkAdapterAlive();  // returns false and transitions to NO_ADAPTER if adapter gone
    void startTest();
    void sendResults();
    void flushEeprom();

    MuxController&    _mux;
    AdcDriver&        _adc;
    SK6812Controller& _leds;
    Buttons&          _buttons;
    AT21CS01Driver&   _eeprom;
    DutDetector&      _dutDetector;
    TestRunner&       _testRunner;
    HostProtocol&     _hostProtocol;

    State         _state           = State::NO_ADAPTER;
    AdapterBase*  _adapter         = nullptr;
    const PadMap* _padMap          = nullptr;
    TestResult    _lastResult      = {};
    EepromData    _eepromData;

    uint32_t      _lastAdapterPoll  = 0;
    uint32_t      _lastDutPoll      = 0;
    uint32_t      _dutSettleUntil   = 0;
};
