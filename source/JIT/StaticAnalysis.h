#pragma once

#include "Config.h"

#include "Code/Array.h"
#include "Code/BlockStackEffect.h"
#include "Code/Region.h"
#include "InstructionMetadata.h"
#include <cstdint>
#include <cstdio>
#include <vector>

namespace JIT {

/**
 * Performs core static analysis of Stack code to categorise regions of code
 */
class StaticAnalysis {
public:
    enum class Status : uint32_t {
        UnknownFailure,
        Success,
        CodeOverlapsWithIllegalInstruction,
        BasicBlockStartNotCode,
        InvalidWidth,
        FunctionStartNotTreatedAsBasicBlock,
        VariableJumpNotAllowed,
        IllegalJump,
        IllegalCall
    };
    static const char* statusString(Status status);

    StaticAnalysis(Code::Array source)
        : m_source(source)
        , m_metadata(source.length(), InstructionMetadata::Nothing)
        , m_functionRegions(0)
        , m_newFunctionRegions(0)
        , m_hasHalts(false)
        , m_hasDynamicCalls(false)
    {
    }

    Status analyse();

    /**
     * Begins the analysis from a specific offset. If analysis has already been performed on a function/basic block
     * then it is skipped. If a contradiction is reached (e.g. jumping/calling what was previously determined to be an
     * illegal location) then static analysis will termiante as normal.
     *
     * Currently this is O(n) in the length of the code, not just the new instructions that are analysed for the first
     * time, which is a little annoying. Therefore at the moment it is best to avoid having lots of dynamic calls
     * because the first time each one is called the *whole* of the code needs to be rechecked.
     */
    Status analyse(size_t offset);

    Code::Region codeRegion() const { return m_codeRegion; }
    Code::Region dataRegion() const { return m_dataRegion; }

    std::vector<Code::Region>& functionRegions() { return m_functionRegions; }
    std::vector<Code::Region>& newFunctionRegions() { return m_newFunctionRegions; }

    std::vector<Code::Region> basicBlocksForFunction(Code::Region functionRegion) const;
    Code::Region basicBlockAtIndex(int index) const;

    Code::BlockStackEffect stackEffectForBasicBlock(Code::Region basicBlock) const;

    bool hasHalts() const { return m_hasHalts; }
    bool hasDynamicCalls() const { return m_hasDynamicCalls; }

    /**
     * Returns negative value for the first instruction. The result should be ignored if
     * this is also the start of a basic block
     */
    int previousInstructionIndex(int offset) const;

    bool isCallDestination(size_t i) const { return (bool)(m_metadata[i] & InstructionMetadata::FunctionStart); }
    bool isJumpDestination(size_t i) const { return (bool)(m_metadata[i] & InstructionMetadata::BasicBlockStart); }
    bool isCallOrJumpDestination(size_t i) const { return isCallDestination(i) || isJumpDestination(i); }

    /**
     * NOTE: Under the current compilation approach this actually just means needs to push ONE register, i.e. the LR
     */
    bool functionNeedsToPushRegisters(size_t i) const;

    void printStaticAnalyis() const;

    void serialise();
    void deserialise();

private:
    Code::Array m_source;

    Code::Region m_codeRegion, m_dataRegion;
    std::vector<InstructionMetadata> m_metadata;
    std::vector<Code::Region> m_functionRegions;
    std::vector<Code::Region> m_newFunctionRegions;

    bool m_hasHalts;
    bool m_hasDynamicCalls;

    Status determineCallLocations(size_t offset);

    /**
     * Currently this method is what causes static analysis to always be O(n) in the length of the code :( as per above
     */
    Status verifyInstructionMetadata() const;

    void dumpRegions() const;
};
}