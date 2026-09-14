#pragma once
#include <stdint.h>
#include "test/result.h"
#include "test/pad_map.h"
#include "adapter/eeprom_layout.h"

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
};

class HostProtocol {
public:
    void        begin();
    HostCommand poll();
    uint8_t     setPadmapId() const { return _setPadmapId; }

    const ProvisionRequest& provisionRequest() const { return _provision; }

    void setAdapterUid(const char* uid16);

    void sendAdapterInfo(uint8_t hwId, const uint8_t (&padmapIds)[AdapterBase::PADMAP_ID_COUNT],
                         uint32_t lifespan, uint32_t dateOfManufacture,
                         uint32_t insertions, uint32_t tests, bool eol,
                         bool dutPresent);
    void sendAdapterDetected(uint8_t hwId, const uint8_t (&padmapIds)[AdapterBase::PADMAP_ID_COUNT]);
    void sendAdapterRemoved();
    void sendDutInserted();
    void sendDutRemoved();
    void sendTestStart(uint8_t hwId, const uint8_t (&padmapIds)[AdapterBase::PADMAP_ID_COUNT], const PadMap* padMap,
                       uint32_t insertions, uint32_t tests);
    void sendEolWarning(uint32_t insertionCount);
    void sendWrongOrientation();
    void sendPadResult(uint8_t slot, uint8_t adapterPin, uint8_t diePad, TestStrategy strategy,
                       MeasureDirections dirs, const PadResult& r);
    void sendSlotStatus(uint8_t slot, bool present, bool tested);
    void sendSummary(const TestResult& result);
    void sendError(ErrorCode code, const char* msg);
    void sendFault(const char* msg);
    void sendOk(const char* what);
    void sendHello();

private:
    HostCommand processLine(const char* line);
    static bool parseKvUint(const char* kv, const char* key, uint32_t& out);
    static bool parseKvUintList(const char* kv, const char* key, uint8_t* out, uint8_t maxCount);
    void printPadmapList(const uint8_t (&padmapIds)[AdapterBase::PADMAP_ID_COUNT]);
    void printBondThreshold(const PadMap* padMap);
    void printCapSchedule(const PadMap* padMap);

    char     _lineBuf[256];
    uint16_t _lineLen           = 0;
    bool     _overflowWarned    = false;
uint8_t  _setPadmapId       = 0;
    ProvisionRequest _provision;  // markUnset()ed at the start of each PROVISION parse
    char     _adapterUid[17]    = {};  // aid= on the wire — adapter's AT21CS01 serial (HELLO's uid= is the tester's own board ID)
};
