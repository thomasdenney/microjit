#pragma once

#include "Environment/VM.h"

// TODO: Verify if it can actually be placed in a namespace, or whether that messes with name mangling
// namespace JIT {

extern "C" {
/**
 * This function is implemented in ASM
 */
Environment::VM* compileFunctionDynamicallyASM(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

// }
}