#include "ConditionalBranch.h"

#include "Compiler.h"

namespace JIT {

size_t ConditionalBranch::instructionCount()
{
    return 6;
}

bool ConditionalBranch::linkStackCode(ARM::Functor& func, size_t realDestinationOffset)
{
    size_t i = insertionOffset();
    uint16_t* buffer = func.buffer();

    buffer[i] = ARM::moveLowToLow(TempRegister, StackTopRegister);
    buffer[i + 1] = ARM::addSmallImm(StackPointerRegister, StackPointerRegister, 4);
    buffer[i + 2] = ARM::loadWordWithOffset(StackTopRegister, StackPointerRegister, 0);
    buffer[i + 3] = ARM::compareImmediate(TempRegister, 0);

    // -2 because awkward
    int offsetFromFourthInstruction = (int)realDestinationOffset - (i + 4) - 2;
    if (offsetFromFourthInstruction < -128 || offsetFromFourthInstruction > 127) {
        int offsetFromFifthInstruction = (int)realDestinationOffset - (i + 5) - 2;
        if (offsetFromFifthInstruction < -1024 || offsetFromFifthInstruction > 1023) {
            // TODO Come up with a better strategy for handling this case
            return false;
        } else {
            buffer[i + 4] = ARM::conditionalBranch(ARM::Condition::eq, 0);
            buffer[i + 5] = ARM::unconditionalBranch(offsetFromFifthInstruction);
        }
    } else {
        buffer[i + 4] = ARM::conditionalBranch(ARM::Condition::ne, offsetFromFourthInstruction);
        buffer[i + 5] = ARM::nop();
    }
    return true;
}
}