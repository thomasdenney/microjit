#pragma once

#include "Config.h"

#include <cstdint>

namespace Environment {

/**
 * Although this is represented with a uint32_t it is essential
 * that all enum values can fit in uint8_t because of instruction
 * generation
 */
enum class VMStatus : uint32_t {
    Success = 0,
    UnknownFailure = 1,
    StackOverflow = 2,
    StackUnderflow = 3,
    OutOfBoundsFetch = 4,
    CompilerError = 5
};

const char* VMStatusString(VMStatus status);
}