#include "host_protocol.h"
#include "build_info.h"
#include "debug/log.h"
#include "hal/kelvin.h"
#include <Arduino.h>
#include <pico/unique_id.h>
#include <string.h>

void HostProtocol::begin() {
    _lineLen = 0;
    memset(_adapterUid, 0, sizeof(_adapterUid));
}

HostCommand HostProtocol::poll() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') {
            if (_lineOverflow) {
                // Never run what's left of an over-long line: a PROVISION cut
                // short would lose its trailing counters, which default to 0.
                _lineOverflow = false;
                _lineLen = 0;
                sendError(ErrorCode::LINE_TOO_LONG, "LINE_TOO_LONG");
            } else if (_lineLen > 0) {
                _lineBuf[_lineLen] = '\0';
                _lineLen = 0;
                HostCommand cmd = processLine(_lineBuf);
                if (cmd != HostCommand::NONE) return cmd;
            }
        } else if (_lineLen < sizeof(_lineBuf) - 1) {
            _lineBuf[_lineLen++] = c;
        } else {
            _lineOverflow = true;  // rest of the line is dropped; reported at its end
        }
    }
    return HostCommand::NONE;
}

HostCommand HostProtocol::processLine(const char* line) {
    if (strcmp(line, "RUN") == 0)              return HostCommand::RUN;
    if (strcmp(line, "GET_RESULTS") == 0)      return HostCommand::GET_RESULTS;
    if (strcmp(line, "GET_ADAPTER") == 0)       return HostCommand::GET_ADAPTER;
    if (strcmp(line, "HELLO") == 0)             return HostCommand::HELLO;
    if (strcmp(line, "LIST_PADMAPS") == 0)      return HostCommand::LIST_PADMAPS;

    // SET_PADMAP and SET_DEFAULT_PADMAP share one id= argument. A missing or
    // invalid id still dispatches the command, as PADMAP_UNSET, so the host
    // gets an UNKNOWN_PADMAP error instead of silence. The name must be
    // followed by a space or end of line, so "SET_PADMAPX" doesn't match.
    const struct { const char* name; HostCommand cmd; } padmapCmds[] = {
        { "SET_PADMAP",         HostCommand::SET_PADMAP },
        { "SET_DEFAULT_PADMAP", HostCommand::SET_DEFAULT_PADMAP },
    };
    for (const auto& pc : padmapCmds) {
        const size_t len = strlen(pc.name);
        if (strncmp(line, pc.name, len) != 0 || (line[len] != ' ' && line[len] != '\0')) continue;
        uint32_t id;
        _padmapIdArg = (parseKvUint(line + len, "id", id) == KvStatus::OK && id < EepromData::PADMAP_UNSET)
                           ? (uint8_t)id : EepromData::PADMAP_UNSET;
        return pc.cmd;
    }

    if (strncmp(line, "PROVISION ", sizeof("PROVISION ") - 1) == 0)
        return parseProvision(line + sizeof("PROVISION ") - 1);

    return HostCommand::NONE;
}

// YYYYMMDD with a plausible year and in-range month and day.
static bool isValidDate(uint32_t d) {
    const uint32_t year = d / 10000, month = d / 100 % 100, day = d % 100;
    return year >= 2000 && year <= 2099 && month >= 1 && month <= 12 && day >= 1 && day <= 31;
}

