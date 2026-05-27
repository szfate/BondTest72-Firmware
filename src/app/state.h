#pragma once
#include <stdint.h>

enum class State : uint8_t {
    NO_ADAPTER,
    EOL_ADAPTER,
    ADAPTER_DETECTED,
    READY,
    WRONG_ORIENTATION,
    TESTING,
    PASS,
    FAIL,
    FAULT,
};