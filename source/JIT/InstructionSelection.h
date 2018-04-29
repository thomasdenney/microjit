#pragma once

#include "Config.h"

#include "Code/Instruction.h"

namespace JIT {

bool instructionImplementedWithCall(Code::Instruction instruction);
}