// Fills _provision. Omitted fields keep their unset sentinels; the state
// machine reports the required ones as MISSING_<FIELD>. A field that is
// present but malformed or out of range makes the whole request
// PROVISION_INVALID, naming the first such field in _provisionError, so
// nothing is written from a half-understood line. The sentinels themselves
// (0xFF, 0xFFFFFFFF) are out of range, so a host can't send "unset".
// Unknown keys are ignored, so newer hosts can add fields.
HostCommand HostProtocol::parseProvision(const char* args) {
    _provision.markUnset();
    _provisionError = nullptr;

    uint32_t hw;
    KvStatus hwStatus = parseKvUint(args, "hw", hw);
    if (hwStatus == KvStatus::INVALID || (hwStatus == KvStatus::OK && hw >= EepromData::HWID_UNSET)) {
        _provisionError = "INVALID_HW";
        return HostCommand::PROVISION_INVALID;
    }
    if (hwStatus == KvStatus::OK) _provision.hwId = (uint8_t)hw;

    if (parseKvUintList(args, "padmap", _provision.padmapIds, AdapterBase::PADMAP_ID_COUNT) == KvStatus::INVALID) {
        _provisionError = "INVALID_PADMAP";
        return HostCommand::PROVISION_INVALID;
    }

    struct Field { const char* key; uint32_t* dst; uint32_t min, max; const char* error; };
    const Field fields[] = {
        { "lifespan", &_provision.designedLifespan,  1, EepromData::FIELD_UNSET - 1, "INVALID_LIFESPAN" },
        { "date",     &_provision.dateOfManufacture, 0, EepromData::FIELD_UNSET - 1, "INVALID_DATE" },
        { "ins",      &_provision.insertionCount,    0, EepromData::FIELD_UNSET - 1, "INVALID_INS" },
        { "tests",    &_provision.testCount,         0, EepromData::FIELD_UNSET - 1, "INVALID_TESTS" },
        { "eol",      &_provision.eol,               0, 1,                           "INVALID_EOL" },
    };
    for (const Field& f : fields) {
        uint32_t v;
        KvStatus st = parseKvUint(args, f.key, v);
        if (st == KvStatus::ABSENT) continue;
        if (st == KvStatus::INVALID || v < f.min || v > f.max) {
            _provisionError = f.error;
            return HostCommand::PROVISION_INVALID;
        }
        *f.dst = v;
    }
    if (_provision.hasDate() && !isValidDate(_provision.dateOfManufacture)) {
        _provisionError = "INVALID_DATE";
        return HostCommand::PROVISION_INVALID;
    }
    return HostCommand::PROVISION;
}

// Unsigned decimal at s: one or more digits, no sign, no overflow. *end is set
// to the first character after the digits.
static bool parseDecimal(const char* s, const char** end, uint32_t& out) {
    uint32_t v = 0;
    const char* p = s;
    for (; *p >= '0' && *p <= '9'; p++) {
        const uint32_t digit = *p - '0';
        if (v > (UINT32_MAX - digit) / 10) return false;
        v = v * 10 + digit;
    }
    if (p == s) return false;
    *end = p;
    out = v;
    return true;
}

// Finds key=<value> among space-separated tokens. INVALID if the value is not
// a plain decimal running up to the next space or end of line.
HostProtocol::KvStatus HostProtocol::parseKvUint(const char* kv, const char* key, uint32_t& out) {
    uint8_t klen = strlen(key);
    const char* p = kv;
    while (*p) {
        if (strncmp(p, key, klen) == 0 && p[klen] == '=') {
            const char* end;
            if (!parseDecimal(p + klen + 1, &end, out) || (*end != ' ' && *end != '\0')) return KvStatus::INVALID;
            return KvStatus::OK;
        }
        while (*p && *p != ' ') p++;
        if (*p == ' ') p++;
    }
    return KvStatus::ABSENT;
}

// key=<id>[,<id>...] into out, PADMAP_UNSET-terminated. INVALID on an empty
// or non-numeric entry (a trailing or doubled comma would otherwise be a
// phantom ID), an ID that collides with PADMAP_UNSET, or more than maxCount
// entries.
HostProtocol::KvStatus HostProtocol::parseKvUintList(const char* kv, const char* key, uint8_t* out, uint8_t maxCount) {
    uint8_t klen = strlen(key);
    const char* p = kv;
    while (*p) {
        if (strncmp(p, key, klen) == 0 && p[klen] == '=') {
            const char* val = p + klen + 1;
            uint8_t count = 0;
            while (true) {
                uint32_t id;
                if (count == maxCount || !parseDecimal(val, &val, id) || id >= EepromData::PADMAP_UNSET)
                    return KvStatus::INVALID;
                out[count++] = (uint8_t)id;
                if (*val == ',') { val++; continue; }
                if (*val != ' ' && *val != '\0') return KvStatus::INVALID;
                break;
            }
            while (count < maxCount) out[count++] = EepromData::PADMAP_UNSET;
            return KvStatus::OK;
        }
        while (*p && *p != ' ') p++;
        if (*p == ' ') p++;
    }
    return KvStatus::ABSENT;
}

