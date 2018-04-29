#pragma once

#include "Config.h"

#include "Bit/Bit.h"
#include <cstdint>
#include <cstdlib>

namespace Code {

enum class Instruction : uint8_t {
    Add = 0x00,
    Sub = 0x01,
    Mul = 0x02,
    Div = 0x03,
    Mod = 0x04,
    Inc = 0x05,
    Dec = 0x06,
    Max = 0x07,
    Min = 0x08,
    Lt = 0x09,
    Le = 0x0a,
    Eq = 0x0b,
    Ge = 0x0c,
    Gt = 0x0d,
    Drop = 0x0e,
    Dup = 0x0f,
    Ndup = 0x10,
    Swap = 0x11,
    Rot = 0x12,
    Nrot = 0x13,
    Tuck = 0x14,
    Ntuck = 0x15,
    Size = 0x16,
    Nrnd = 0x17,
    Push8 = 0x18,
    Push16 = 0x19,
    Fetch = 0x1a,
    Call = 0x1b,
    Ret = 0x1c,
    Jmp = 0x1d,
    Cjmp = 0x1e,
    Wait = 0x1f,
    Halt = 0x20,
    // Optional instructions
    Sleep = 0x80,
    SleepEffect = 0x01,
    Tone = 0x81,
    ToneEffect = 0x01,
    Beep = 0x82,
    BeepEffect = 0x02,
    Rgb = 0x83,
    RgbEffect = 0x03,
    Colour = 0x84,
    ColourEffect = 0x01,
    Flash = 0x85,
    FlashEffect = 0x02,
    Temp = 0x86,
    TempEffect = 0x10,
    Accel = 0x87,
    AccelEffect = 0x30,
    Pixel = 0x88,
    PixelEffect = 0x02,
    // Thomas' custom instructions
    ShowNumber = 0xA0,
    ShowNumberEffect = 0x01
};

/**
 * Unsafe method that doesn't do bounds checking
 */
void printStackInstruction(const Instruction* code);

bool isJump(Instruction instr);
bool isBranch(Instruction instr);
bool isJumpOrCall(Instruction instr);
bool isOptional(Instruction instr);
bool isPush(Instruction instr);
bool isCondition(Instruction instr);

int signed8BitValueAtOffset(const Instruction* data, size_t offset);
int signed16BitValueAtOffset(const Instruction* data, size_t offset);

inline constexpr uint16_t uintRegion(Instruction instruction, uint8_t offset, uint8_t length)
{
    return Bit::uintRegion((unsigned)instruction, offset, length);
}
}
