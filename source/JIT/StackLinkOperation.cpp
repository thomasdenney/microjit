#include "StackLinkOperation.h"

namespace JIT {

bool StackLinkOperation::link(ARM::Functor& func, const StaticAnalysis& analysis, const std::map<size_t, size_t>& jumpOffsets, const SpecialLinkerLocations& specialLocations)
{
    auto functionLocInBytecode = destination();
    auto destinationIter = jumpOffsets.find(functionLocInBytecode);
    if (destinationIter == jumpOffsets.end()) {
        return false;
    }
    auto destination = destinationIter->second;
    if (!isCall()) {
        if (analysis.isCallDestination(functionLocInBytecode) && analysis.functionNeedsToPushRegisters(functionLocInBytecode)) {
            // Go one forward
            destination++;
        }
        destination += m_skipCount;
    }
    return linkStackCode(func, destination);
}

bool StackLinkOperation::isCall()
{
    return false;
}
}