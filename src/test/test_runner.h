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

    // Writes the result into `out` (caller-owned storage) instead of returning by
    // value: TestResult is ~9 KB, too large for the stack, and a by-value return
    // would also copy the whole struct on the way out.
    void run(AdapterBase& adapter, const PadMap& padMap, TestResult& out);

private:
    PadResult sweepPad(AdapterBase& adapter, const TestCase& tc, MeasureDirections dirs);

    MuxController& _mux;
    AdcDriver&     _adc;
    DutDetector&   _dutDetector;
};
