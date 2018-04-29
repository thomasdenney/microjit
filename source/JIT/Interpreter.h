#pragma once

#include "Config.h"

#include "Code/Instruction.h"
#include "Environment/Device.h"
#include "Environment/VM.h"

namespace JIT {

/**
 * Sets the device that is used by the interpreter only; the compiler is self-contained
 */
void setGlobalInterpreterDevice(Environment::Device* device);

/**
 * This function implements the complete virtual machine. It is a simple
 * implementation that effectively unrolls a tail-recursive implementation of
 * Stack.
 *
 * In theory this can be called by the JIT virtual machine once it is
 * implemented.
 *
 * Calling convention:
 *  - The first register contains the address of the Virtual Machine state,
 *    which contains the stack, program counter, and original code
 *  - The second register contains the address of the top of the stack
 *  - The third register contains the value of the top of the stack, or 0
 *    otherwise
 *
 * The return value is always the address of the Virtual Machine state. This
 * address never changes during execution, but the contents (hopefully) will
 *
 * Recursion is implemented using the normal stack.
 */
Environment::VM* execute(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

/**
 * Same as the above but fills the stackPointer and topOfStackParameters for you
 */
Environment::VM* execute(Environment::VM* state);

#define OVERFLOW_CHECK(n)                                                          \
    do {                                                                           \
        state = JIT::preCheckForStackOverflow(state, stackPointer, topOfStack, n); \
        if (state->m_status != Environment::VMStatus::Success) {                   \
            return state;                                                          \
        }                                                                          \
    } while (0)

#define UNDERFLOW_CHECK(n)                                                          \
    do {                                                                            \
        state = JIT::preCheckForStackUnderflow(state, stackPointer, topOfStack, n); \
        if (state->m_status != Environment::VMStatus::Success) {                    \
            return state;                                                           \
        }                                                                           \
    } while (0)

inline Environment::VM* preCheckForStackOverflow(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack, uint32_t pushCount)
{
    if (stackPointer - pushCount < state->m_stack.m_base) {
        state->m_status = Environment::VMStatus::StackOverflow;
    }
    return state;
}

inline Environment::VM* preCheckForStackUnderflow(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack, uint32_t popCount)
{
    if (stackPointer + popCount > state->m_stack.m_end) {
        state->m_status = Environment::VMStatus::StackUnderflow;
    }
    return state;
}

/**
 * These functions are used by both the interpreter and the compiler
 */

Environment::VM* executeDiv(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeMod(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeMax(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeMin(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeRot(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeNrot(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeTuck(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeNtuck(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeSize(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeEffect(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack, Code::Instruction effect);
}