#pragma once

#include "Config.h"

#include "Array.h"
#include "Instruction.h"
#include "Region.h"

/**
 * This should not be less than three in the current implementation
 * Increasing this will significantly damage the performance of the iterator
 * (it is designed to support a small number of *very* small lookback operations)
 */
#define NUMBER_OF_INDICIES_TO_TRACK 3

namespace Code {

/**
 * Iterates through the code and automatically parses PUSH instructions that affect the stack.
 * 
 * Push instructions are *not* skipped by this iterator
 */
class Iterator {
public:
    Iterator(const Array& code)
        : Iterator(code, code.region())
    {
    }

    Iterator(const Array& code, const Region& region)
        : m_code(code.m_code)
        , m_region(region)
    {
        for (int i = 0; i < NUMBER_OF_INDICIES_TO_TRACK; ++i) {
            m_lastIndicies[i] = region.start();
        }
    }

    Region region() const;

    bool hasMoreInstructions() const;
    bool finished() const;
    size_t index() const;

    size_t nextIndex() const;
    size_t lastIndex() const;

    size_t nPreviousIndex(int n) const;

    Iterator& operator++();

    bool currentIsOptional() const;

    size_t optionalPopCount() const;
    size_t optionalPushCount() const;

    Instruction instruction() const;

    /**
     * Do bounds check with |hasMoreInstructions| before calling this function
     */
    Instruction nextInstruction() const;

    bool currentIsPush() const;
    bool currentIsSafePush() const;
    bool lastWasPush() const;
    bool twoPrevWasConditionalCheck() const;

    /**
     * If the current instruction is a push then this will yield the value pushed, but if the current is not a push then
     * it will yield the most recent value pushed
     */
    int pushValue() const;

    /**
     * If the current instruction is a conditional check then usually the next instruction will be
     * some sort of push followed by a conditional jump
     */
    bool nextArePushAndConditionalJump() const;

private:
    Instruction* m_code;
    Region m_region;
    size_t m_lastIndicies[NUMBER_OF_INDICIES_TO_TRACK];
};
}