#include "VMStatus.h"

#include "Config.h"

namespace Environment {

static const char* vmStatusStrings[] = {
    "Success",
    "UnknownFailure",
    "StackOverflow",
    "StackUnderflow",
    "OutOfBoundsFetch",
    "CompilerError"
};

const char* VMStatusString(VMStatus status)
{
    return vmStatusStrings[(int)status];
}
}