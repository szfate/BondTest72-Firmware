#pragma once
#include "state.h"
#include "hal/mux.h"
#include "hal/adc.h"
#include "hal/sk6812.h"
#include "hal/buttons.h"
#include "adapter/adapter_base.h"
#include "adapter/eeprom_manager.h"
#include "test/dut_detector.h"
#include "test/test_runner.h"
#include "test/result.h"
#include "host_protocol.h"
#include "led_manager.h"
#include "test/discovery_scanner.h"

constexpr uint32_t ADAPTER_POLL_INTERVAL_MS       = 1500;
constexpr uint32_t ADAPTER_POLL_INTERVAL_FAST_MS  = 100;
constexpr uint32_t ADAPTER_INSERT_SETTLE_MS  = 100;  // wait after adapter presence first detected before the first EEPROM read — half-seated contacts read as garbage/blank
constexpr uint32_t DUT_INSERT_SETTLE_MS      = 1000;  // debounce: ignore poll after insertion until connector is seated

class StateMachine {
public:
    StateMachine(MuxController&    mux,
                 AdcDriver&        adc,
                 SK6812Controller& leds,
                 Buttons&          buttons,
                 EepromManager&    eepromMgr,
                 DutDetector&      dutDetector,
                 TestRunner&       testRunner,
                 HostProtocol&     hostProtocol);

    void begin();   // call once from setup()
    void update();  // call every loop()

private:
    void transition(State next);
    void handleDutEvent(DutEvent ev);
    void handleCommand(HostCommand cmd);
    bool tryInitAdapter();
    bool provisionEeprom(uint8_t hwId, const uint8_t padmapIds[4], uint32_t lifespan, uint32_t mfgDate,
                         uint32_t ins, uint32_t tests, uint32_t eol);
    void selectPadMap();
    bool checkAdapterAlive();  // returns false and transitions to NO_ADAPTER if adapter gone
    void startTest();
    void tryStartTest();
    void sendResults();
    void flushEeprom();

    MuxController&    _mux;
    AdcDriver&        _adc;
    LedManager        _ledManager;
    Buttons&          _buttons;
    EepromManager&     _eepromMgr;
    DutDetector&      _dutDetector;
    TestRunner&       _testRunner;
    HostProtocol&     _hostProtocol;
    DiscoveryScanner  _discoveryScanner;

    State         _state           = State::NO_ADAPTER;
    AdapterBase*  _adapter         = nullptr;
    const PadMap* _padMap          = nullptr;
    const PadMap* _lastResultPadMap = nullptr;  // pad map active when _lastResult was captured; a later SET_PADMAP must not relabel it
    TestResult    _lastResult      = {};
    EepromData    _eepromData;
    char          _adapterUid[17]  = {};
    EepromManager::ReadResult _lastEepromResult = EepromManager::ReadResult::Blank;

    uint32_t      _lastAdapterLivePoll   = 0;  // 1500 ms liveness cadence while an adapter is present
    uint32_t      _lastAdapterInsertPoll = 0;  // 100 ms insertion cadence while in NO_ADAPTER — separate so the cadences can't fight
    uint32_t      _adapterSettleUntil = 0;  // 0 = not detecting; else don't init adapter until this time (insertion settle)
    uint32_t      _lastDutPoll      = 0;
    uint32_t      _dutSettleUntil   = 0;
};
