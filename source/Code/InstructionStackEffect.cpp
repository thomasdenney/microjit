#include "InstructionStackEffect.h"

namespace Code {

void InstructionStackEffect::resolveFromInstruction(Instruction instr)
{
    switch (instr) {
    case Instruction::Add:
    case Instruction::Sub:
    case Instruction::Mul:
    case Instruction::Div:
    case Instruction::Mod:
    case Instruction::Max:
    case Instruction::Min:
    case Instruction::Lt:
    case Instruction::Le:
    case Instruction::Eq:
    case Instruction::Ge:
    case Instruction::Gt:
        m_popCount = 2;
        m_pushCount = 1;
        break;
    case Instruction::Inc:
    case Instruction::Dec:
    case Instruction::Fetch:
    case Instruction::Nrnd:
        m_popCount = 1;
        m_pushCount = 1;
        break;
    case Instruction::Drop:
    case Instruction::Call:
    case Instruction::Jmp:
    case Instruction::Wait:
        m_popCount = 1;
        m_pushCount = 0;
        break;
    case Instruction::Halt:
    case Instruction::Ret:
        m_popCount = 0;
        m_pushCount = 0;
        break;
    case Instruction::Dup:
        m_popCount = 1;
        m_pushCount = 2;
        break;
    case Instruction::Cjmp:
        m_popCount = 2;
        m_pushCount = 0;
        break;
    case Instruction::Size:
    case Instruction::Push8:
    case Instruction::Push16:
        m_popCount = 0;
        m_pushCount = 1;
        break;
    case Instruction::Swap:
        m_popCount = 2;
        m_pushCount = 2;
        break;
    case Instruction::Tuck:
    case Instruction::Rot:
        m_popCount = 3;
        m_pushCount = 3;
        break;
    case Instruction::Ndup:
    case Instruction::Nrot:
    case Instruction::Ntuck:
        m_popCount = UnknownEffect;
        m_pushCount = 0;
        break;
    default:
        // Currently: all optional instructions
        m_popCount = UnknownEffect;
        m_pushCount = UnknownEffect;
        break;
    }
}
}