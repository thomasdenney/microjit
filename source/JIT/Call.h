#pragma once

#include "Config.h"

#include "StackLinkOperation.h"

namespace JIT {

class Call : public StackLinkOperation {
private:
public:
    Call(size_t startOffset, size_t to)
        : StackLinkOperation(startOffset, to, 0)
    {
    }

    bool isCall() final;
    size_t instructionCount() final;
    bool linkStackCode(ARM::Functor& func, size_t realDestinationOffset) final;
};
}