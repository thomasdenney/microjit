#pragma once

#include "Config.h"

#include "Encoder.h"
#include <cstdlib>

namespace ARM {

/**
 * Dumps the address, hex, binary, and ARM (where possible) for a function
 */
void printFunction(void (*func)(void), size_t length);

/** GENERAL INSTRUCTION DECODING */

/**
 * |buffer| should be an empty string (i.e. all zero) with sufficient capacity
 * to fill with the decoded instruction. If instruction decoding fails then the
 * buffer is left empty */
char* decode(ARM::Instruction instruction, char* buffer);

/**
 * Alternative version that can be used with branch instructions
 */
char* decode(ARM::Instruction instruction, char* buffer, uint16_t* address);

bool isLongCall(ARM::Instruction instruction);

const char* decodeCondition(ARM::Condition c);

const char* decodeRegister(ARM::Condition r);

/** BL, BLX (1) A7-26 */
void decodeBranchLong(ARM::Instruction instruction1, ARM::Instruction instruction2, char* buffer, uint16_t* address);
}