#include "BlockStackEffect.h"

#include "InstructionStackEffect.h"
#include <algorithm>

namespace Code {

BlockStackEffect::BlockStackEffect(Code::Iterator iter)
    : BlockStackEffect()
{
    for (; !iter.finished(); ++iter) {
        InstructionStackEffect iEffect(iter.instruction());

        if (iter.instruction() == Instruction::Ndup) {
            // The height doesn't change as a result of this instruction, but we do need to access at least one
            // element from the stack
            if (iter.lastWasPush()) {
                m_popCount = std::max(m_popCount, m_heightDifference + iter.pushValue());
            } else {
                m_deterministicPops = false;
                m_popCount = std::max(m_popCount, m_heightDifference + 1);
            }
        } else if (iter.instruction() == Instruction::Ntuck || iter.instruction() == Instruction::Nrot) {
            if (iter.lastWasPush()) {
                m_popCount = std::max(m_popCount, m_heightDifference + iter.pushValue());
            } else {
                m_deterministicPops = false;
                m_popCount = std::max(m_popCount, m_heightDifference + 1);
            }
            m_heightDifference++;
        } else {
            int popCount = iEffect.popCount();
            int pushCount = iEffect.pushCount();
            if (iter.currentIsOptional()) {
                popCount = iter.optionalPopCount();
                pushCount = iter.optionalPushCount();
            } else if (!iEffect.deterministicPops()) {
                // Shouldn't ever occur
                m_deterministicPops = false;
            }
            m_heightDifference += popCount;
            m_popCount = std::max(m_popCount, m_heightDifference);
            m_heightDifference -= pushCount;
            m_pushCount = std::max(m_pushCount, -m_heightDifference);
        }
    }
}

bool BlockStackEffect::supersedes(const BlockStackEffect& anotherBlock)
{
    return anotherBlock.popCount() <= (popCount() - heightDifference()) && anotherBlock.pushCount() <= (pushCount() + heightDifference());
}
}