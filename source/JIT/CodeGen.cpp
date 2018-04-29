#include "CodeGen.h"

#include "Bit/Bit.h"
#include "Config.h"
#include "DynamicCompilation.h"

namespace JIT {

const ARM::Register StatePointerRegister = ARM::Register::r0;
const ARM::Register StackPointerRegister = ARM::Register::r1;
const ARM::Register StackTopRegister = ARM::Register::r2;
const ARM::Register TempRegister = ARM::Register::r3;
const ARM::Register TempRegister2 = ARM::Register::r4;
const ARM::Register TempRegister3 = ARM::Register::r5;
const ARM::Register StackBaseRegister = ARM::Register::r8;
const ARM::Register StackEndRegister = ARM::Register::r9;

void compileLoadConstant(ARM::Functor& func, int value, ARM::Register destination)
{
    const bool isNegative = value < 0;
    if (isNegative) {
        value = -value;
    }

    uint32_t absValue = (uint32_t)value;

    bool first = true;
    for (int b = 3; b >= 0; --b) {
        auto byte = Bit::uintRegion(absValue, 8 * b, 8);
        if (byte > 0 || (byte == 0 && b == 0)) {
            if (first) {
                func.add(ARM::moveImmediate(destination, byte));
                first = false;
            } else if (byte > 0) {
                func.add(ARM::addLargeImm(destination, byte));
            }
            if (b != 0) {
                func.add(ARM::logicalShiftLeftImmediate(destination, destination, 8));
            }
        } else {
            if (!first && b != 0) {
                func.add(ARM::logicalShiftLeftImmediate(destination, destination, 8));
            }
        }
    }

    if (isNegative) {
        func.add(ARM::neg(destination, destination));
    }
}

void compileCFunctionCall(ARM::Functor& func, Environment::VMFunction destination, bool needsToRestoreInvariant)
{
    bool startWasAlignedTo4ByteBoundary = ((int)&func.buffer()[func.length()]) % 4 == 0;
    int offset = 1;
    if (needsToRestoreInvariant) {
        offset += 1;
    }

    // The PC will be 2 instrutions after this, so either the first load or the unconditional branch
    // The offset is in multiples of 4
    func.add(ARM::loadWordWithPCOffset(TempRegister, offset));
    func.add(ARM::branchLinkExchangeToRegister(TempRegister));

    if (needsToRestoreInvariant) {
        func.add(ARM::loadWordWithOffset(StackPointerRegister, StatePointerRegister, offsetof(Environment::VM, m_stack.m_stackPointer) / sizeof(int32_t*)));
        func.add(ARM::loadWordWithOffset(StackTopRegister, StackPointerRegister, 0));
    }

    if (startWasAlignedTo4ByteBoundary) {
        func.add(ARM::unconditionalBranch(2));
        func.add(ARM::nop());
    } else {
        func.add(ARM::unconditionalBranch(1));
    }

    func.addData((int)destination);
}

void compileWriteStateToMemory(ARM::Functor& func)
{
    // We only want to store the top of stack if the stack is not empty
    func.add(ARM::compareRegistersGeneral(StackEndRegister, StackPointerRegister));
    func.add(ARM::conditionalBranch(ARM::Condition::le, 2)); // end <= stack, i.e. stack underflow

    func.add(ARM::compareRegistersGeneral(StackBaseRegister, StackPointerRegister));
    func.add(ARM::conditionalBranch(ARM::Condition::gt, 0)); // base > stack, i.e. stack overflow
    // Actually do the store
    func.add(ARM::storeWordWithOffset(StackTopRegister, StackPointerRegister, 0));
    // The first value in the VM state struct should always be the stack pointer
    func.add(ARM::storeWordWithOffset(StackPointerRegister, StatePointerRegister, offsetof(Environment::VM, m_stack.m_stackPointer) / sizeof(int32_t*)));
}
}