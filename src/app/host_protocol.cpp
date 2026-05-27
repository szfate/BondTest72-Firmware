#include "host_protocol.h"
#include "debug/log.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

void HostProtocol::begin() {
    _lineLen = 0;
}

HostCommand HostProtocol::poll() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') {
            if (_lineLen > 0) {
                _lineBuf[_lineLen] = '\0';
                _lineLen = 0;
                _overflowWarned = false;
                HostCommand cmd = processLine(_lineBuf);
                if (cmd != HostCommand::NONE) return cmd;
            }
        } else if (_lineLen < sizeof(_lineBuf) - 1) {
            _lineBuf[_lineLen++] = c;
            if (_lineLen >= 60 && !_overflowWarned) {
                _overflowWarned = true;
                LOG_W("host: command line approaching buffer limit");
            }
        }
    }
    return HostCommand::NONE;
}

HostCommand HostProtocol::processLine(const char* line) {
    if (strcmp(line, "RUN") == 0)         return HostCommand::RUN;
    if (strcmp(line, "GET_RESULTS") == 0) return HostCommand::GET_RESULTS;
    if (strcmp(line, "GET_ADAPTER") == 0)       return HostCommand::GET_ADAPTER;
    if (strcmp(line, "DISCOVER") == 0)          return HostCommand::DISCOVER;
    if (strcmp(line, "DISCOVERY_SCAN") == 0)   return HostCommand::DISCOVERY_SCAN;
    if (strncmp(line, "PROVISION", 9) == 0) {
        _provisionMfgDate = 0;
        if (line[9] == ' ') {
            _provisionPadmapId = (uint8_t)atoi(line + 10);
            const char* p = line + 10;
            while (*p && *p != ' ') p++;
            if (*p == ' ') _provisionMfgDate = (uint32_t)atol(p + 1);  // YYYYMMDD
        } else {
            _provisionPadmapId = 0xFF;
        }
        return HostCommand::PROVISION;
    }
    if (strncmp(line, "SET_PADMAP ", 11) == 0) {
        _setPadmapId = (uint8_t)atoi(line + 11);
        return HostCommand::SET_PADMAP;
    }
    return HostCommand::NONE;
}

// ——————————————————————————————————————————————————————————————————————————

void HostProtocol::sendAdapterInfo(uint8_t model, uint8_t version, const uint8_t* padmapIds,
                                    uint32_t lifespan, uint32_t dateOfManufacture,
                                    uint32_t insertions, uint32_t tests, bool eol,
                                    const char* padMapName) {
    Serial.print("ADAPTER");
    Serial.print(" model="); Serial.print(model);
    Serial.print(" ver="); Serial.print(version);
    for (uint8_t i = 0; i < 4 && padmapIds[i] != 0xFF; i++) {
        Serial.print(" padmap"); Serial.print(i); Serial.print('='); Serial.print(padmapIds[i]);
    }
    Serial.print(" lifespan="); Serial.print(lifespan);
    Serial.print(" mfg_date="); Serial.print(dateOfManufacture);
    Serial.print(" ins=");      Serial.print(insertions);
    Serial.print(" tests=");    Serial.print(tests);
    Serial.print(" eol=");      Serial.print(eol ? 1 : 0);
    Serial.print(" padmap=");   Serial.print(padMapName ? padMapName : "none");
    Serial.println();
}

void HostProtocol::sendAdapterDetected(uint8_t model, uint8_t version, const uint8_t* padmapIds) {
    Serial.print("EVENT ADAPTER_DETECTED ");
    Serial.print(model);   Serial.print(' ');
    Serial.print(version);
    for (uint8_t i = 0; i < 4 && padmapIds[i] != 0xFF; i++) {
        Serial.print(' '); Serial.print(padmapIds[i]);
    }
    Serial.println();
}

void HostProtocol::sendDutInserted() {
    Serial.println("EVENT DUT_INSERTED");
}

void HostProtocol::sendDutRemoved() {
    Serial.println("EVENT DUT_REMOVED");
}

void HostProtocol::sendTestStart(uint8_t model, uint8_t version, const uint8_t* padmapIds) {
    Serial.print("EVENT TEST_START ");
    Serial.print(model);   Serial.print(' ');
    Serial.print(version);
    for (uint8_t i = 0; i < 4 && padmapIds[i] != 0xFF; i++) {
        Serial.print(' '); Serial.print(padmapIds[i]);
    }
    Serial.println();
}

void HostProtocol::sendPadResult(uint8_t slot, uint8_t mezPin, const PadResult& r) {
    Serial.print("PAD ");
    Serial.print(slot);    Serial.print(' ');
    Serial.print(mezPin);  Serial.print(' ');
    switch (r.bond) {
        case BondResult::GOOD:      Serial.print("GOOD");  break;
        case BondResult::OPEN:      Serial.print("OPEN");  break;
        case BondResult::SHORT_GND: Serial.print("SHORT"); break;
    }
    Serial.print(' '); Serial.print(r.prevShort ? 1 : 0);
    Serial.print(' '); Serial.println(r.nextShort ? 1 : 0);
}

void HostProtocol::sendSlotStatus(uint8_t slot, bool present, bool tested) {
    Serial.print("SLOT ");
    Serial.print(slot); Serial.print(' ');
    Serial.print(present ? 1 : 0); Serial.print(' ');
    Serial.println(tested ? 1 : 0);
}

void HostProtocol::sendSummary(const TestResult& result) {
    uint8_t totalGood = 0, totalTested = 0;
    for (uint8_t s = 0; s < result.slotCount; s++) {
        totalGood   += result.slots[s].goodCount;
        totalTested += result.slots[s].testedCount;
    }
    Serial.print("SUMMARY ");
    Serial.print(result.outcome == TestOutcome::PASS ? "PASS" : "FAIL");
    Serial.print(' ');
    Serial.print(totalGood); Serial.print('/'); Serial.print(totalTested);
    if (result.outcome == TestOutcome::FAIL_DUT_REMOVED)
        Serial.print(" FAIL_DUT_REMOVED");
    Serial.println();
}

void HostProtocol::sendError(const char* description) {
    Serial.print("ERROR "); Serial.println(description);
}

void HostProtocol::sendDiscoveryScanPoint(uint8_t src, uint8_t snk, float v) {
    Serial.print("DSCAN "); Serial.print(src); Serial.print(' ');
    Serial.print(snk);     Serial.print(' '); Serial.println(v, 3);
}

void HostProtocol::sendDiscoveryScanDone() {
    Serial.println("DSCAN DONE");
}

