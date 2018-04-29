#pragma once

#include "Config.h"

#include <cstddef>
#include <cstdint>

namespace Transfer {

#define MAX_SERIAL_BUFFER_SIZE 128

class Serial {
private:
    static size_t readProgramLength();
    static void readProgramSafe();

    /// Returns -1 in case of failure
    static int readUnsignedNumber();

public:
    static void readProgram();
    static size_t programLength();
    static uint8_t* programBuffer();
};
}