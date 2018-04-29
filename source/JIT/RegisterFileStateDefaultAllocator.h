#pragma once

#include "Config.h"
#include "RegisterFileState.h"

namespace JIT {

class RegisterFileStateDefaultAllocator : public RegisterFileState {
private:
    /**
     * If this is true then we fill up the registers like
     *  r2 = stack.peek(0), r4 = stack.peek(1), r5 = stack.peek(2), etc
     * 
     * Otherwise we fill the registers downwards, so the top of stack is instead at a high register value
     * 
     * This should flip 
     */
    bool m_up;

    bool returnToNaiveState(ARM::Functor& func, int offset);

    /**
     * Initially this should be zero because the top of stack is in the top of stack register
     */
    int32_t m_topOfStackOffsetFromStackPointer;

    /**
     * Initially this should be 1 because the top of stack is in r2
     * 
     * This shouldn't ever be less than 1 because the stack underflow invariant is ensured before this code is ever
     * executed
     */
    int32_t m_numberOfRegistersHoldingValues;

    /**
     */
    int32_t m_startRegister;

public:
    RegisterFileStateDefaultAllocator();

    bool inNaiveState() final;

    bool ensureRegistersHoldValues(int n, ARM::Functor& func) final;

    ARM::Register topOfStackWriteBackRegister() final;

    ARM::Register readRegister(int n) final;

    ARM::Register pop() final;

    ARM::Register push(ARM::Functor& func) final;

    bool returnToNaiveState(ARM::Functor& func) final;

    bool dupTopOfStack(ARM::Functor& func) final;

    bool dropTopOfStack(ARM::Functor& func) final;

    bool rot(ARM::Functor& func) final;

    bool swap(ARM::Functor& func) final;

    bool tuck(ARM::Functor& func) final;

    /// Unimplemented
    bool returnToComparisonState(ARM::Functor& func);

    /// Unimplemented
    std::pair<ARM::Register, ARM::Register> comparisonRegisters();

    /// Unsupported
    bool registerValueIsKnown(ARM::Register reg);

    /// Unsupported
    int knownRegisterValue(ARM::Register reg);

    /// Unsupported
    void setKnownRegisterValue(ARM::Functor& func, ARM::Register reg, int value);

    void commitRegisterValue(ARM::Functor& func, int stackOffset);
    void commitRegisterValue(ARM::Functor& func, ARM::Register reg);
};
}