// aid= on the wire: the adapter's 16-hex-digit AT21CS01 serial (HELLO's uid=
// is the tester's own board ID). nullptr falls back to sixteen '0's so the
// host always gets a valid-length aid.
void HostProtocol::setAdapterUid(const char* uid16) {
    if (uid16) {
        strncpy(_adapterUid, uid16, sizeof(_adapterUid) - 1);
        _adapterUid[sizeof(_adapterUid) - 1] = '\0';
    } else {
        memset(_adapterUid, '0', sizeof(_adapterUid) - 1);
        _adapterUid[sizeof(_adapterUid) - 1] = '\0';
    }
}

// ——————————————————————————————————————————————————————————————————————————

// " pm=<id>[,<id>...]" from a PADMAP_UNSET-terminated list; nothing if empty.
void HostProtocol::printPadmapList(const uint8_t (&padmapIds)[AdapterBase::PADMAP_ID_COUNT]) {
    uint8_t pmCount = 0;
    for (uint8_t i = 0; i < AdapterBase::PADMAP_ID_COUNT && padmapIds[i] != EepromData::PADMAP_UNSET; i++) pmCount++;
    if (pmCount == 0) return;
    Serial.print(" pm=");
    for (uint8_t i = 0; i < pmCount; i++) {
        if (i > 0) Serial.print(',');
        Serial.print(padmapIds[i]);
    }
}

// " active=<id>" for the pad map the next test will run; nothing if none.
void HostProtocol::printActivePadmap(const PadMap* activePadMap) {
    if (!activePadMap) return;
    Serial.print(" active=");
    Serial.print(activePadMap->id);
}

void HostProtocol::sendAdapterInfo(uint8_t hwId, const uint8_t (&padmapIds)[AdapterBase::PADMAP_ID_COUNT],
                                    const PadMap* activePadMap,
                                    uint32_t lifespan, uint32_t dateOfManufacture,
                                    uint32_t insertions, uint32_t tests, bool eol,
                                    bool dutPresent, const char* state) {
    Serial.print("ADAPTER");
    Serial.print(" aid=");     Serial.print(_adapterUid);
    Serial.print(" ahw=");     Serial.print(hwId);
    printPadmapList(padmapIds);
    printActivePadmap(activePadMap);
    Serial.print(" lifespan=");   Serial.print(lifespan);
    Serial.print(" mfg_date=");   Serial.print(dateOfManufacture);
    Serial.print(" ins=");        Serial.print(insertions);
    Serial.print(" tests=");      Serial.print(tests);
    Serial.print(" eol=");        Serial.print(eol ? 1 : 0);
    Serial.print(" dut=");        Serial.print(dutPresent ? 1 : 0);
    // State machine state, so a host that connects mid-session can tell e.g.
    // READY from PASS without having seen the events.
    Serial.print(" state=");      Serial.print(state);
    Serial.println();
}

void HostProtocol::sendAdapterDetected(uint8_t hwId, const uint8_t (&padmapIds)[AdapterBase::PADMAP_ID_COUNT],
                                        const PadMap* activePadMap) {
    Serial.print("EVENT ADAPTER_DETECTED ");
    Serial.print("aid=");     Serial.print(_adapterUid);
    Serial.print(" ahw=");    Serial.print(hwId);
    printPadmapList(padmapIds);
    printActivePadmap(activePadMap);
    Serial.println();
}

