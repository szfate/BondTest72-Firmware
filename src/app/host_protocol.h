#pragma once
#include <stdint.h>
#include "../test/result.h"
#include "../test/pad_map.h"

enum class HostCommand : uint8_t {
    NONE,
    RUN,
    GET_RESULTS,
    SET_PADMAP,
    DISCOVER,
    PROVISION,
    DEBUG_PWR_SWEEP,
    DEBUG_BIASED_SWEEP,
    GET_ADAPTER,
};

class HostProtocol {
public:
    void        begin();
    HostCommand poll();
    uint8_t     setPadmapId() const { return _setPadmapId; }

    uint8_t     provisionPadmapId()  const { return _provisionPadmapId; }
    uint8_t     debugPadMapId()      const { return _debugPadMapId; }

    void sendAdapterInfo(uint8_t model, uint8_t version, const uint8_t* padmapIds,
                         uint32_t lifespan, uint32_t dateOfManufacture,
                         uint32_t insertions, uint32_t tests, bool eol,
                         const char* padMapName);
    void sendAdapterDetected(uint8_t model, uint8_t version, const uint8_t* padmapIds);
    void sendDutInserted();
    void sendDutRemoved();
    void sendTestStart(uint8_t model, uint8_t version, const uint8_t* padmapIds);
    void sendPadResult(uint8_t slot, uint8_t mezPin, const PadResult& r);
    void sendSummary(const TestResult& result);
    void sendError(const char* description);

private:
    HostCommand processLine(const char* line);

    char    _lineBuf[32];
    uint8_t _lineLen           = 0;
    uint8_t _setPadmapId       = 0;
    uint8_t _provisionPadmapId = 0xFF;
    uint8_t _debugPadMapId     = 0;
};
