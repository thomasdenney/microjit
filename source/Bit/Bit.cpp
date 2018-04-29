#include "Bit.h"

#include <cstdlib>

namespace Bit {

int unTwosComplement(uint32_t value, uint8_t bits)
{
    // Sign-bit is set at the front
    if (value & (1 << (bits - 1))) {
        // TODO Be smarter about this
        return value - (1 << bits);
    }
    return (int)value;
}

char* itob(int32_t x, char* buffer)
{
    const size_t len = 16; // TODO Parameterise - but the function signature is the same as itoa
    buffer[len] = 0;
    for (size_t i = 0; i < len; i++) {
        buffer[i] = ((x >> ((len - 1) - i)) & 1) + '0';
    }
    return buffer;
}

uint32_t twosComplement(int32_t x, uint8_t bits)
{
    uint32_t y = (uint32_t)x;
    if (y & (1 << 31)) {
        y = (uint32_t)(1 << (bits - 1)) | (uint32_t)(x + (1 << (bits - 1)));
    }
    return y;
}
}