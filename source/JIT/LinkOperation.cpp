#include "LinkOperation.h"

namespace JIT {

void LinkOperation::reserve(ARM::Functor& func)
{
    auto k = instructionCount();
    for (size_t i = 0; i != k; ++i) {
        func.add(ARM::nop());
    }
}
}