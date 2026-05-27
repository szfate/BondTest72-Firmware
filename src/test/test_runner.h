#pragma once
#include "pad_map.h"
#include "result.h"
#include "dut_detector.h"
#include "hal/mux.h"
#include "hal/adc.h"
#include "adapter/adapter_base.h"

class TestRunner {
public:
    TestRunner(MuxController& mux, AdcDriver& adc, DutDetector& dutDetector);

    TestResult run(AdapterBase& adapter, const PadMap& padMap);

private:
    static PadResult classify(const AdcReadings& r, const TestCase& tc);

    MuxController& _mux;
    AdcDriver&     _adc;
    DutDetector&   _dutDetector;
};
