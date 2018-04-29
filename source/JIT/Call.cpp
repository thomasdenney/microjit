#include "Call.h"

#include "Compiler.h"

namespace JIT {

size_t Call::instructionCount()
{
    return 2;
}

bool Call::isCall()
{
    return true;
}

bool Call::linkStackCode(ARM::Functor& func, size_t realDestinationOffset)
{
    int i = (int)insertionOffset();

    auto pair = ARM::branchAndLinkNatural((int)realDestinationOffset - (int)i);
    func.buffer()[i] = pair.instruction1;
    func.buffer()[i + 1] = pair.instruction2;
    
    return true;
}
}