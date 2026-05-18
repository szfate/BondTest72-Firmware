#include "host_protocol.h"
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
                HostCommand cmd = processLine(_lineBuf);
                if (cmd != HostCommand::NONE) return cmd;
            }
        } else if (_lineLen < sizeof(_lineBuf) - 1) {
            _lineBuf[_lineLen++] = c;
        }
    }
    return HostCommand::NONE;
}

HostCommand HostProtocol::processLine(const char* line) {
    if (strcmp(line, "RUN") == 0)         return HostCommand::RUN;
    if (strcmp(line, "GET_RESULTS") == 0) return HostCommand::GET_RESULTS;
    if (strcmp(line, "DISCOVER") == 0)    return HostCommand::DISCOVER;
    if (strncmp(line, "SET_PADMAP ", 11) == 0) {
        _setPadmapId = (uint8_t)atoi(line + 11);
        return HostCommand::SET_PADMAP;
    }
    return HostCommand::NONE;
}

// ——————————————————————————————————————————————————————————————————————————

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
    Serial.print(' '); Serial.print(r.leftShort  ? 1 : 0);
    Serial.print(' '); Serial.println(r.rightShort ? 1 : 0);
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
