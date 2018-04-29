#include "Linker.h"

#include "Call.h"
#include "ConditionalBranch.h"
#include "MinimalConditionalBranch.h"
#include "SpecialLinkerOperation.h"
#include "Support/Memory.h"
#include "Transfer/Deserialiser.h"
#include "Transfer/Serialiser.h"
#include "UnconditionalBranch.h"

namespace JIT {

bool Linker::link(ARM::Functor& func, StaticAnalysis& analysis)
{
    for (auto& linkOperation : m_linkOperations) {
        if (!linkOperation->link(func, analysis, m_linkLocations, m_specialLocations)) {
            return false;
        }
    }
    clear();
    return true;
}

void Linker::addOperation(ARM::Functor& func, std::unique_ptr<LinkOperation> operation)
{
    operation->reserve(func);
    m_linkOperations.push_back(std::move(operation));
}

void Linker::addUnconditionalJump(ARM::Functor& func, size_t offset, int skipCount)
{
    addOperation(func, Support::make_unique<UnconditionalBranch>(func.length(), offset, skipCount));
}

void Linker::addConditionalJump(ARM::Functor& func, size_t offset, int skipCount)
{
    addOperation(func, Support::make_unique<ConditionalBranch>(func.length(), offset, skipCount));
}

void Linker::addMinimalBranchConditionalJump(ARM::Functor& func, size_t offset, int skipCount, ARM::Condition condition, ARM::Register cmp1, ARM::Register cmp2)
{
    addOperation(func, Support::make_unique<MinimalConditionalBranch>(func.length(), offset, skipCount, condition, cmp1, cmp2));
}

void Linker::addCall(ARM::Functor& func, size_t offset)
{
    addOperation(func, Support::make_unique<Call>(func.length(), offset));
}

void Linker::addHalt(ARM::Functor& func)
{
    addOperation(func, Support::make_unique<SpecialLinkerOperation>(func.length(), SpecialLinkerOperation::Kind::Halt));
}

void Linker::addStackOverflowCheck(ARM::Functor& func)
{
    addOperation(func, Support::make_unique<SpecialLinkerOperation>(func.length(), SpecialLinkerOperation::Kind::StackOverflowError));
}

void Linker::addStackUnderflowCheck(ARM::Functor& func)
{
    addOperation(func, Support::make_unique<SpecialLinkerOperation>(func.length(), SpecialLinkerOperation::Kind::StackUnderflowError));
}

void Linker::setHaltOffset(size_t offset)
{
    m_specialLocations.m_locations[(int)SpecialLinkerOperation::Kind::Halt] = offset;
}

void Linker::setStackUnderflowOffset(size_t offset)
{
    m_specialLocations.m_locations[(int)SpecialLinkerOperation::Kind::StackUnderflowError] = offset;
}

void Linker::setStackOverflowOffset(size_t offset)
{
    m_specialLocations.m_locations[(int)SpecialLinkerOperation::Kind::StackOverflowError] = offset;
}

void Linker::setLinkOffset(size_t stackCodeOffset, size_t bytecodeOffset)
{
    m_linkLocations[stackCodeOffset] = bytecodeOffset;
}

bool Linker::hasOffsetForBasicBlock(size_t offset) const
{
    return m_linkLocations.find(offset) != m_linkLocations.end();
}

size_t Linker::offsetForBasicBlock(size_t offset) const
{
    return m_linkLocations.find(offset)->second;
}

void Linker::clear()
{
    m_linkOperations.clear();
}

void Linker::serialise()
{
    Transfer::Serialiser serialiser("linker");
    serialiser.appendUnsignedInt(m_linkLocations.size());
    for (auto& pair : m_linkLocations) {
        serialiser.appendUnsignedInt(pair.first);
        serialiser.appendUnsignedInt(pair.second);
    }
    serialiser.appendUnsignedInt(m_specialLocations.m_locations[0]);
    serialiser.appendUnsignedInt(m_specialLocations.m_locations[1]);
    serialiser.appendUnsignedInt(m_specialLocations.m_locations[2]);
}

void Linker::deserialise()
{
    Transfer::Deserialiser deserialiser("linker");
    if (deserialiser.exists() && deserialiser.length() > 0) {
        size_t linkLocationCount = deserialiser.readUnsignedInt();
        for (size_t i = 0; i < linkLocationCount; ++i) {
            m_linkLocations[deserialiser.readUnsignedInt()] = deserialiser.readUnsignedInt();
        }
        m_specialLocations.m_locations[0] = deserialiser.readUnsignedInt();
        m_specialLocations.m_locations[1] = deserialiser.readUnsignedInt();
        m_specialLocations.m_locations[2] = deserialiser.readUnsignedInt();
    }
}
}