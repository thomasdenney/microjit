#pragma once

#include "Config.h"

#include "Instruction.h"
#include <climits>

namespace Code {

class InstructionStackEffect {
private:
    int m_popCount, m_pushCount;
    static const int UnknownEffect = INT_MAX;

    void resolveFromInstruction(Instruction instr);

public:
    InstructionStackEffect(Instruction instr)
    {
        resolveFromInstruction(instr);
    }

    int popCount() const { return m_popCount; }
    int pushCount() const { return m_pushCount; }
    int heightDifference() const { return popCount() - pushCount(); }
    bool deterministicPops() const { return m_popCount != UnknownEffect; };
};
}