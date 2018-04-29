#pragma once

#include "Config.h"

#include "Code/Instruction.h"
#include "VM.h"

namespace Environment {

class Device {
public:
    virtual VMFunction resolveVirtualMachineFunction(Code::Instruction instruction) const = 0;
    virtual void programHalted() const = 0;
};
}