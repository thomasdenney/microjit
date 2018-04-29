#pragma once

#include "Config.h"

#include "LinkOperation.h"
#include <cstddef>

namespace JIT {

class StackLinkOperation : public LinkOperation {
private:
    size_t m_destination;
    int m_skipCount;

public:
    StackLinkOperation(size_t startOffset, size_t destination, int skipCount)
        : LinkOperation(startOffset)
        , m_destination(destination)
        , m_skipCount(skipCount)
    {
    }

    size_t destination() const { return m_destination; }

    virtual bool isCall();

    /// Subclasses should override this
    virtual bool linkStackCode(ARM::Functor& func, size_t realDestinationOffset);

    bool link(ARM::Functor& func, const StaticAnalysis& analysis, const std::map<size_t, size_t>& jumpOffsets, const SpecialLinkerLocations& specialLocations) final;
};
}
