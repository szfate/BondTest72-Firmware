#pragma once
#include <stdint.h>

class MuxController;
class AdcDriver;
class HostProtocol;

class DiscoveryScanner {
public:
    DiscoveryScanner(MuxController& mux, AdcDriver& adc, HostProtocol& host);
    void run();  // blocking 70×70 sweep, streams results to host

private:
    MuxController& _mux;
    AdcDriver&     _adc;
    HostProtocol&  _host;
};