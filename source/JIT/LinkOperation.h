#pragma once

#include "Config.h"

#include "ARM/Functor.h"
#include "SpecialLinkerLocations.h"
#include "StaticAnalysis.h"
#include <cstddef>
#include <map>

namespace JIT {

/**
 * Subclasses should work based on the offset
 */
class LinkOperation {
private:
    size_t m_location;

public:
    LinkOperation(size_t startOffset)
        : m_location(startOffset)
    {
    }

    size_t insertionOffset() const { return m_location; }

    void reserve(ARM::Functor& func);

    /**
     * The number of operations required
     */
    virtual size_t instructionCount() = 0;

    /**
     * Actually insert the link from the generated code offset to the desination code offset
     */
    virtual bool link(ARM::Functor& func, const StaticAnalysis& analysis, const std::map<size_t, size_t>& jumpOffsets, const SpecialLinkerLocations& specialLocations) = 0;
};
}