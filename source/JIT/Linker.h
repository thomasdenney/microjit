#pragma once

#include "Config.h"

#include "ARM/Functor.h"
#include "Code/Array.h"
#include "Code/Region.h"
#include "LinkOperation.h"
#include "SpecialLinkerLocations.h"
#include "StaticAnalysis.h"
#include <map>
#include <memory>
#include <vector>

namespace JIT {

class Linker {
private:
    std::vector<std::unique_ptr<LinkOperation>> m_linkOperations;
    std::map<size_t, size_t> m_linkLocations;
    SpecialLinkerLocations m_specialLocations;

    void addOperation(ARM::Functor& func, std::unique_ptr<LinkOperation> operation);

public:
    Linker()
        : m_linkOperations()
    {
    }

    bool link(ARM::Functor& functor, StaticAnalysis& analysis);

    void addUnconditionalJump(ARM::Functor& func, size_t offset, int skipCount);
    void addConditionalJump(ARM::Functor& func, size_t offset, int skipCount);
    void addMinimalBranchConditionalJump(ARM::Functor& func, size_t offset, int skipCount, ARM::Condition condition, ARM::Register cmp1, ARM::Register cmp2);
    void addCall(ARM::Functor& func, size_t offset);

    void addHalt(ARM::Functor& func);
    void addStackOverflowCheck(ARM::Functor& func);
    void addStackUnderflowCheck(ARM::Functor& func);

    /// Set the offset to jump to when halting
    void setHaltOffset(size_t offset);
    /// Set the offset to jump to when stack underflow
    void setStackUnderflowOffset(size_t offset);
    /// Set the offset to jump to when stack overflow
    void setStackOverflowOffset(size_t offset);

    /// Should only be used for basic block heads
    void setLinkOffset(size_t stackCodeOffset, size_t bytecodeOffset);

    bool hasOffsetForBasicBlock(size_t offset) const;
    size_t offsetForBasicBlock(size_t offset) const;

    /// After static analysis is complete you should clear the list of jobs
    void clear();

    void serialise();
    void deserialise();
};
}