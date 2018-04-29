#pragma once

#include "Config.h"

#include <cstdint>

namespace Bit {

/**
 * Takes a twos-complemenet integer stored in the lower |bits| bits of |value|
 * and returns the 32-bit representation
 */
int unTwosComplement(uint32_t value, uint8_t bits);

/** Like itoa, but decodes binary in 16 bits only */
char* itob(int32_t x, char* buffer);

/** Retrieves the value of instruction[offset..(offset + length)) */
inline constexpr uint16_t uintRegion(unsigned instruction, uint8_t offset, uint8_t length)
{
    return ((((1 << length) - 1) << offset) & instruction) >> offset;
}

/**
 * Represents |x| as a twos complement number in the lower |bits| bits of the
 * result
 */
uint32_t twosComplement(int32_t x, uint8_t bits);
}