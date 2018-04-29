#include "InstructionSelection.h"

namespace JIT {

bool instructionImplementedWithCall(Code::Instruction instruction)
{
    switch (instruction) {
    case Code::Instruction::Add:
    case Code::Instruction::Sub:
    case Code::Instruction::Mul:
    case Code::Instruction::Inc:
    case Code::Instruction::Dec:
    case Code::Instruction::Lt:
    case Code::Instruction::Le:
    case Code::Instruction::Eq:
    case Code::Instruction::Ge:
    case Code::Instruction::Gt:
    case Code::Instruction::Drop:
    case Code::Instruction::Dup:
    case Code::Instruction::Ndup:
    case Code::Instruction::Push8:
    case Code::Instruction::Push16:
    case Code::Instruction::Jmp:
    case Code::Instruction::Cjmp:
    case Code::Instruction::Halt:
    case Code::Instruction::Ret:
        return false;
    default:
        // Conservative assumption: all instructions are implemented in terms of a call
        return true;
    }
}
}