#pragma once

#include "ARM/Functor.h"
#include "Environment/VM.h"

namespace JIT {

extern const ARM::Register StatePointerRegister;
extern const ARM::Register StackPointerRegister;
extern const ARM::Register StackTopRegister;
extern const ARM::Register TempRegister;
extern const ARM::Register TempRegister2;
extern const ARM::Register TempRegister3;
extern const ARM::Register StackBaseRegister;
extern const ARM::Register StackEndRegister;

/// Generates code for the full range of 32-bit integers
void compileLoadConstant(ARM::Functor& func, int value, ARM::Register destination);

void compileCFunctionCall(ARM::Functor& func, Environment::VMFunction destination, bool needsToRestoreInvariant = true);

void compileWriteStateToMemory(ARM::Functor& func);
}