#include "UnconditionalBranch.h"

namespace JIT {

size_t UnconditionalBranch::instructionCount()
{
    return 1;
}

bool UnconditionalBranch::linkStackCode(ARM::Functor& func, size_t realDestinationOffset)
{
    uint16_t* buffer = func.buffer();
    int i = insertionOffset();
    buffer[i] = ARM::unconditionalBranchNatural((int)realDestinationOffset - i);
    return true;
}
}