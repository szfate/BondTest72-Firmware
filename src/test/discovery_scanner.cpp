#include "discovery_scanner.h"
#include "hal/mux.h"
#include "hal/adc.h"
#include "app/host_protocol.h"
#include "debug/log.h"
#include <Arduino.h>

DiscoveryScanner::DiscoveryScanner(MuxController& mux, AdcDriver& adc, HostProtocol& host)
    : _mux(mux)
    , _adc(adc)
    , _host(host)
{
}

void DiscoveryScanner::run() {
    LOG_I("=== discovery scan start ===");
    // Scans all mez pins 
    for (uint8_t src = 1; src <= 70; src++) {
        for (uint8_t snk = 1; snk <= 70; snk++) {
            if (src == snk) continue;
            _mux.clearAll();
            _mux.setChannel(src - 1, Bus::D);
            _mux.setChannel(snk - 1, Bus::B);
            delay(1);
            float v = _adc.readVoltage(0);
            _host.sendDiscoveryScanPoint(src, snk, v);
        }
    }
    _mux.clearAll();
    _host.sendDiscoveryScanDone();
    LOG_I("=== discovery scan done ===");
}