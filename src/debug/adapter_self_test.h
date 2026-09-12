#pragma once

class AdapterBase;
class MuxController;
class AdcDriver;

void adapterSelfTest(const AdapterBase* adapter, MuxController& mux, AdcDriver& adc);
