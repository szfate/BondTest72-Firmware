#pragma once
#include <stdint.h>
#include "test/result.h"
#include "test/pad_map.h"

enum class ErrorCode : uint8_t {
    NO_ADAPTER       = 1,
    BUSY             = 2,
    UNKNOWN_PADMAP   = 3,
    PROVISION_FAILED = 4,
    NOT_IMPLEMENTED  = 5,
};

enum class HostCommand : uint8_t {
    NONE,
    RUN,
    GET_RESULTS,
    SET_PADMAP,
    PROVISION,
    GET_ADAPTER,
    DISCOVERY_SCAN,
    HELLO,
};

class HostProtocol {
public:
    void        begin();
    HostCommand poll();
    uint8_t     setPadmapId() const { return _setPadmapId; }

    const uint8_t* provisionPadmapIds() const { return _provisionPadmapIds; }
    uint8_t     provisionHwId()       const { return _provisionHwId; }
    uint32_t    provisionMfgDate()    const { return _provisionMfgDate; }

    void setUid(const char* uid16);

void sendAdapterInfo(uint8_t hwId, const uint8_t* padmapIds,
                     uint32_t lifespan, uint32_t dateOfManufacture,
                     uint32_t insertions, uint32_t tests, bool eol,
                     bool dutPresent);
    void sendAdapterDetected(uint8_t hwId, const uint8_t* padmapIds);
    void sendAdapterRemoved();
    void sendDutInserted();
    void sendDutRemoved();
    void sendTestStart(uint8_t hwId, const uint8_t* padmapIds);
    void sendEolWarning(uint32_t insertionCount);
    void sendWrongOrientation();
    void sendPadResult(uint8_t slot, uint8_t adapterPin, uint8_t diePad, const PadResult& r);
    void sendSlotStatus(uint8_t slot, bool present, bool tested);
    void sendSummary(const TestResult& result);
    void sendError(ErrorCode code, const char* msg);
    void sendFault(const char* msg);
    void sendDiscoveryScanPoint(uint8_t src, uint8_t snk, float v);
    void sendDiscoveryScanDone();
    void sendHello();

private:
    HostCommand processLine(const char* line);
    static bool parseKvUint(const char* kv, const char* key, uint32_t& out);
    static bool parseKvUintList(const char* kv, const char* key, uint8_t* out, uint8_t maxCount);

    char     _lineBuf[256];
    uint16_t _lineLen           = 0;
    bool     _overflowWarned    = false;
    uint8_t  _setPadmapId       = 0;
    uint8_t  _provisionPadmapIds[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t  _provisionHwId     = 0xFF;
    uint32_t _provisionMfgDate = 0;
    char     _uid[17]           = {};
};