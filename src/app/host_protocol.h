#pragma once
#include <stdint.h>
#include "test/result.h"
#include "test/pad_map.h"
#include "adapter/eeprom_layout.h"

// Serial protocol version, reported as proto= on HELLO. Bump it whenever a
// command or message format changes, so host tools can detect firmware that is
// too old (or too new) for them. HELLO without proto= means version 1.
//   1 — original protocol
//   2 — LIST_PADMAPS, SET_DEFAULT_PADMAP, OK reply to SET_PADMAP,
//       active= on ADAPTER/ADAPTER_DETECTED/TEST_START, state= on ADAPTER,
//       type= on PAD
constexpr uint8_t PROTOCOL_VERSION = 2;

enum class ErrorCode : uint8_t {
    NO_ADAPTER            = 1,
    BUSY                  = 2,
    UNKNOWN_PADMAP        = 3,
    PROVISION_FAILED      = 4,  // PROVISION rejected — malformed request or refused write at provision time
    NOT_IMPLEMENTED       = 5,
    MISSING_FIELD         = 6,
    ADAPTER_NOT_PROVISIONED = 7,  // EEPROM chip present but blank — needs PROVISION before use
    WRONG_STATE             = 8,  // command not valid in the tester's current state
    EEPROM_WRITE_FAILED     = 9,  // RESERVED — superseded by EVENT FAULT (R2 escalation); never emitted since
    PADMAP_INCOMPATIBLE     = 10, // pad map exists but is written for a different adapter pinout
};

enum class HostCommand : uint8_t {
    NONE,
    RUN,
    GET_RESULTS,
    SET_PADMAP,
    PROVISION,
    PROVISION_INVALID,
    GET_ADAPTER,
    HELLO,
    LIST_PADMAPS,
    SET_DEFAULT_PADMAP,
};

class HostProtocol {
public:
    void        begin();
    HostCommand poll();
    // id= argument of the last SET_PADMAP / SET_DEFAULT_PADMAP. PADMAP_UNSET
    // when the line had no usable id, which no registered pad map matches.
    uint8_t     padmapIdArg() const { return _padmapIdArg; }

    const ProvisionRequest& provisionRequest() const { return _provision; }

    void setAdapterUid(const char* uid16);

    // activePadMap may be nullptr (no compatible map); active= is then omitted.
    void sendAdapterInfo(uint8_t hwId, const uint8_t (&padmapIds)[AdapterBase::PADMAP_ID_COUNT],
                         const PadMap* activePadMap,
                         uint32_t lifespan, uint32_t dateOfManufacture,
                         uint32_t insertions, uint32_t tests, bool eol,
                         bool dutPresent, const char* state);
    void sendAdapterDetected(uint8_t hwId, const uint8_t (&padmapIds)[AdapterBase::PADMAP_ID_COUNT],
                             const PadMap* activePadMap);
    // One LIST_PADMAPS entry. The caller sends the terminating OK line.
    void sendPadmapEntry(const PadMap& padMap, bool isDefault, bool isActive);
    void sendAdapterRemoved();
    void sendDutInserted();
    void sendDutRemoved();
    void sendTestStart(uint8_t hwId, const uint8_t (&padmapIds)[AdapterBase::PADMAP_ID_COUNT], const PadMap* padMap,
                       uint32_t insertions, uint32_t tests);
    void sendEolWarning(uint32_t insertionCount);
    void sendWrongOrientation();
    void sendPadResult(uint8_t slot, uint8_t adapterPin, uint8_t diePad, TestStrategy strategy,
                       PadType padType, MeasureDirections dirs, const PadResult& r);
    void sendSlotStatus(uint8_t slot, bool present, bool tested);
    void sendSummary(const TestResult& result);
    void sendError(ErrorCode code, const char* msg);
    void sendFault(const char* msg);
    void sendOk(const char* what);
    void sendOkPadmap(const char* what, uint8_t padmapId);  // "OK <what> id=<padmapId>"
    void sendHello();

private:
    HostCommand processLine(const char* line);
    static bool parseKvUint(const char* kv, const char* key, uint32_t& out);
    static bool parseKvUintList(const char* kv, const char* key, uint8_t* out, uint8_t maxCount);
    void printPadmapList(const uint8_t (&padmapIds)[AdapterBase::PADMAP_ID_COUNT]);
    void printActivePadmap(const PadMap* activePadMap);
    void printBondThreshold(const PadMap* padMap);
    void printCapSchedule(const PadMap* padMap);

    char     _lineBuf[256];
    uint16_t _lineLen           = 0;
    bool     _overflowWarned    = false;
    uint8_t  _padmapIdArg       = EepromData::PADMAP_UNSET;
    ProvisionRequest _provision;  // markUnset()ed at the start of each PROVISION parse
    char     _adapterUid[17]    = {};  // aid= on the wire — adapter's AT21CS01 serial (HELLO's uid= is the tester's own board ID)
};
