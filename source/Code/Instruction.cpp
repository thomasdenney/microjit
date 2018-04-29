#include "Instruction.h"

#include <cstdio>

namespace Code {

bool isJump(Instruction instr)
{
    return instr == Instruction::Jmp || instr == Instruction::Cjmp;
}

bool isCall(Instruction instr)
{
    return instr == Instruction::Call;
}

bool isJumpOrCall(Instruction instr)
{
    return isJump(instr) || isCall(instr);
}

bool isOptional(Instruction instr)
{
    return (unsigned)instr >= 0x80;
}

bool isPush(Instruction instr)
{
    return instr == Instruction::Push8 || instr == Instruction::Push16;
}

bool isCondition(Instruction instr)
{
    return instr == Instruction::Lt || instr == Instruction::Le || instr == Instruction::Eq || instr == Instruction::Ge || instr == Instruction::Gt;
}

static const char* coreInstructionStrings[] = {
    "add",
    "sub",
    "mul",
    "div",
    "mod",
    "inc",
    "dec",
    "max",
    "min",
    "lt",
    "le",
    "eq",
    "ge",
    "gt",
    "drop",
    "dup",
    "ndup",
    "swap",
    "rot",
    "nrot",
    "tuck",
    "ntuck",
    "size",
    "nrnd",
    "push8",
    "push16",
    "fetch",
    "call",
    "ret",
    "jmp",
    "cjmp",
    "wait",
    "halt"
};

static const char* optionalInstructionStrings[] = {
    "sleep",
    "tone",
    "beep",
    "rgb",
    "colour",
    "flash",
    "temp",
    "accel",
    "pixel"
};

static const char* thomasInstructionStrings[] = {
    "showNum"
};

void printStackInstruction(const Instruction* code)
{
    if (code[0] == Instruction::Push8) {
        printf("%d", Bit::unTwosComplement((unsigned)code[1], 8));
    } else if (code[0] == Instruction::Push16) {
        printf("%d", Bit::unTwosComplement((unsigned)code[1] | (unsigned)code[2] << 8, 16));
    } else if ((int)code[0] >= 0x80) {
        if ((int)code[0] <= 0x80 + 9) {
            printf("%s", optionalInstructionStrings[(int)code[0] - 0x80]);
        } else if ((int)code[0] >= 0xA0 && (int)code[0] <= 0xA0 + 0) {
            printf("%s", thomasInstructionStrings[(int)code[0] - 0xA0]);
        } else {
            printf("optional(%d, %d)", (int)code[1] >> 4, (int)code[1] & 0xF);
        }
    } else {
        printf("%s", coreInstructionStrings[(int)code[0]]);
    }
}

int signed8BitValueAtOffset(const Instruction* data, size_t offset)
{
    return Bit::unTwosComplement((unsigned)data[offset], 8);
}

int signed16BitValueAtOffset(const Instruction* data, size_t offset)
{
    return Bit::unTwosComplement((unsigned)data[offset] | (unsigned)data[offset + 1] << 8, 16);
}
}