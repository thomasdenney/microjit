#pragma once

#include "Config.h"

#include "LinkOperation.h"

namespace JIT {

class SpecialLinkerOperation : public LinkOperation {
public:
    enum class Kind : int {
        Halt = 0,
        StackUnderflowError = 1,
        StackOverflowError = 2
    };

private:
    Kind m_kind;

    int destinationOffset(const SpecialLinkerLocations& specialLocations);

public:
    SpecialLinkerOperation(size_t insertionIndex, Kind kind)
        : LinkOperation(insertionIndex)
        , m_kind(kind)
    {
    }

    size_t instructionCount() final;
    bool link(ARM::Functor& func, const StaticAnalysis& analysis, const std::map<size_t, size_t>& jumpOffsets, const SpecialLinkerLocations& specialLocations) final;
};
}