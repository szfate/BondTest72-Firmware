#include "host_protocol.h"
#include "build_info.h"
#include "debug/log.h"
#include <Arduino.h>
#include <pico/unique_id.h>
#include <string.h>
#include <stdlib.h>

void HostProtocol::begin() {
    _lineLen = 0;
    memset(_uid, 0, sizeof(_uid));
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
            if (_lineLen >= 250 && !_overflowWarned) {
                _overflowWarned = true;
                LOG_W("host: command line approaching buffer limit");
            }
        }
    }
    return HostCommand::NONE;
}

HostCommand HostProtocol::processLine(const char* line) {
    if (strcmp(line, "RUN") == 0)              return HostCommand::RUN;
    if (strcmp(line, "GET_RESULTS") == 0)      return HostCommand::GET_RESULTS;
    if (strcmp(line, "GET_ADAPTER") == 0)       return HostCommand::GET_ADAPTER;
    if (strcmp(line, "DISCOVERY_SCAN") == 0)    return HostCommand::DISCOVERY_SCAN;
    if (strcmp(line, "HELLO") == 0)             return HostCommand::HELLO;

    if (strncmp(line, "SET_PADMAP ", 11) == 0) {
        uint32_t id;
        if (parseKvUint(line + 11, "id", id)) {
            _setPadmapId = (uint8_t)id;
            return HostCommand::SET_PADMAP;
        }
        return HostCommand::NONE;
    }

    if (strncmp(line, "PROVISION ", 10) == 0) {
        _provisionMfgDate  = 0xFFFFFFFF;
        _provisionIns    = 0xFFFFFFFF;
        _provisionTests  = 0xFFFFFFFF;
        _provisionEol    = 0xFFFFFFFF;
        _provisionHwId     = 0xFF;
        _provisionLifespan = 0xFFFFFFFF;
        for (uint8_t i = 0; i < 4; i++) _provisionPadmapIds[i] = 0xFF;
        uint32_t hw;
        bool hasHw = parseKvUint(line + 10, "hw", hw);
        if (hasHw) _provisionHwId = (uint8_t)hw;
        uint32_t ls;
        if (parseKvUint(line + 10, "lifespan", ls)) _provisionLifespan = ls;
        if (parseKvUintList(line + 10, "padmap", _provisionPadmapIds, 4)) {
            uint32_t dt;
            if (parseKvUint(line + 10, "date", dt)) {
                _provisionMfgDate = dt;
            }
            uint32_t ins;
            if (parseKvUint(line + 10, "ins", ins)) _provisionIns = ins;
            uint32_t tests;
            if (parseKvUint(line + 10, "tests", tests)) _provisionTests = tests;
            uint32_t eol;
            if (parseKvUint(line + 10, "eol", eol)) _provisionEol = eol;
            return HostCommand::PROVISION;
        }
        return HostCommand::NONE;
    }

    return HostCommand::NONE;
}

bool HostProtocol::parseKvUint(const char* kv, const char* key, uint32_t& out) {
    uint8_t klen = strlen(key);
    const char* p = kv;
    while (*p) {
        if (strncmp(p, key, klen) == 0 && p[klen] == '=') {
            out = (uint32_t)atol(p + klen + 1);
            return true;
        }
        while (*p && *p != ' ') p++;
        if (*p == ' ') p++;
    }
    return false;
}

bool HostProtocol::parseKvUintList(const char* kv, const char* key, uint8_t* out, uint8_t maxCount) {
    uint8_t klen = strlen(key);
    const char* p = kv;
    while (*p) {
        if (strncmp(p, key, klen) == 0 && p[klen] == '=') {
            const char* val = p + klen + 1;
            uint8_t count = 0;
            while (count < maxCount) {
                out[count] = (uint8_t)atol(val);
                count++;
                while (*val && *val != ',' && *val != ' ') val++;
                if (*val == ',') { val++; continue; }
                break;
            }
            while (count < maxCount) out[count++] = 0xFF;
            return true;
        }
        while (*p && *p != ' ') p++;
        if (*p == ' ') p++;
    }
    return false;
}

void HostProtocol::setUid(const char* uid16) {
    if (uid16) {
        strncpy(_uid, uid16, sizeof(_uid) - 1);
        _uid[sizeof(_uid) - 1] = '\0';
    } else {
        memset(_uid, '0', sizeof(_uid) - 1);
        _uid[sizeof(_uid) - 1] = '\0';
    }
}

// ——————————————————————————————————————————————————————————————————————————

void HostProtocol::sendAdapterInfo(uint8_t hwId, const uint8_t* padmapIds,
                                    uint32_t lifespan, uint32_t dateOfManufacture,
                                    uint32_t insertions, uint32_t tests, bool eol,
                                    bool dutPresent) {
    Serial.print("ADAPTER");
    Serial.print(" uid=");     Serial.print(_uid);
    Serial.print(" hw=");      Serial.print(hwId);
    uint8_t pmCount = 0;
    for (uint8_t i = 0; i < 4 && padmapIds[i] != 0xFF; i++) pmCount++;
    if (pmCount > 0) {
        Serial.print(" pm=");
        for (uint8_t i = 0; i < pmCount; i++) {
            if (i > 0) Serial.print(',');
            Serial.print(padmapIds[i]);
        }
    }
    Serial.print(" lifespan=");   Serial.print(lifespan);
    Serial.print(" mfg_date=");   Serial.print(dateOfManufacture);
    Serial.print(" ins=");        Serial.print(insertions);
    Serial.print(" tests=");      Serial.print(tests);
    Serial.print(" eol=");        Serial.print(eol ? 1 : 0);
    Serial.print(" dut=");        Serial.print(dutPresent ? 1 : 0);
    Serial.println();
}

