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
    COUNT,  // sentinel — array sizing only
};

// Wire/log spelling of a State — shared by the state machine's logs, the
// WRONG_STATE error msg= field, and anything else that prints a state.
inline const char* stateName(State s) {
    switch (s) {
        case State::NO_ADAPTER:        return "NO_ADAPTER";
        case State::EOL_ADAPTER:       return "EOL_ADAPTER";
        case State::ADAPTER_DETECTED:  return "ADAPTER_DETECTED";
        case State::READY:             return "READY";
        case State::WRONG_ORIENTATION: return "WRONG_ORIENTATION";
        case State::TESTING:           return "TESTING";
        case State::PASS:              return "PASS";
        case State::FAIL:              return "FAIL";
        case State::FAULT:             return "FAULT";
        default:                       return "?";
    }
}