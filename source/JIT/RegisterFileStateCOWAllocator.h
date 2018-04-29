#pragma once

#include "Config.h"
#include "RegisterFileState.h"

namespace JIT {

// The total number of registers available on the platform
#define REGISTER_COUNT 16

/**
 * Avoids most register move oeprations by maintaining three internal maps to avoid
 *
 *  a) Moving values between registers on stack perations like TUCK, ROT, and SWAP
 *  b) Maintaining separate maps for which registers to read/write to, preventing the DUP
 *     instruction from generating code, except in cases where a value needs to be written
 *     to memory
 *  c) Where values can be statically determined at compile time they don't need to be
 *     written to registers, allowing compile-time arithmetic to avoid needing to do register
 *     operations
 *
 * TODO:
 *
 *  - Although the maps help to avoid a lot of operations, an alternative approach would be to
 *    track which values are at which position on the stack (this could be done by literally
 *    maintaining an internal stack) and then allocate registers afterwards. This means that if you
 *    needed to access a stack value further down the stack that was statically known you
 *    wouldn't have to worry about *ever* writing it to Stack unless absolutely necessary
 *  - Support a 'smarter' naive state where the top five values are in registers, avoiding the
 *    need to access memory for a bunch of operations
 *  - Where NTUCK/NDUP/NROT operations can be statically determined, do
 *  - Deprecate the original RegisterFileState implementation
 */
class RegisterFileStateCOWAllocator : public RegisterFileState {
private:
    bool m_registerIsInUse[REGISTER_COUNT];

    ARM::Register m_readRegisterForOffset[NUMBER_OF_REGISTERS_FOR_STACK];

    ARM::Register m_writeRegisterForOffset[NUMBER_OF_REGISTERS_FOR_STACK];

    int m_topOfStackOffsetFromStackPointer;

    int m_numberOfRegistersHoldingValues;

    ARM::Register m_comparisonRegister1, m_comparisonRegister2;

    bool m_knowRegisterValue[REGISTER_COUNT];
    int m_registerValues[REGISTER_COUNT];

    /**
     * Garbage collects the registers.
     *
     * If a register has readers but no writers it will ensure that it has a writer.
     */
    void redetermineRegistersInUse();

    /**
     * If m_numberOfRegistersHoldingValues == NumberOfRegistersAvailableForStack then the result
     * of this method should be disregarded
     */
    ARM::Register nextFreeRegister();

    bool returnToNaiveState(ARM::Functor& func, int offset);

    bool moveConstantsFromKnownRegisters(ARM::Functor& func);

    /**
     * |offset| is the start stack index to write back to memory from registers. On termination
     * this function will ensure that at least NUMBER_OF_REGISTERS_FOR_STACK - offset registers
     * are free for allocation.
     */
    void resetMemoryInvariant(ARM::Functor& func, int offset, int delta);

    /**
     * Prints a table showing which stack values are in which registers and which registers are in use
     */
    void printStatus();

public:
    RegisterFileStateCOWAllocator();

    bool inNaiveState() final;

    bool ensureRegistersHoldValues(int n, ARM::Functor& func) final;

    /**
     * Calling this will update ensure that future reads to this stack item will be
     * done at this register
     */
    ARM::Register topOfStackWriteBackRegister() final;

    ARM::Register readRegister(int n) final;

    ARM::Register pop() final;

    ARM::Register push(ARM::Functor& func) final;

    bool returnToNaiveState(ARM::Functor& func) final;

    bool dupTopOfStack(ARM::Functor& func) final;

    /// 0 <= n < 4
    // bool ndupTopOfStack(ARM::Functor& func);

    bool dropTopOfStack(ARM::Functor& func) final;

    bool rot(ARM::Functor& func) final;

    /// 1 <= n <= 5
    // bool nrot(ARM::Functor& func, int n);

    bool swap(ARM::Functor& func) final;

    bool tuck(ARM::Functor& func) final;

    /// 0 <= n <= 4
    bool ntuck(ARM::Functor& func, int n) final;

    bool returnToComparisonState(ARM::Functor& func);

    std::pair<ARM::Register, ARM::Register> comparisonRegisters();

    bool stackValueIsKnown(int index);

    int knownStackValue(int index);

    void setKnownStackValue(int index, int value);

    bool registerValueIsKnown(ARM::Register reg);

    int knownRegisterValue(ARM::Register reg);

    /// If register write elimination is disabled we must immediately issue the push instruction
    void setKnownRegisterValue(ARM::Functor& func, ARM::Register reg, int value);

    void commitRegisterValue(ARM::Functor& func, int stackOffset);

    void commitRegisterValue(ARM::Functor& func, ARM::Register reg);
};
}
