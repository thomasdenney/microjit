#include "RegisterFileStateCOWAllocator.h"

#include "Compiler.h"

namespace JIT {

RegisterFileStateCOWAllocator::RegisterFileStateCOWAllocator()
    : m_registerIsInUse{ false } // Remainder initialised to false anyway
    , m_readRegisterForOffset{ StackTopRegister } // Remainder initialised to r0
    , m_writeRegisterForOffset{ StackTopRegister } // Remainder initialised to r0
    , m_topOfStackOffsetFromStackPointer(0)
    , m_numberOfRegistersHoldingValues(1)
    , m_comparisonRegister1(ARM::Register::r0)
    , m_comparisonRegister2(ARM::Register::r0)
    , m_knowRegisterValue{ false }
    , m_registerValues{ 0 }
{
    redetermineRegistersInUse();
}

void RegisterFileStateCOWAllocator::redetermineRegistersInUse()
{
    bool hasReader[REGISTER_COUNT];
    bool hasWriter[REGISTER_COUNT];

    for (int i = 0; i < REGISTER_COUNT; ++i) {
        m_registerIsInUse[i] = false;
        hasReader[i] = false;
        hasWriter[i] = false;
    }

    for (int i = 0; i < m_numberOfRegistersHoldingValues; ++i) {
        hasReader[(int)m_readRegisterForOffset[i]] = true;
        hasWriter[(int)m_writeRegisterForOffset[i]] = true;
    }

    for (int i = 0; i < REGISTER_COUNT; ++i) {
        if (hasReader[i] && !hasWriter[i]) {
            // If a register has a reader but no writer then we find one of its readers and move it to write here
            for (int j = 0; j < m_numberOfRegistersHoldingValues; ++j) {
                if ((int)readRegister(j) == i) {
                    hasWriter[(int)m_writeRegisterForOffset[j]] = false;
                    hasWriter[i] = true;
                    m_writeRegisterForOffset[j] = (ARM::Register)i;
                    break;
                }
            }
        }
    }

    for (int i = 0; i < REGISTER_COUNT; ++i) {
        m_registerIsInUse[i] = hasReader[i] || hasWriter[i];
    }

    int k = 0;
    for (int i = 0; i < REGISTER_COUNT; ++i) {
        if (m_registerIsInUse[i]) {
            k++;
        }
    }

    if (k > m_numberOfRegistersHoldingValues) {
        printf("%d registers are in use, but %d should be:\n", k, m_numberOfRegistersHoldingValues);
        printStatus();
    }
}

bool RegisterFileStateCOWAllocator::inNaiveState()
{
    return m_numberOfRegistersHoldingValues == 1 && m_topOfStackOffsetFromStackPointer == 0 && m_registerIsInUse[(int)StackTopRegister] && m_readRegisterForOffset[0] == StackTopRegister && m_writeRegisterForOffset[0] == StackTopRegister && !m_knowRegisterValue[(int)StackTopRegister];
}

ARM::Register RegisterFileStateCOWAllocator::nextFreeRegister()
{
    for (int i = 0; i < NumberOfRegistersAvailableForStack; ++i) {
        if (!m_registerIsInUse[(int)StackRegisters[i]]) {
            return StackRegisters[i];
        }
    }
    printf("WARNING: Couldn't allocate a register\n");
    printStatus();
    // Deliberately garbage (if this is written to then it will overwrite the state pointer)
    return ARM::Register::r0;
}

bool RegisterFileStateCOWAllocator::ensureRegistersHoldValues(int n, ARM::Functor& func)
{
    if (n > NumberOfRegistersAvailableForStack) {
        return false;
    }
    while (m_numberOfRegistersHoldingValues < n) {
        auto nextRegister = nextFreeRegister();
        m_readRegisterForOffset[m_numberOfRegistersHoldingValues] = nextRegister;
        m_writeRegisterForOffset[m_numberOfRegistersHoldingValues] = nextRegister;
        m_registerIsInUse[(int)nextRegister] = true;

        // This is important: as soon as we've loaded a value from memory we don't know what it is
        m_knowRegisterValue[(int)nextRegister] = false;

        auto offset = m_topOfStackOffsetFromStackPointer + m_numberOfRegistersHoldingValues;
        ++m_numberOfRegistersHoldingValues;
        func.add(ARM::loadWordWithOffset(nextRegister, StackPointerRegister, offset));
    }
    return true;
}

ARM::Register RegisterFileStateCOWAllocator::topOfStackWriteBackRegister()
{
    m_readRegisterForOffset[0] = m_writeRegisterForOffset[0];
    redetermineRegistersInUse();
    return m_writeRegisterForOffset[0];
}

ARM::Register RegisterFileStateCOWAllocator::readRegister(int n)
{
    // This can be different depending on if we are trying to read or write
    return m_readRegisterForOffset[n];
}

ARM::Register RegisterFileStateCOWAllocator::pop()
{
    auto previousTopRead = readRegister(0);
    --m_numberOfRegistersHoldingValues;

    for (int i = 0; i < m_numberOfRegistersHoldingValues; ++i) {
        m_readRegisterForOffset[i] = m_readRegisterForOffset[i + 1];
        m_writeRegisterForOffset[i] = m_writeRegisterForOffset[i + 1];
    }
    redetermineRegistersInUse();

    m_topOfStackOffsetFromStackPointer++;
    return previousTopRead;
}

ARM::Register RegisterFileStateCOWAllocator::push(ARM::Functor& func)
{
    if (m_numberOfRegistersHoldingValues == NumberOfRegistersAvailableForStack) {
        returnToNaiveState(func, m_numberOfRegistersHoldingValues - 1);
        --m_numberOfRegistersHoldingValues;
        redetermineRegistersInUse();
    }

    auto reg = nextFreeRegister();
    for (int i = m_numberOfRegistersHoldingValues; i > 0; --i) {
        m_readRegisterForOffset[i] = m_readRegisterForOffset[i - 1];
        m_writeRegisterForOffset[i] = m_writeRegisterForOffset[i - 1];
    }

    m_knowRegisterValue[(int)reg] = false;
    m_readRegisterForOffset[0] = reg;
    m_writeRegisterForOffset[0] = reg;

    ++m_numberOfRegistersHoldingValues;
    --m_topOfStackOffsetFromStackPointer;

    redetermineRegistersInUse();

    return reg;
}

void RegisterFileStateCOWAllocator::commitRegisterValue(ARM::Functor& func, ARM::Register writeRegister)
{
    if (registerValueIsKnown(writeRegister)) {
        compileLoadConstant(func, knownRegisterValue(writeRegister), writeRegister);
        m_knowRegisterValue[(int)writeRegister] = false;
    }
}

void RegisterFileStateCOWAllocator::commitRegisterValue(ARM::Functor& func, int stackOffset)
{
    auto writeRegister = m_writeRegisterForOffset[stackOffset];
    if (registerValueIsKnown(writeRegister)) {
        commitRegisterValue(func, writeRegister);
        m_readRegisterForOffset[stackOffset] = writeRegister;
    }
}

bool RegisterFileStateCOWAllocator::returnToNaiveState(ARM::Functor& func)
{
    return returnToNaiveState(func, 1);
}

bool RegisterFileStateCOWAllocator::returnToNaiveState(ARM::Functor& func, int offset)
{
    /*
    Currently the implementation of this method is the same as the implementation in the 
    default allocator but in this class we can track whether or not a particular stack
    value was ever written to, and if it wasn't (i.e. we just loaded it to a register to get its
    value) then there is absolutely no need to write it back to memory
    */
    if (inNaiveState()) {
        return true;
    }
    // Restore invariant for stack pointer register
    if (m_topOfStackOffsetFromStackPointer > 0) {
        func.add(ARM::addLargeImm(StackPointerRegister, m_topOfStackOffsetFromStackPointer * 4));
    } else if (m_topOfStackOffsetFromStackPointer < 0) {
        func.add(ARM::subLargeImm(StackPointerRegister, -m_topOfStackOffsetFromStackPointer * 4));
    }

    resetMemoryInvariant(func, offset, 0);

    if (offset == 1) {
        commitRegisterValue(func, 0);
    }

    // Restore invariant for the stack top register
    if (offset == 1 && (m_writeRegisterForOffset[0] != StackTopRegister || m_readRegisterForOffset[0] != StackTopRegister)) {
        func.add(ARM::moveLowToLow(StackTopRegister, readRegister(0)));
        m_readRegisterForOffset[0] = StackTopRegister;
        m_writeRegisterForOffset[0] = StackTopRegister;
    }

    // Restore this data structure to the 'naive' state
    m_topOfStackOffsetFromStackPointer = 0;

    // We only restore this particular part of the invariant if we are doing a full reset
    if (offset == 1) {
        m_numberOfRegistersHoldingValues = 1;
    }

    redetermineRegistersInUse();

    return true;
}

void RegisterFileStateCOWAllocator::resetMemoryInvariant(ARM::Functor& func, int offset, int delta)
{
    // This loop is split into two stages that 1) write values back to memory and 2) ensure that we
    // free a register
    for (int i = offset; i < m_numberOfRegistersHoldingValues; i++) {
        //  1. Iterate through values that we no longer want to keep in registers and
        //     write them to memory. In order to do this we may have to commit a value that is a
        //     compile time constant, hence the call to the |commitRegisterValue|
        commitRegisterValue(func, i);
        func.add(ARM::storeWordWithOffset(readRegister(i), StackPointerRegister, i + delta));

        //  2. Values that are still on the stack might actually point to the same read register as
        //     the value we just wrote to the stack (with a different write register). Therefore to
        //     ensure that this function actually frees a register we move one of other stack values
        //     to write to this register instead. This will then mean that any readers of this register
        //     are unaffected
        for (int j = offset - 1; j >= 0; j--) {
            if (readRegister(j) == readRegister(i)) {
                m_writeRegisterForOffset[j] = m_writeRegisterForOffset[i];
                break;
            }
        }
    }
}

bool RegisterFileStateCOWAllocator::dupTopOfStack(ARM::Functor& func)
{
    if (!ensureRegistersHoldValues(1, func)) {
        return false;
    }
    auto writeRegister = push(func); // Allocates a new register, but we want to continue reading
    m_readRegisterForOffset[0] = readRegister(1);
    m_knowRegisterValue[(int)writeRegister] = registerValueIsKnown(readRegister(1));
    m_registerValues[(int)writeRegister] = knownRegisterValue(readRegister(1));
    return true;
}

bool RegisterFileStateCOWAllocator::dropTopOfStack(ARM::Functor& func)
{
    // I don't think that I need to worry about known register values here
    if (!ensureRegistersHoldValues(2, func)) {
        return false;
    }
    pop();
    return true;
}

bool RegisterFileStateCOWAllocator::rot(ARM::Functor& func)
{
    if (!ensureRegistersHoldValues(3, func)) {
        return false;
    }

    auto rA = m_readRegisterForOffset[0];
    auto rB = m_readRegisterForOffset[1];
    auto rC = m_readRegisterForOffset[2];

    auto wA = m_writeRegisterForOffset[0];
    auto wB = m_writeRegisterForOffset[1];
    auto wC = m_writeRegisterForOffset[2];

    m_readRegisterForOffset[0] = rC;
    m_readRegisterForOffset[1] = rA;
    m_readRegisterForOffset[2] = rB;

    m_writeRegisterForOffset[0] = wC;
    m_writeRegisterForOffset[1] = wA;
    m_writeRegisterForOffset[2] = wB;

    return true;
}

bool RegisterFileStateCOWAllocator::swap(ARM::Functor& func)
{
    if (!ensureRegistersHoldValues(2, func)) {
        return false;
    }

    auto s = m_readRegisterForOffset[0];
    m_readRegisterForOffset[0] = m_readRegisterForOffset[1];
    m_readRegisterForOffset[1] = s;

    s = m_writeRegisterForOffset[0];
    m_writeRegisterForOffset[0] = m_writeRegisterForOffset[1];
    m_writeRegisterForOffset[1] = s;

    return true;
}

bool RegisterFileStateCOWAllocator::tuck(ARM::Functor& func)
{
    return ntuck(func, 3);
}

bool RegisterFileStateCOWAllocator::ntuck(ARM::Functor& func, int n)
{
    if (n < 0 || n > 4) {
        return false;
    }
    if (!ensureRegistersHoldValues(n, func)) {
        return false;
    }

    auto topRead = m_readRegisterForOffset[0];
    auto topWrite = m_writeRegisterForOffset[0];

    for (int i = 0; i < n - 1; ++i) {
        m_readRegisterForOffset[i] = m_readRegisterForOffset[i + 1];
        m_writeRegisterForOffset[i] = m_writeRegisterForOffset[i + 1];
    }

    m_readRegisterForOffset[n - 1] = topRead;
    m_writeRegisterForOffset[n - 1] = topWrite;

    return true;
}

bool RegisterFileStateCOWAllocator::returnToComparisonState(ARM::Functor& func)
{
    // Need 3 so that something is in stack top register
    if (!ensureRegistersHoldValues(3, func)) {
        return false;
    }

    // Pop 2
    m_topOfStackOffsetFromStackPointer += 2;

    // Restore invariant for stack pointer register
    if (m_topOfStackOffsetFromStackPointer > 0) {
        func.add(ARM::addLargeImm(StackPointerRegister, m_topOfStackOffsetFromStackPointer * 4));
    } else if (m_topOfStackOffsetFromStackPointer < 0) {
        func.add(ARM::subLargeImm(StackPointerRegister, -m_topOfStackOffsetFromStackPointer * 4));
    }

    // For reset to invariant
    m_topOfStackOffsetFromStackPointer = 0;

    resetMemoryInvariant(func, 3, -2);

    // At this point if we had more than three values in registers then they've been written back
    // to the stack. We now have several cases to consider:

    //      Let the new top of stack be top = m_readRegisterForOffset[2]
    //      Let the 1st compatator be     a = m_readRegisterForOffset[1]
    //      Let the 2nd comparator be     b = m_readRegisterForOffset[0]

    //      The operators are flipped because we do a OP b where the stack is ab (bottom to top)

    //      1. top == StackTopRegister -- do nothing
    //      2. Neither a == StackTopRegister nor b == StackTopRegister -- copy top to StackTopRegister
    //      3. Either a == StackTopRegister or b == StackTopRegister -- then we have at least
    //         one spare register c in { r4, r5, r6, r7 }. We copy a or b to c and update the relevant
    //         one (if it is both then update both). Then copy top to StackTopRegister

    //      Finally set m_comparisonRegister1 to a and m_comparisonRegister2 to b and restore
    //      the remainder of the naive state invariant

    //      Potential TODO: If m_comparisonRegister1 == m_comparisonRegister2 then eliminate the
    //      comparison and jump

    // Another TODO: If we know register values then we can eliminate the comparison
    for (int i = 0; i < 3; ++i) {
        commitRegisterValue(func, i);
    }

    auto top = m_readRegisterForOffset[2];
    auto a = m_readRegisterForOffset[1];
    auto b = m_readRegisterForOffset[0];

    if (top != StackTopRegister) {
        if (a != StackTopRegister && b != StackTopRegister) {
            func.add(ARM::moveLowToLow(StackTopRegister, top));
        } else if (a == StackTopRegister || b == StackTopRegister) {
            // This case is always true if the if case isn't; I've left it as else if for clarity
            int i;
            for (i = 1; i < NumberOfRegistersAvailableForStack; ++i) {
                if (StackRegisters[i] != top && StackRegisters[i] != a && StackRegisters[i] != b) {
                    break;
                }
            }
            auto c = StackRegisters[i];
            if (a == StackTopRegister && b == StackTopRegister) {
                func.add(ARM::moveLowToLow(c, a));
                a = b = c;
            } else if (a == StackTopRegister) {
                func.add(ARM::moveLowToLow(c, a));
                a = c;
            } else if (b == StackTopRegister) {
                func.add(ARM::moveLowToLow(c, b));
                b = c;
            }
            func.add(ARM::moveLowToLow(StackTopRegister, top));
        }
    }

    m_comparisonRegister1 = a;
    m_comparisonRegister2 = b;

    m_numberOfRegistersHoldingValues = 1;
    m_readRegisterForOffset[0] = m_writeRegisterForOffset[0] = StackTopRegister;
    redetermineRegistersInUse();

    return true;
}

std::pair<ARM::Register, ARM::Register> RegisterFileStateCOWAllocator::comparisonRegisters()
{
    return std::pair<ARM::Register, ARM::Register>(m_comparisonRegister1, m_comparisonRegister2);
}

bool RegisterFileStateCOWAllocator::registerValueIsKnown(ARM::Register reg)
{
    if (!RegisterWriteElimination) {
        return false;
    }
    return m_knowRegisterValue[(int)reg];
}

int RegisterFileStateCOWAllocator::knownRegisterValue(ARM::Register reg)
{
    return m_registerValues[(int)reg];
}

void RegisterFileStateCOWAllocator::setKnownRegisterValue(ARM::Functor& func, ARM::Register reg, int value)
{
    if (!RegisterWriteElimination) {
        compileLoadConstant(func, value, reg);
        return;
    }
    m_knowRegisterValue[(int)reg] = true;
    m_registerValues[(int)reg] = value;
}

void RegisterFileStateCOWAllocator::printStatus()
{
    printf("COW allocator has %d stack values in registers:\n", m_numberOfRegistersHoldingValues);
    printf("  ");
    for (int i = 0; i < m_numberOfRegistersHoldingValues; ++i) {
        printf("%x   ", i);
    }
    printf("\n");
    printf("R ");
    for (int i = 0; i < m_numberOfRegistersHoldingValues; ++i) {
        printf("r%02d ", m_readRegisterForOffset[i]);
    }
    printf("\n");
    printf("W ");
    for (int i = 0; i < m_numberOfRegistersHoldingValues; ++i) {
        printf("r%02d ", m_writeRegisterForOffset[i]);
    }
    printf("\n");
    printf("Registers in use = {");
    bool first = true;
    for (int i = 0; i < REGISTER_COUNT; ++i) {
        if (m_registerIsInUse[i]) {
            if (first) {
                first = false;
            } else {
                printf(",");
            }
            printf(" r%02d", i);
            if (m_knowRegisterValue[i]) {
                printf(" = %d", m_registerValues[i]);
            }
        }
    }
    printf(" }\n");
}
}