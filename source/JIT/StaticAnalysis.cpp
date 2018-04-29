#include "StaticAnalysis.h"

#include "Bit/Bit.h"
#include "Code/Iterator.h"
#include "InstructionSelection.h"
#include "Queue.h"
#include "Tests/Utilities.h"
#include "Transfer/Deserialiser.h"
#include "Transfer/Serialiser.h"
#include <algorithm>
#include <cstdio>

namespace JIT {

StaticAnalysis::Status StaticAnalysis::analyse()
{
    return analyse(0);
}

StaticAnalysis::Status StaticAnalysis::analyse(size_t offset)
{
    auto callLocRes = determineCallLocations(offset);
    if (callLocRes != Status::Success) {
        return callLocRes;
    }

    auto verifiedLocations = verifyInstructionMetadata();
    if (verifiedLocations != Status::Success) {
        return verifiedLocations;
    } else if (m_functionRegions.size() > 0) {
        // Not strictly necessary but ensures static analysis prints in order
        std::sort(m_functionRegions.begin(), m_functionRegions.end());

        m_codeRegion = m_functionRegions[0];
        for (auto& region : m_functionRegions) {
            m_codeRegion = m_codeRegion.add(region);
        }
    }
    return Status::Success;
}

StaticAnalysis::Status StaticAnalysis::determineCallLocations(size_t offset)
{
    // m_FunctionRegions is global, but this is just for newly discovered functions
    m_newFunctionRegions.clear();

    const auto end = m_source.region().end();
    if (m_source.length() == 0) {
        return Status::Success;
    }

    Queue functionLocations;
    Queue basicBlockLocations;
    functionLocations.push(offset);

    while (!functionLocations.empty()) {
        auto fHead = functionLocations.pop();
        // We have already visited this function
        if (any(m_metadata[fHead] & InstructionMetadata::FunctionStart)) {
            continue;
        }

        m_metadata[fHead] = m_metadata[fHead] | InstructionMetadata::FunctionStart;

        bool functionHasRecursiveCalls = false;

        basicBlockLocations.reset();

        basicBlockLocations.push(fHead);
        size_t fEnd = fHead;

        while (!basicBlockLocations.empty()) {
            auto blockHead = basicBlockLocations.pop();
            if (any(m_metadata[blockHead] & InstructionMetadata::BasicBlockStart)) {
                // Already visited this basic block
                continue;
            }

            m_metadata[blockHead] = m_metadata[blockHead] | InstructionMetadata::BasicBlockStart;

            Code::Region blockRegion(blockHead, end - blockHead);

            Code::Iterator iter(m_source, blockRegion);
            for (; !iter.finished(); ++iter) {
                m_metadata[iter.index()] = m_metadata[iter.index()] | InstructionMetadata::Code;

                if (iter.index() - iter.lastIndex() == 3) {
                    m_metadata[iter.index()] = m_metadata[iter.index()] | InstructionMetadata::LastInstructionTripleWidth;
                } else if (iter.index() - iter.lastIndex() == 2) {
                    m_metadata[iter.index()] = m_metadata[iter.index()] | InstructionMetadata::LastInstructionDoubleWidth;
                }

                if (iter.instruction() == Code::Instruction::Halt) {
                    m_hasHalts = true;
                }

                if (iter.instruction() == Code::Instruction::Call && iter.lastWasPush() && iter.pushValue() == fHead) {
                    if (iter.hasMoreInstructions()) {
                        // If a call is followed by a return instruction then this is tail call, and not a recursive call
                        functionHasRecursiveCalls |= iter.nextInstruction() != Code::Instruction::Ret;
                    }
                } else if (instructionImplementedWithCall(iter.instruction())) {
                    functionHasRecursiveCalls = true;
                }

                // Next instruction is unreachable from this one
                if (iter.instruction() == Code::Instruction::Ret || iter.instruction() == Code::Instruction::Halt) {
                    fEnd = std::max(fEnd, iter.index() + 1);
                    break;
                } else if (isJump(iter.instruction())) {
                    if (!iter.lastWasPush()) {
                        return Status::VariableJumpNotAllowed;
                    }
                    if (iter.pushValue() < fHead || (size_t)iter.pushValue() >= end) {
                        return Status::IllegalJump;
                    }

                    // A new basic block starts immediately after a conditional jump
                    // because execution can fall through
                    if (iter.instruction() == Code::Instruction::Cjmp && iter.nextIndex() != iter.region().end()) {
                        basicBlockLocations.push(iter.nextIndex());
                    }

                    basicBlockLocations.push(iter.pushValue());

                    // If the jump is at the end of a function then we need to know the 'next index' to correctly
                    // find the end of the function
                    ++iter;

                    // A jump determines the end of a basic block
                    break;
                } else if (iter.instruction() == Code::Instruction::Call) {
                    if (iter.lastWasPush()) {
                        if (m_source.region().contains((size_t)iter.pushValue())) {
                            functionLocations.push(iter.pushValue());
                        } else {
                            printf("WARNING: Call to illegal location %d at %d\n", iter.pushValue(), (int)iter.index());
                            return Status::IllegalCall;
                        }
                    } else {
                        // printf("WARNING: Non-static call at %d\n", (int)iter.index());
                        m_hasDynamicCalls = true;
                    }
                }
            }

            fEnd = std::max(fEnd, iter.index());
        }

        auto functionRegion = Code::Region(fHead, fEnd - fHead);
        m_functionRegions.push_back(functionRegion);
        m_newFunctionRegions.push_back(functionRegion);

        if (!functionHasRecursiveCalls && TailCallsOptimised) {
            m_metadata[fHead] = m_metadata[fHead] | InstructionMetadata::NoRecursion;
        }
    }

    return Status::Success;
}

StaticAnalysis::Status StaticAnalysis::verifyInstructionMetadata() const
{
    for (auto metadata : m_metadata) {
        auto violated = violatedProperty(metadata);
        switch (violated) {
        case InstructionMetadata::Code:
            return Status::CodeOverlapsWithIllegalInstruction;
        case InstructionMetadata::BasicBlockStart:
            return Status::BasicBlockStartNotCode;
        case InstructionMetadata::LastInstructionDoubleWidth:
        case InstructionMetadata::LastInstructionTripleWidth:
            return Status::InvalidWidth;
        case InstructionMetadata::FunctionStart:
            return Status::FunctionStartNotTreatedAsBasicBlock;
        default:
            break;
        }
    }
    return Status::Success;
}

bool StaticAnalysis::functionNeedsToPushRegisters(size_t i) const
{
    return !any(m_metadata[i] & InstructionMetadata::NoRecursion);
}

int StaticAnalysis::previousInstructionIndex(int offset) const
{
    if (any(m_metadata[offset] & InstructionMetadata::LastInstructionTripleWidth)) {
        return offset - 3;
    } else if (any(m_metadata[offset] & InstructionMetadata::LastInstructionDoubleWidth)) {
        return offset - 2;
    } else {
        return offset - 1;
    }
}

void StaticAnalysis::printStaticAnalyis() const
{
    printf("STATIC ANALYSIS\n");
    printf("---------------\n");
    printf("Data length = %u\n", m_source.length());
    printf("Code region [%u, %u)\n", m_codeRegion.start(), m_codeRegion.end());
    if (m_hasHalts) {
        printf("Halt instructions\n");
    } else {
        printf("No halt instructions\n");
    }
    for (auto function : m_functionRegions) {
        printf("Function [%u, %u)\n", function.start(), function.end());
        for (auto basicBlock : basicBlocksForFunction(function)) {
            auto effect = stackEffectForBasicBlock(basicBlock);
            printf("  Basic block [%u,%u) pops %d, pushes %d, difference = %d\n", basicBlock.start(), basicBlock.end(), effect.popCount(), effect.pushCount(), effect.heightDifference());
            for (Code::Iterator iter(m_source, basicBlock); !iter.finished(); ++iter) {
                auto meta = m_metadata[iter.index()];
                printf("    %2u | ", iter.index());
                int width = 0;
                if (any(meta & InstructionMetadata::Illegal)) {
                    printf("illegal ");
                    width += 8; // strlen("illegal ")
                }
                if (any(meta & InstructionMetadata::Code)) {
                    printf("code ");
                    width += 5; // strlen("code ")
                }
                if (any(meta & InstructionMetadata::BasicBlockStart)) {
                    printf("block ");
                    width += 6; // strlen("block ")
                }
                if (any(meta & InstructionMetadata::FunctionStart)) {
                    printf("func ");
                    width += 5; // strlen("func ")
                }
                if (any(meta & InstructionMetadata::NoRecursion)) {
                    printf("norec ");
                    width += 6; // strlen("norec ")
                }
                // 31 is the width of all these strings taken together
                while (width < 31) {
                    printf(" ");
                    width++;
                }
                printf("| ");
                Code::printStackInstruction(&m_source[iter.index()]);
                printf("\n");
            }
        }
    }
}

std::vector<Code::Region> StaticAnalysis::basicBlocksForFunction(Code::Region functionRegion) const
{
    std::vector<Code::Region> blocks;
    Code::Iterator iter(m_source, functionRegion);
    while (!iter.finished()) {
        auto start = iter.index();
        ++iter;
        while (!iter.finished() && !isJumpDestination(iter.index())) {
            ++iter;
        }
        auto end = iter.index();
        blocks.emplace_back(start, end - start);
    }
    return blocks;
}

Code::Region StaticAnalysis::basicBlockAtIndex(int start) const
{
    auto iter = Code::Iterator(m_source, Code::Region(start, m_source.region().end() - start));
    // A basic block is at least one instruction, hence the initial ++iter;
    for (++iter; !iter.finished() && !isJumpDestination(iter.index()); ++iter) {
    }
    return Code::Region(start, iter.index() - start);
}

Code::BlockStackEffect StaticAnalysis::stackEffectForBasicBlock(Code::Region basicBlock) const
{
    return Code::BlockStackEffect(Code::Iterator(m_source, basicBlock));
}

static const char* statusStrings[] = {
    "UnknownFailure",
    "Success",
    "CodeOverlapsWithIllegalInstruction",
    "BasicBlockStartNotCode",
    "InvalidWidth",
    "FunctionStartNotTreatedAsBasicBlock",
    "VariableJumpNotAllowed",
    "IllegalJump",
    "IllegalCall"
};

const char* StaticAnalysis::statusString(Status status)
{
    return statusStrings[(int)status];
}

void StaticAnalysis::serialise()
{
    Transfer::Serialiser serialiser("sa");
    serialiser.appendUnsignedInt(m_codeRegion.start());
    serialiser.appendUnsignedInt(m_codeRegion.length());
    serialiser.appendUnsignedInt(m_dataRegion.start());
    serialiser.appendUnsignedInt(m_dataRegion.length());
    serialiser.appendInt(m_hasHalts);
    serialiser.appendInt(m_hasDynamicCalls);

    serialiser.appendUnsignedInt(m_metadata.size());
    serialiser.appendData((uint8_t*)m_metadata.data(), m_metadata.size() * sizeof(InstructionMetadata));

    serialiser.appendUnsignedInt(m_functionRegions.size());
    // serialiser.appendData((uint8_t*)m_functionRegions.data(), m_functionRegions.size() * sizeof(Code::Region));
    for (Code::Region& region : m_functionRegions) {
        serialiser.appendUnsignedInt(region.start());
        serialiser.appendUnsignedInt(region.length());
    }

    serialiser.appendUnsignedInt(m_newFunctionRegions.size());
    serialiser.appendData((uint8_t*)m_newFunctionRegions.data(), m_newFunctionRegions.size() * sizeof(Code::Region));
}

void StaticAnalysis::deserialise()
{
    Transfer::Deserialiser deserialiser("sa");
    if (deserialiser.exists() && deserialiser.length() > 0) {
        size_t codeStart = deserialiser.readUnsignedInt();
        size_t codeLength = deserialiser.readUnsignedInt();
        size_t dataStart = deserialiser.readUnsignedInt();
        size_t dataLength = deserialiser.readUnsignedInt();

        m_codeRegion = Code::Region(codeStart, codeLength);
        m_dataRegion = Code::Region(dataStart, dataLength);

        m_hasHalts = deserialiser.readInt();
        m_hasDynamicCalls = deserialiser.readInt();

        size_t metadataLength = deserialiser.readUnsignedInt();
        m_metadata = std::vector<InstructionMetadata>(metadataLength, InstructionMetadata::Nothing);
        deserialiser.readData((uint8_t*)m_metadata.data(), metadataLength * sizeof(InstructionMetadata));

        size_t functionRegionLength = deserialiser.readUnsignedInt();
        m_functionRegions = std::vector<Code::Region>(functionRegionLength, Code::Region());
        deserialiser.readData((uint8_t*)m_functionRegions.data(), functionRegionLength * sizeof(Code::Region));

        size_t newFunctionRegionLength = deserialiser.readUnsignedInt();
        m_newFunctionRegions = std::vector<Code::Region>(newFunctionRegionLength, Code::Region());
        deserialiser.readData((uint8_t*)m_newFunctionRegions.data(), newFunctionRegionLength / sizeof(Code::Region));
    }
}
}