// pads= counts measured pads only; DISCHARGE steps never produce a PAD line.
// name= is always the last field and runs to end of line, because pad map
// names contain spaces.
void HostProtocol::sendPadmapEntry(const PadMap& padMap, bool isDefault, bool isActive) {
    uint8_t pads = 0;
    for (uint8_t i = 0; i < padMap.caseCount; i++)
        if (padMap.cases[i].strategy != TestStrategy::DISCHARGE) pads++;

    Serial.print("PADMAP id=");    Serial.print(padMap.id);
    Serial.print(" pads=");        Serial.print(pads);
    Serial.print(" default=");     Serial.print(isDefault ? 1 : 0);
    Serial.print(" active=");      Serial.print(isActive ? 1 : 0);
    Serial.print(" name=");        Serial.println(padMap.name);
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

// Thresholds are per test case, but every case in a pad map shares kThresh
// today, so one value describes the map. Omitted if no case has one.
void HostProtocol::printBondThreshold(const PadMap* padMap) {
    if (!padMap) return;
    for (uint8_t i = 0; i < padMap->caseCount; i++) {
        if (padMap->cases[i].thresholds) {
            Serial.print(" max_bond_r_ohms=");
            Serial.print(padMap->cases[i].thresholds->maxBondResistanceOhms, 0);
            return;
        }
    }
}

// settleUs is per test case, but every CAP_SENSE case in a pad map shares one
// today, so one schedule describes the map. Omitted if the map has no
// CAP_SENSE cases.
void HostProtocol::printCapSchedule(const PadMap* padMap) {
    if (!padMap) return;
    for (uint8_t i = 0; i < padMap->caseCount; i++) {
        if (padMap->cases[i].strategy != TestStrategy::CAP_SENSE) continue;
        uint16_t times[CAP_SENSE_SAMPLE_COUNT];
        curveSampleTimesUs(padMap->cases[i].settleUs, times);
        Serial.print(" cap_time_list_us=");
        for (uint8_t k = 0; k < CAP_SENSE_SAMPLE_COUNT; k++) {
            if (k > 0) Serial.print(',');
            Serial.print(times[k]);
        }
        return;
    }
}

void HostProtocol::sendTestStart(uint8_t hwId, const uint8_t (&padmapIds)[AdapterBase::PADMAP_ID_COUNT], const PadMap* padMap,
                                  uint32_t insertions, uint32_t tests) {
    // Counters as of now: tests= excludes this test, which is only counted
    // once it completes.
    Serial.printf("EVENT TEST_START aid=%s ahw=%u ins=%lu tests=%lu",
                  _adapterUid, hwId, (unsigned long)insertions, (unsigned long)tests);
    printPadmapList(padmapIds);
    printActivePadmap(padMap);

    // Fixed per firmware build, so sent once here instead of on every PAD
    // line. Same order as the rf/rr/vf/vr values, lowest current first.
    Serial.print(" current_list_ua=");
    for (uint8_t i = 0; i < PULLUP_LEVEL_COUNT; i++) {
        if (i > 0) Serial.print(',');
        Serial.print(PULLUP_LEVELS[i].nominalUA, 0);
    }

    printBondThreshold(padMap);
    printCapSchedule(padMap);
    Serial.println();
}

void HostProtocol::sendEolWarning(uint32_t insertionCount) {
    Serial.print("EVENT EOL_WARNING ins=");
    Serial.println(insertionCount);
}

void HostProtocol::sendWrongOrientation() {
    Serial.println("EVENT WRONG_ORIENTATION");
}

// One direction group as CSV: per pullup level for STD, per curve sample for CAP.
static void printReadingGroupCsv(const PadReading* group, uint8_t count, bool resistance) {
    for (uint8_t k = 0; k < count; k++) {
        if (k > 0) Serial.print(',');
        if (resistance) Serial.print(group[k].resistanceOhms, 0);
        else            Serial.print(group[k].voltageV, 3);
    }
}

// Wire names for PadType (type= on PAD lines).
static const char* padTypeName(PadType t) {
    switch (t) {
        case PadType::IO:       return "IO";
        case PadType::VDDIO:    return "VDDIO";
        case PadType::VDD_CORE: return "VDD_CORE";
        case PadType::PWR_AUX:  return "PWR_AUX";
        case PadType::GND:      return "GND";
    }
    return "IO";
}

void HostProtocol::sendPadResult(uint8_t slot, uint8_t adapterPin, uint8_t diePad,
                                  TestStrategy strategy, PadType padType,
                                  MeasureDirections dirs, const PadResult& r) {
    // method= tells the host which directions the line carries: STD/CAP for
    // both, a _FW or _REV suffix for one. Unmeasured directions are left off
    // entirely — their zeroed 0Ω/0V values would read as a dead short.
    const bool fwdMeasured = measuresForward(dirs);
    const bool revMeasured = measuresReverse(dirs);
    const char* dirSuffix = (fwdMeasured && revMeasured) ? "" : (fwdMeasured ? "_FW" : "_REV");

    Serial.print("PAD ");
    Serial.print("slot=");   Serial.print(slot);
    Serial.print(" apin=");    Serial.print(adapterPin);
    Serial.print(" dp=");     Serial.print(diePad);
    Serial.print(" type=");   Serial.print(padTypeName(padType));
    Serial.print(" method="); Serial.print(strategy == TestStrategy::CAP_SENSE ? "CAP" : "STD");
    Serial.print(dirSuffix);
    Serial.print(" result=");
    // NOT_TESTED shouldn't reach the wire; spelled out so it's never sent as OPEN.
    Serial.print(r.bond == BondResult::GOOD ? "GOOD" : r.bond == BondResult::OPEN ? "OPEN" : "NOT_TESTED");

    if (strategy == TestStrategy::CAP_SENSE) {
        // Raw curve voltages only, forward and reverse under separate keys
        // (they are two separate charge cycles). No resistances: the formula
        // assumes a settled reading and is meaningless mid-charge — the final
        // sample's verdict is already in result=.
        const PadReading* fwd = r.fwd;
        const PadReading* rev = r.rev;
        if (fwdMeasured) {
            Serial.print(" vfs="); printReadingGroupCsv(fwd, readingsPerDir(strategy), false);
        }
        if (revMeasured) {
            Serial.print(" vrs="); printReadingGroupCsv(rev, readingsPerDir(strategy), false);
        }
    } else {
        const PadReading* fwd = r.fwd;
        const PadReading* rev = r.rev;

        if (fwdMeasured) {
            Serial.print(" rf="); printReadingGroupCsv(fwd, readingsPerDir(strategy), true);
        }
        if (revMeasured) {
            Serial.print(" rr="); printReadingGroupCsv(rev, readingsPerDir(strategy), true);
        }
        if (fwdMeasured) {
            Serial.print(" vf="); printReadingGroupCsv(fwd, readingsPerDir(strategy), false);
        }
        if (revMeasured) {
            Serial.print(" vr="); printReadingGroupCsv(rev, readingsPerDir(strategy), false);
        }
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

void HostProtocol::sendOk(const char* what) {
    Serial.print("OK ");
    Serial.println(what);
}

void HostProtocol::sendOkPadmap(const char* what, uint8_t padmapId) {
    Serial.print("OK ");
    Serial.print(what);
    Serial.print(" id=");
    Serial.println(padmapId);
}

// uid= is the tester's own RP2350 board ID, distinct from the adapter's aid=.
void HostProtocol::sendHello() {
    Serial.print("HELLO name=");
    Serial.print(FW_NAME);
    Serial.print(" build=");
    Serial.print(FW_BUILD_ID);
    Serial.print(" uid=");
    char uidhex[17];
    pico_get_unique_board_id_string(uidhex, sizeof(uidhex));
    Serial.print(uidhex);
    Serial.print(" proto=");
    Serial.print(PROTOCOL_VERSION);
    Serial.println();
}
