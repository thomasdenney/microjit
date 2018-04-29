#include "StackLinkOperation.h"

#include "Config.h"

#include "ARM/Encoder.h"

namespace JIT {
class MinimalConditionalBranch : public StackLinkOperation {
private:
    ARM::Condition m_condition;
    ARM::Register m_operand1, m_operand2;

public:
    MinimalConditionalBranch(size_t startOffset, size_t to, int skipCount, ARM::Condition cond, ARM::Register cmp1, ARM::Register cmp2)
        : StackLinkOperation(startOffset, to, skipCount)
        , m_condition(cond)
        , m_operand1(cmp1)
        , m_operand2(cmp2)
    {
    }

    size_t instructionCount() final;
    bool linkStackCode(ARM::Functor& func, size_t realDestinationOffset) final;
};
}