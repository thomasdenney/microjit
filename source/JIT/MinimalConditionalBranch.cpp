#include "MinimalConditionalBranch.h"

#include "Compiler.h"

namespace JIT {

size_t MinimalConditionalBranch::instructionCount()
{
    return 3;
}

bool MinimalConditionalBranch::linkStackCode(ARM::Functor& func, size_t realDestinationOffset)
{
    size_t i = insertionOffset();
    uint16_t* buffer = func.buffer();

    buffer[i] = ARM::compareLowRegisters(m_operand1, m_operand2);

    // -2 because awkward
    int offsetFromFirstInstruction = (int)realDestinationOffset - (i + 1) - 2;
    if (offsetFromFirstInstruction < -128 || offsetFromFirstInstruction > 127) {
        int offsetFromSecondInstruction = (int)realDestinationOffset - (i + 2) - 2;
        if (offsetFromSecondInstruction < -1024 || offsetFromSecondInstruction > 1023) {
            // TODO Come up with a better strategy for handling this case
            return false;
        } else {
            buffer[i + 1] = ARM::conditionalBranch(ARM::InvertCondition(m_condition), 0);
            buffer[i + 2] = ARM::unconditionalBranch(offsetFromSecondInstruction);
        }
    } else {
        buffer[i + 1] = ARM::conditionalBranch(m_condition, offsetFromFirstInstruction);
    }
    return true;
}
}