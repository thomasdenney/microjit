#include "InstructionMetadata.h"

namespace JIT {

InstructionMetadata violatedProperty(InstructionMetadata meta)
{
    if (any(meta & InstructionMetadata::NoRecursion)) {
        if (!any(meta & InstructionMetadata::FunctionStart)) {
            return InstructionMetadata::NoRecursion;
        }
    } else if (any(meta & InstructionMetadata::FunctionStart)) {
        if (!any(meta & InstructionMetadata::BasicBlockStart)) {
            return InstructionMetadata::FunctionStart;
        }
    } else if (any(meta & InstructionMetadata::LastInstructionTripleWidth)) {
        if (!any(meta & InstructionMetadata::Code) || any(meta & InstructionMetadata::LastInstructionDoubleWidth)) {
            return InstructionMetadata::LastInstructionTripleWidth;
        }
    } else if (any(meta & InstructionMetadata::LastInstructionDoubleWidth)) {
        if (!any(meta & InstructionMetadata::Code) || any(meta & InstructionMetadata::LastInstructionTripleWidth)) {
            return InstructionMetadata::LastInstructionDoubleWidth;
        }
    } else if (any(meta & InstructionMetadata::BasicBlockStart)) {
        if (!any(meta & InstructionMetadata::Code)) {
            return InstructionMetadata::BasicBlockStart;
        }
    } else if (any(meta & InstructionMetadata::Code)) {
        if (any(meta & InstructionMetadata::Illegal)) {
            return InstructionMetadata::Code;
        }
    }
    return InstructionMetadata::Nothing;
}
}