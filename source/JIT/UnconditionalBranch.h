#pragma once

#include "Config.h"

#include "StackLinkOperation.h"

namespace JIT {
class UnconditionalBranch : public StackLinkOperation {
public:
    UnconditionalBranch(size_t startOffset, size_t destination, int skipCount)
        : StackLinkOperation(startOffset, destination, skipCount)
    {
    }

    size_t instructionCount() final;
    bool linkStackCode(ARM::Functor& func, size_t realDestinationOffset) final;
};
}