void HostProtocol::sendAdapterDetected(uint8_t hwId, const uint8_t* padmapIds) {
    Serial.print("EVENT ADAPTER_DETECTED ");
    Serial.print("uid=");     Serial.print(_uid);
    Serial.print(" hw=");      Serial.print(hwId);
    uint8_t pmCount = 0;
    for (uint8_t i = 0; i < 4 && padmapIds[i] != 0xFF; i++) pmCount++;
    if (pmCount > 0) {
        Serial.print(" pm=");
        for (uint8_t i = 0; i < pmCount; i++) {
            if (i > 0) Serial.print(',');
            Serial.print(padmapIds[i]);
        }
    }
    Serial.println();
}

void HostProtocol::sendAdapterRemoved() {
    Serial.println("EVENT ADAPTER_REMOVED");
}

void HostProtocol::sendDutInserted() {
    Serial.println("EVENT DUT_INSERTED");
}

void HostProtocol::sendDutRemoved() {
    Serial.println("EVENT DUT_REMOVED");
}

void HostProtocol::sendTestStart(uint8_t hwId, const uint8_t* padmapIds) {
    Serial.print("EVENT TEST_START ");
    Serial.print("uid=");     Serial.print(_uid);
    Serial.print(" hw=");      Serial.print(hwId);
    uint8_t pmCount = 0;
    for (uint8_t i = 0; i < 4 && padmapIds[i] != 0xFF; i++) pmCount++;
    if (pmCount > 0) {
        Serial.print(" pm=");
        for (uint8_t i = 0; i < pmCount; i++) {
            if (i > 0) Serial.print(',');
            Serial.print(padmapIds[i]);
        }
    }
    Serial.println();
}

void HostProtocol::sendEolWarning(uint32_t insertionCount) {
    Serial.print("EVENT EOL_WARNING ins=");
    Serial.println(insertionCount);
}

void HostProtocol::sendWrongOrientation() {
    Serial.println("EVENT WRONG_ORIENTATION");
}

void HostProtocol::sendPadResult(uint8_t slot, uint8_t adapterPin, uint8_t diePad, const PadResult& r) {
    Serial.print("PAD ");
    Serial.print("slot=");   Serial.print(slot);
    Serial.print(" apin=");    Serial.print(adapterPin);
    Serial.print(" dp=");     Serial.print(diePad);
    Serial.print(" result=");
    switch (r.bond) {
        case BondResult::GOOD:      Serial.print("GOOD");      break;
        case BondResult::OPEN:      Serial.print("OPEN");      break;
        case BondResult::SHORT_GND: Serial.print("SHORT");      break;
    }
    Serial.print(" ps=");     Serial.print(r.prevShort ? 1 : 0);
    Serial.print(" ns=");     Serial.print(r.nextShort ? 1 : 0);
    Serial.print(" sv=");     Serial.print(r.senseV, 3);
    Serial.print(" pv=");     Serial.print(r.prevV,  3);
    Serial.print(" nv=");     Serial.println(r.nextV, 3);
}

void HostProtocol::sendSlotStatus(uint8_t slot, bool present, bool tested) {
    Serial.print("SLOT ");
    Serial.print("slot=");       Serial.print(slot);
    Serial.print(" present=");   Serial.print(present ? 1 : 0);
    Serial.print(" tested=");     Serial.println(tested ? 1 : 0);
}

void HostProtocol::sendSummary(const TestResult& result) {
    uint8_t totalGood = 0, totalTested = 0;
    for (uint8_t s = 0; s < result.slotCount; s++) {
        totalGood   += result.slots[s].goodCount;
        totalTested += result.slots[s].testedCount;
    }
    Serial.print("SUMMARY ");
    Serial.print("outcome=");
    Serial.print(result.outcome == TestOutcome::PASS ? "PASS" : "FAIL");
    Serial.print(" good=");      Serial.print(totalGood);
    Serial.print(" tested=");     Serial.print(totalTested);
    if (result.outcome == TestOutcome::FAIL_DUT_REMOVED)
        Serial.print(" fail_reason=DUT_REMOVED");
    Serial.println();
}

void HostProtocol::sendError(ErrorCode code, const char* msg) {
    Serial.print("ERROR code=");
    Serial.print(static_cast<uint8_t>(code));
    Serial.print(" msg=");
    Serial.println(msg);
}

void HostProtocol::sendFault(const char* msg) {
    Serial.print("EVENT FAULT msg=");
    Serial.println(msg);
}

void HostProtocol::sendDiscoveryScanPoint(uint8_t src, uint8_t snk, float v) {
    Serial.print("DSCAN ");
    Serial.print("src="); Serial.print(src);
    Serial.print(" snk="); Serial.print(snk);
    Serial.print(" sv=");  Serial.println(v, 3);
}

void HostProtocol::sendDiscoveryScanDone() {
    Serial.println("DSCAN DONE");
}

void HostProtocol::sendHello() {
    Serial.print("HELLO name=");
    Serial.print(FW_NAME);
    Serial.print(" build=");
    Serial.print(FW_BUILD_ID);
    Serial.print(" uid=");
    char uidhex[17];
    pico_get_unique_board_id_string(uidhex, sizeof(uidhex));
    Serial.print(uidhex);
    Serial.println();
}