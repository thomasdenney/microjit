#include "RegisterFileStateDefaultAllocator.h"
#include "Compiler.h"

namespace JIT {

RegisterFileStateDefaultAllocator::RegisterFileStateDefaultAllocator()
    : m_topOfStackOffsetFromStackPointer(0)
    , m_numberOfRegistersHoldingValues(1) // What if the stack is empty?
    , m_startRegister(0)
{
}

bool RegisterFileStateDefaultAllocator::inNaiveState()
{
    return m_topOfStackOffsetFromStackPointer == 0 && m_numberOfRegistersHoldingValues == 1;
}

bool RegisterFileStateDefaultAllocator::ensureRegistersHoldValues(int n, ARM::Functor& func)
{
    if (n > NumberOfRegistersAvailableForStack) {
        return false;
    }
    while (m_numberOfRegistersHoldingValues < n) {
        auto nextRegister = StackRegisters[(m_startRegister + m_numberOfRegistersHoldingValues) % NumberOfRegistersAvailableForStack];
        auto offset = m_topOfStackOffsetFromStackPointer + m_numberOfRegistersHoldingValues;
        ++m_numberOfRegistersHoldingValues;
        func.add(ARM::loadWordWithOffset(nextRegister, StackPointerRegister, offset));
    }
    return true;
}

ARM::Register RegisterFileStateDefaultAllocator::topOfStackWriteBackRegister()
{
    // R/W registers are the same in this implementation
    return readRegister(0);
}

ARM::Register RegisterFileStateDefaultAllocator::readRegister(int n)
{
    return StackRegisters[(m_startRegister + n) % NumberOfRegistersAvailableForStack];
}

ARM::Register RegisterFileStateDefaultAllocator::pop()
{
    auto top = readRegister(0);

    m_numberOfRegistersHoldingValues--;
    m_startRegister = (m_startRegister + 1) % NumberOfRegistersAvailableForStack;
    m_topOfStackOffsetFromStackPointer++;

    return top;
}

ARM::Register RegisterFileStateDefaultAllocator::push(ARM::Functor& func)
{
    if (m_numberOfRegistersHoldingValues == 0) {
        // Base state
        m_startRegister = 0;
        m_numberOfRegistersHoldingValues = 1;
    } else if (m_numberOfRegistersHoldingValues == NumberOfRegistersAvailableForStack) {
        // Therefore need to write the last element of the circular buffer to memory
        returnToNaiveState(func, NumberOfRegistersAvailableForStack - 1);
        m_startRegister = (m_startRegister + NumberOfRegistersAvailableForStack - 1) % NumberOfRegistersAvailableForStack;
    } else {
        // Can just fill another register
        m_numberOfRegistersHoldingValues++;
        m_startRegister = (m_startRegister + NumberOfRegistersAvailableForStack - 1) % NumberOfRegistersAvailableForStack;
    }
    m_topOfStackOffsetFromStackPointer--;
    return topOfStackWriteBackRegister();
}

bool RegisterFileStateDefaultAllocator::returnToNaiveState(ARM::Functor& func)
{
    return returnToNaiveState(func, 1);
}

bool RegisterFileStateDefaultAllocator::returnToNaiveState(ARM::Functor& func, int offset)
{
    if (inNaiveState()) {
        return true;
    }
    // Restore invariant for stack pointer register
    if (m_topOfStackOffsetFromStackPointer > 0) {
        func.add(ARM::addLargeImm(StackPointerRegister, m_topOfStackOffsetFromStackPointer * 4));
    } else if (m_topOfStackOffsetFromStackPointer < 0) {
        func.add(ARM::subLargeImm(StackPointerRegister, -m_topOfStackOffsetFromStackPointer * 4));
    }

    // Restore memory invariant. We can skip the top of stack seen as this is always done on Stack exit
    for (int i = offset; i < m_numberOfRegistersHoldingValues; i++) {
        func.add(ARM::storeWordWithOffset(readRegister(i), StackPointerRegister, i));
    }

    // Restore invariant for the stack top register
    if (offset == 1 && topOfStackWriteBackRegister() != StackTopRegister) {
        func.add(ARM::moveLowToLow(StackTopRegister, topOfStackWriteBackRegister()));
    }

    // Restore this data structure to the 'naive' state
    m_topOfStackOffsetFromStackPointer = 0;

    // We only restore this particular part of the invariant if we are doing a full reset
    if (offset == 1) {
        m_numberOfRegistersHoldingValues = 1;
        m_startRegister = 0;
    }

    return true;
}

bool RegisterFileStateDefaultAllocator::dupTopOfStack(ARM::Functor& func)
{
    auto top = topOfStackWriteBackRegister();
    auto dest = push(func);
    func.add(ARM::moveLowToLow(dest, top));
    return true;
}

bool RegisterFileStateDefaultAllocator::dropTopOfStack(ARM::Functor& func)
{
    if (!ensureRegistersHoldValues(2, func)) {
        return false;
    }
    pop();
    return true;
}

bool RegisterFileStateDefaultAllocator::rot(ARM::Functor& func)
{
    if (!ensureRegistersHoldValues(3, func)) {
        return false;
    }
    auto a = readRegister(0);
    auto b = readRegister(1);
    auto c = readRegister(2);
    func.add(ARM::moveLowToLow(TempRegister, c));
    func.add(ARM::moveLowToLow(c, b));
    func.add(ARM::moveLowToLow(b, a));
    func.add(ARM::moveLowToLow(a, TempRegister));
    return true;
}

bool RegisterFileStateDefaultAllocator::swap(ARM::Functor& func)
{
    if (!ensureRegistersHoldValues(2, func)) {
        return false;
    }
    auto a = readRegister(0);
    auto b = readRegister(1);
    func.add(ARM::moveLowToLow(TempRegister, a));
    func.add(ARM::moveLowToLow(a, b));
    func.add(ARM::moveLowToLow(b, TempRegister));
    return true;
}

bool RegisterFileStateDefaultAllocator::tuck(ARM::Functor& func)
{
    if (!ensureRegistersHoldValues(3, func)) {
        return false;
    }
    auto a = readRegister(0);
    auto b = readRegister(1);
    auto c = readRegister(2);
    func.add(ARM::moveLowToLow(TempRegister, a));
    func.add(ARM::moveLowToLow(a, b));
    func.add(ARM::moveLowToLow(b, c));
    func.add(ARM::moveLowToLow(c, TempRegister));
    return true;
}

bool RegisterFileStateDefaultAllocator::returnToComparisonState(ARM::Functor& func)
{
    return false;
}

std::pair<ARM::Register, ARM::Register> RegisterFileStateDefaultAllocator::comparisonRegisters()
{
    printf("WARNING: Attempting to use comparisonRegisters with default allocator (not supported)\n");
    return std::pair<ARM::Register, ARM::Register>(ARM::Register::r0, ARM::Register::r0);
}

bool RegisterFileStateDefaultAllocator::registerValueIsKnown(ARM::Register reg)
{
    return false;
}

int RegisterFileStateDefaultAllocator::knownRegisterValue(ARM::Register reg)
{
    return 0;
}

void RegisterFileStateDefaultAllocator::setKnownRegisterValue(ARM::Functor& func, ARM::Register reg, int value)
{
    compileLoadConstant(func, value, reg);
}

void RegisterFileStateDefaultAllocator::commitRegisterValue(ARM::Functor& func, int stackOffset)
{
}
void RegisterFileStateDefaultAllocator::commitRegisterValue(ARM::Functor& func, ARM::Register reg)
{
}
}