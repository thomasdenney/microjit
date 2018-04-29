#pragma once

#include "Config.h"

#include "Instruction.h"
#include "Iterator.h"

namespace Code {

/**
 * See also InstructionStackEffect. These are deliberately different classes because they have different semantics
 */
class BlockStackEffect {
private:
    bool m_deterministicPops;
    int m_popCount, m_pushCount;
    int m_heightDifference;

    void resolveFromInstruction(Instruction instr);

    BlockStackEffect()
        : m_deterministicPops(true)
        , m_popCount(0)
        , m_pushCount(0)
        , m_heightDifference(0)
    {
    }

public:
    BlockStackEffect(Code::Iterator iter);

    /**
     * Indicates that there are NTUCK, NDUP, or NROT instructions for which we could not determine the number of values
     * that must be present on the stack already. This doesn't actually matter in practice because in this case a
     * function will be called that does the bounds checks and it is sufficient for us to verify that there is at least
     * one value on the stack.
     */
    bool deterministicPops() const { return m_deterministicPops; }

    int popCount() const { return m_popCount; }
    int pushCount() const { return m_pushCount; }

    /**
     * Offset from a descending stack pointer, so a positive height difference indicates that values are popped from the
     * stack whilst a negative height difference indicates that values are pushed to the stack.
     */
    int heightDifference() const { return m_heightDifference; }

    /**
     * Can be used to determine if the stack check for |anotherBlock| can be eliminated because we know that we're in
     * this block
     */
    bool supersedes(const BlockStackEffect& anotherBlock);
};
}