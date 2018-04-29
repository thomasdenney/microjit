#include "StackLinkOperation.h"

#include "Config.h"

namespace JIT {

/**
 * This naive version should only be used with the naive register allocation routine
 * or the 'dumb' stack allocation one, i.e. not the COW allocator
 */
class ConditionalBranch : public StackLinkOperation {
public:
    ConditionalBranch(size_t startOffset, size_t to, int skipCount)
        : StackLinkOperation(startOffset, to, skipCount)
    {
    }

    size_t instructionCount() final;
    bool linkStackCode(ARM::Functor& func, size_t realDestinationOffset) final;
};
}