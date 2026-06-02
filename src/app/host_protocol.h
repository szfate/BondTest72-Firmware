#pragma once
#include <stdint.h>
#include "test/result.h"
#include "test/pad_map.h"

enum class HostCommand : uint8_t {
    NONE,
    RUN,
    GET_RESULTS,
    SET_PADMAP,
    DISCOVER,
    PROVISION,
    GET_ADAPTER,
    DISCOVERY_SCAN,
};

class HostProtocol {
public:
    void        begin();
    HostCommand poll();
    uint8_t     setPadmapId() const { return _setPadmapId; }

    uint8_t     provisionPadmapId()   const { return _provisionPadmapId; }
    uint32_t    provisionMfgDate()    const { return _provisionMfgDate; }  // YYYYMMDD
    void sendAdapterInfo(uint8_t model, uint8_t version, const uint8_t* padmapIds,
                         uint32_t lifespan, uint32_t dateOfManufacture,
                         uint32_t insertions, uint32_t tests, bool eol,
                         const char* padMapName);
    void sendAdapterDetected(uint8_t model, uint8_t version, const uint8_t* padmapIds);
    void sendDutInserted();
    void sendDutRemoved();
    void sendTestStart(uint8_t model, uint8_t version, const uint8_t* padmapIds);
    void sendPadResult(uint8_t slot, uint8_t mezPin, uint8_t diePad, const PadResult& r);
    void sendSlotStatus(uint8_t slot, bool present, bool tested);
    void sendSummary(const TestResult& result);
    void sendError(const char* description);
    void sendDiscoveryScanPoint(uint8_t src, uint8_t snk, float v);
    void sendDiscoveryScanDone();

private:
    HostCommand processLine(const char* line);

    char    _lineBuf[64];
    uint8_t _lineLen           = 0;
    bool    _overflowWarned    = false;
    uint8_t _setPadmapId       = 0;
    uint8_t  _provisionPadmapId  = 0xFF;
    uint32_t _provisionMfgDate = 0;
};
