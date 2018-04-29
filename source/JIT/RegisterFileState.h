#pragma once

#include "ARM/Encoder.h"
#include "ARM/Functor.h"
#include "Config.h"
#include <cstdint>
#include <utility>

namespace JIT {

const static ARM::Register StackRegisters[] = {
    ARM::Register::r2,
    // Keep TempRegister == r3 free
    // The next two are temp registers but they are only used by the stack check, so we can use them after that
    ARM::Register::r4,
    ARM::Register::r5,
    ARM::Register::r6,
    ARM::Register::r7,
};

/**
 * Only use this in places where a compile-time constant is needed and a constant variable
 * can't be used instead
 */
#define NUMBER_OF_REGISTERS_FOR_STACK (sizeof(StackRegisters) / sizeof(ARM::Register))

const static int32_t NumberOfRegistersAvailableForStack = NUMBER_OF_REGISTERS_FOR_STACK;

/**
 * An abstract class for handling which stack values are in which registers, how that corresponds
 * to the actual stack state, and reading/writing those values back to memory
 */
class RegisterFileState {
public:
    virtual bool inNaiveState() = 0;

    /**
     * Seen as we only have five registers to play with, 1 <= n <= 5
     * Returns whether this succeeded
     */
    virtual bool ensureRegistersHoldValues(int n, ARM::Functor& func) = 0;

    /**
     * This should only be used when you want to know the register to write-back to
     */
    virtual ARM::Register topOfStackWriteBackRegister() = 0;

    /**
     * This should only be used when you want to know the register to write to
     */
    virtual ARM::Register readRegister(int n) = 0;

    /**
     * Doesn't emit any assembly, just updates internal state
     * Returns the register that WAS the top of stack
     * 
     * TODO: Ensure that it is never possible for m_numberOfRegistersHoldingValues to go below 1 (???)
     */
    virtual ARM::Register pop() = 0;

    /**
     * Returns the register that IS the top of stack
     * Handles the case that we already have the maximum number of values in registers so one of them needs to get
     * written back.
     */
    virtual ARM::Register push(ARM::Functor& func) = 0;

    /**
     * Returns whether this succeeded
     */
    virtual bool returnToNaiveState(ARM::Functor& func) = 0;

    /**
     * Returns whether this succeeded
     * Call comparisonRegisters to get the two registers that you should emit a compare operation
     * for immediately after this method.
     */
    virtual bool returnToComparisonState(ARM::Functor& func) = 0;

    virtual std::pair<ARM::Register, ARM::Register> comparisonRegisters() = 0;

    /**
     * Implements the duplicate operation
     * Clients should raise a register allocation error on failure
     */
    virtual bool dupTopOfStack(ARM::Functor& func) = 0;

    /**
     * Implements the drop operation
     * Clients should raise a register allocation error on failure
     */
    virtual bool dropTopOfStack(ARM::Functor& func) = 0;

    /**
     * Implements the rotation operation
     * Clients should raise a register allocation error on failure
     */
    virtual bool rot(ARM::Functor& func) = 0;

    /**
     * Implements the swap operation
     * Clients should raise a register allocation error on failure
     */
    virtual bool swap(ARM::Functor& func) = 0;

    /**
     * Implements the tuck operation
     * Clients should raise a register allocation error on failure
     */
    virtual bool tuck(ARM::Functor& func) = 0;

    virtual bool ntuck(ARM::Functor& func, int n) { return false; }

    /**
     * Only used in a particular mode of the COW allocator
     * 
     * Implementations should return false if they don't automatically hold register values
     * or if it is currently disabled (RegisterWriteElimination in Config.h is false)
     */
    virtual bool registerValueIsKnown(ARM::Register reg) = 0;

    /**
     * Ignore the return result if |knownStackValue(index)| is false
     */
    virtual int knownRegisterValue(ARM::Register reg) = 0;

    /**
     * Doesn't have to do anything if not supported
     */
    virtual void setKnownRegisterValue(ARM::Functor& func, ARM::Register reg, int value) = 0;

    virtual void commitRegisterValue(ARM::Functor& func, int stackOffset) = 0;
    virtual void commitRegisterValue(ARM::Functor& func, ARM::Register reg) = 0;
};
}