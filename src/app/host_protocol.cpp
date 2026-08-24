#include "host_protocol.h"
#include "build_info.h"
#include "debug/log.h"
#include "hal/kelvin.h"
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
    Serial.print(" aid=");     Serial.print(_uid);
    Serial.print(" ahw=");     Serial.print(hwId);
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
    Serial.print("aid=");     Serial.print(_uid);
    Serial.print(" ahw=");    Serial.print(hwId);
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

void HostProtocol::sendTestStart(uint8_t hwId, const uint8_t* padmapIds, const PadMap* padMap,
                                  uint32_t insertions, uint32_t tests) {
    Serial.print("EVENT TEST_START ");
    Serial.print("aid=");     Serial.print(_uid);
    Serial.print(" ahw=");    Serial.print(hwId);
    // Counts as of this insertion/before this test's run — testCount excludes
    // the test currently starting (incremented only after it completes).
    Serial.print(" ins=");     Serial.print(insertions);
    Serial.print(" tests=");   Serial.print(tests);
    uint8_t pmCount = 0;
    for (uint8_t i = 0; i < 4 && padmapIds[i] != 0xFF; i++) pmCount++;
    if (pmCount > 0) {
        Serial.print(" pm=");
        for (uint8_t i = 0; i < pmCount; i++) {
            if (i > 0) Serial.print(',');
            Serial.print(padmapIds[i]);
        }
    }

    // Order matches PULLUP_LEVELS / rf,rr,vf,vr in PAD lines, low-current-first.
    // Fixed for the life of the firmware build, so sent once here rather than
    // repeated on every SLOT/PAD line.
    Serial.print(" current_list_ua=");
    for (uint8_t i = 0; i < PULLUP_LEVEL_COUNT; i++) {
        if (i > 0) Serial.print(',');
        Serial.print(pullupCurrentUA(PULLUP_LEVELS[i].ohms), 0);
    }

    // thresholds is per-TestCase (pad_map.h) but every case in a padmap
    // currently shares one TestThresholds instance (see kThresh in
    // pad_map_registry.cpp), so a single value here is valid today. Taken
    // from the first case with non-null thresholds; omitted if none.
    if (padMap) {
        const TestThresholds* thresh = nullptr;
        for (uint8_t i = 0; i < padMap->caseCount; i++) {
            if (padMap->cases[i].thresholds != nullptr) {
                thresh = padMap->cases[i].thresholds;
                break;
            }
        }
        if (thresh) {
            Serial.print(" max_bond_r_ohms=");
            Serial.print(thresh->maxBondResistanceOhms, 0);
        }
    }

    // settleUs is per-TestCase (pad_map.h), not a firmware-wide constant like
    // PULLUP_LEVELS, so this is only valid as a single list because every
    // CAP_SENSE case in a padmap currently shares one settleUs. Taken from
    // the first CAP_SENSE case found; omitted if the padmap has none.
    if (padMap) {
        const TestCase* capCase = nullptr;
        for (uint8_t i = 0; i < padMap->caseCount; i++) {
            if (padMap->cases[i].strategy == TestStrategy::CAP_SENSE) {
                capCase = &padMap->cases[i];
                break;
            }
        }
        if (capCase) {
            uint16_t times[CAP_SENSE_SAMPLE_COUNT];
            curveSampleTimesUs(capCase->settleUs, times);
            Serial.print(" cap_time_list_us=");
            for (uint8_t i = 0; i < CAP_SENSE_SAMPLE_COUNT; i++) {
                if (i > 0) Serial.print(',');
                Serial.print(times[i]);
            }
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

// Prints `count` comma-separated values from a contiguous group of readings
// (either the forward or reverse half of PadResult.readings). For STD, count
// is PULLUP_LEVEL_COUNT and each slot is a pullup level, lowest-current-first
// (see PULLUP_LEVELS in hal/kelvin.cpp); for CAP_SENSE, count is
// CAP_SENSE_SAMPLE_COUNT and each slot is a curve timepoint, earliest-first.
static void printReadingGroupCsv(const PadReading* group, uint8_t count, bool resistance) {
    for (uint8_t k = 0; k < count; k++) {
        if (k > 0) Serial.print(',');
        if (resistance) Serial.print(group[k].resistanceOhms, 0);
        else            Serial.print(group[k].voltageV, 3);
    }
}

void HostProtocol::sendPadResult(uint8_t slot, uint8_t adapterPin, uint8_t diePad,
                                 TestStrategy strategy, const PadResult& r) {
    Serial.print("PAD ");
    Serial.print("slot=");   Serial.print(slot);
    Serial.print(" apin=");    Serial.print(adapterPin);
    Serial.print(" dp=");     Serial.print(diePad);
    Serial.print(" method="); Serial.print(strategy == TestStrategy::CAP_SENSE ? "CAP" : "STD");
    Serial.print(" result=");
    Serial.print(r.bond == BondResult::GOOD ? "GOOD" : "OPEN");

    if (strategy == TestStrategy::CAP_SENSE) {
        // Raw voltage samples from the charging curve, forward and reverse
        // under separate keywords — they're two independent discharge-then-
        // charge cycles (see measureKelvinCurve), not one continuous curve,
        // so they don't belong in the same array. No resistance shown: R =
        // Rpu·V/(VCC−V) only means something at steady state — applied to a
        // still-rising mid-charge voltage it explodes nonlinearly as V
        // approaches VCC and doesn't represent anything physical. Only the
        // final (most-settled) sample in each direction is steady-state
        // enough for that transform to matter, and that's already reflected
        // in `result` via the classification.
        const PadReading* fwd = &r.readings[0];
        const PadReading* rev = &r.readings[CAP_SENSE_SAMPLE_COUNT];
        Serial.print(" vfs="); printReadingGroupCsv(fwd, CAP_SENSE_SAMPLE_COUNT, false);
        Serial.print(" vrs="); printReadingGroupCsv(rev, CAP_SENSE_SAMPLE_COUNT, false);
    } else {
        const PadReading* fwd = &r.readings[0];
        const PadReading* rev = &r.readings[PULLUP_LEVEL_COUNT];

        Serial.print(" rf="); printReadingGroupCsv(fwd, PULLUP_LEVEL_COUNT, true);
        Serial.print(" rr="); printReadingGroupCsv(rev, PULLUP_LEVEL_COUNT, true);
        Serial.print(" vf="); printReadingGroupCsv(fwd, PULLUP_LEVEL_COUNT, false);
        Serial.print(" vr="); printReadingGroupCsv(rev, PULLUP_LEVEL_COUNT, false);
    }
    Serial.println();
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