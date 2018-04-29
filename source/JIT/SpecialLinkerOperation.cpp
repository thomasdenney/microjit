#include "SpecialLinkerOperation.h"

namespace JIT {

int SpecialLinkerOperation::destinationOffset(const SpecialLinkerLocations& specialLocations)
{
    return specialLocations.m_locations[(int)m_kind];
}

size_t SpecialLinkerOperation::instructionCount()
{
    return 2;
}

bool SpecialLinkerOperation::link(ARM::Functor& func, const StaticAnalysis& analysis, const std::map<size_t, size_t>& jumpOffsets, const SpecialLinkerLocations& specialLocations)
{
    int i = insertionOffset();
    int actualOffset = (destinationOffset(specialLocations) - i) - 2; // -2 from natural offset
    switch (m_kind) {
    case Kind::Halt:
        func.buffer()[i] = ARM::unconditionalBranch(actualOffset);
        break;
    case Kind::StackOverflowError: {
        if (actualOffset < -128 || actualOffset > 127) {
            if (actualOffset < -1024 || actualOffset > 1023) {
                return false;
            }
            func.buffer()[i] = ARM::conditionalBranch(ARM::Condition::ge, 0);
            func.buffer()[i + 1] = ARM::unconditionalBranch(destinationOffset(specialLocations) - (i + 1) - 2);
        } else {
            func.buffer()[i] = ARM::conditionalBranch(ARM::Condition::lt, actualOffset);
        }
        break;
    }
    case Kind::StackUnderflowError: {
        if (actualOffset < -128 || actualOffset > 127) {
            if (actualOffset < -1024 || actualOffset > 1023) {
                return false;
            }
            func.buffer()[i] = ARM::conditionalBranch(ARM::Condition::le, 0);
            func.buffer()[i + 1] = ARM::unconditionalBranch(destinationOffset(specialLocations) - (i + 1) - 2);
        } else {
            func.buffer()[i] = ARM::conditionalBranch(ARM::Condition::gt, actualOffset);
        }
        break;
    }
    }
    return true;
}
}