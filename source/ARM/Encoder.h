#pragma once

#include "Config.h"

#include <cstdint>

/**
 * This files defines functions that encode ARM instructions into the Thumb
 * instruction sets.
 *
 * All instructions are encoded in little endian.
 * 
 * The primary source for the functions in this file is the ARM reference manual,
 * which describes the encoding of each Thumb2 instruction. Additionally all the
 * cycle counts for the Cortex-M0+ processor are from the Cortex-M0+ technical
 * reference manual (http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0484c/CHDCICDF.html)
 *
 * TODO Should be able to optimise a lot with C++ constexpr
 *
 * TODO Conventions:
 *  - Unsigned and signed integer types are used to denote the difference in
 *    instructions
 *  - The page number of the ARM reference manual and the correpsonding
 *    instruction are always noted
 *  - All instructions should be tested
 */

namespace ARM {

enum class Register : uint8_t {
    // Argument/result/scratch
    r0 = 0,
    a1 = 0,
    // Argument/result/scratch
    r1 = 1,
    a2 = 1,
    // Argument/scratch
    r2 = 2,
    a3 = 2,
    // Argument/scratch
    r3 = 3,
    a4 = 3,
    // Variable - all onwards are callee-preserved
    r4 = 4,
    v1 = 4,
    // Variable
    r5 = 5,
    v2 = 5,
    // Variable
    r6 = 6,
    v3 = 6,
    // Variable
    r7 = 7,
    v4 = 7,
    // Variable
    r8 = 8,
    v5 = 8,
    // Variable
    r9 = 9,
    v6 = 9,
    // Platform register
    r10 = 10,
    v7 = 10,
    // Variable
    r11 = 11,
    v8 = 11,
    // The Intra-Procedure-call scratch register
    r12 = 12,
    ip = 12,
    // Stack pointer
    r13 = 13,
    sp = 13,
    // Link registers
    r14 = 14,
    lr = 14,
    // Program counter
    r15 = 15,
    pc = 15
};

/** Arithmetic or Logic Operation */
enum class ArithmeticLogicOperation : uint8_t {
    adc = 0b0101,
    andB = 0b0000,
    asr2 = 0b0100,
    bic = 0b1110,
    cmn = 0b1011, // TODO
    cmp2 = 0b1010, // TODO
    eor = 0b0001,
    lsl2 = 0b0010,
    lsr2 = 0b0011,
    mul = 0b1101,
    mvn = 0b1111,
    neg = 0b1001,
    orr = 0b1100,
    ror = 0b0111,
    sbc = 0b0110,
    tst = 0b1000, // TODO
};

/**
 * Comparison condition codes
 * See page A3-4 of the ARM reference manual
 *
 * TODO Document purpose of each flag
 */
enum class Condition : uint8_t {
    eq = 0b0000, // Equal, Z set
    ne = 0b0001, // Not equal, Z clear
    cs = 0b0010,
    hs = 0b0010, // Carry set/unsigned higher or same, C set
    cc = 0b0011,
    lo = 0b0011, // Carry clear/unsigned lower, C clear
    mi = 0b0100, // Minus/negative, N set
    pl = 0b0101, // Positive or zero, N clear
    vs = 0b0110, // Overflow, V set
    vc = 0b0111, // No overflow, V clear
    hi = 0b1000, // Unsigned higher, C set and Z clear
    ls = 0b1001, // Unsigned lower or same, C clear or Z set
    ge = 0b1010, // Signed greater than or equal Nset and V set, or N clear and V clear
    lt = 0b1011, // Signed less than, N set and V clear, or N clear and V set
    gt = 0b1100, // Signed greater than, Z clear, and either N set and V set, or N clear and V clear
    le = 0b1101, // Signed less than or equal, Z set, or N set and V clear, or N clear and V set
    // al = 0b1110, // Always - this seems to produce undefined instructions...
    // 0b1111 unused in Thumb?
};

/**
 * Only implemented for eq, ne, gt, ge, le, lt
 */
Condition InvertCondition(Condition c);

/**
 * Use bitwise or to combine these
 *
 * TODO contexpr variadic function for cleaner combining
 */
enum class RegisterList : uint8_t {
    empty = 0b00000000,
    r0 = 0b00000001,
    r1 = 0b00000010,
    r2 = 0b00000100,
    r3 = 0b00001000,
    r4 = 0b00010000,
    r5 = 0b00100000,
    r6 = 0b01000000,
    r7 = 0b10000000
};

inline constexpr RegisterList operator|(RegisterList x, RegisterList y)
{
    return static_cast<RegisterList>(static_cast<uint32_t>(x) | static_cast<uint32_t>(y));
}

typedef uint16_t Instruction;

struct InstructionPair {
    Instruction instruction1, instruction2;
};

/**
 * Performs an assertion that |r| is r0-7
 */
void assertLowRegister(Register r);

/**
 * Firstly ensures that |value| only contains the |maxBits| least significant
 * bits, and then shifts the value by |offset|
 */
Instruction shift(uint16_t value, uint16_t offset, uint8_t maxBits);
Instruction shiftReg(Register value, uint16_t offset, uint8_t maxBits);

void resetEncodingStatusFlags();

/**
 * Returns true if all encoding flags are OK
 */
bool checkEncodingStatusFlags();

void printEncodingStatusFlags();

void setAllowedToPrintStatusFlags(bool allowedToPrint);

/**
 * ADC A7-4
 * Adds two values and the carry flag
 * |rd| and |rm| must be low registers
 * 1 cycle
 */
Instruction addWithCarry(Register rd, Register rm);

/**
 * ADD (1) A7-5
 * |rd| = |rn| + |imm|
 * 0 <= |imm| < 8, |rd| and |rn| must be low registers
 * 1 cycle
 */
Instruction addSmallImm(Register rd, Register rn, uint8_t imm);

/**
 * ADD (2) A7-6
 * Add an eight bit unsigned immediate value
 * |rd| must be a low register
 * 1 cycle
 */
Instruction addLargeImm(Register rd, uint8_t imm);

/**
 * ADD (3) A7-7
 * |rd| = |rn| + |rm|
 * All registers must be low registers
 * 1 cycle
 */
Instruction addReg(Register rd, Register rn, Register rm);

/**
 * ADD (4) A7-8
 * |rd| = |rd| + |rm|
 * At least one of the registes should be a high register!
 * 1 cycle, or 2 if |rd| is PC
 */
Instruction addGeneral(Register rd, Register rm);

/**
 * ADD (5) A7-10
 * |rd| = PC + |imm| * 4
 * |rd| must be a a low register
 * 1 cycle
 */
Instruction addPCRelativeAddress(Register rd, uint8_t imm);

/**
 * ADD (6) A7-11
 * |rd| = SP + |imm| * 4
 * |rd| must be a a low register
 * 1 cycle
 */
Instruction addSPRelativeAddress(Register rd, uint8_t imm);

/**
 * ADD (7) A7-12
 * SP = SP + |imm7| * 4
 * 1 cycle
 */
Instruction addSP(uint8_t imm7);

/**
 * AND A7-14
 * |rd| = |rd| & |rm|
 * |rd| and |rm| must be low registers
 * 1 cycle
 */
Instruction andBitwise(Register rd, Register rm);

/**
 * ASR (1) A7-15
 * |rd| = |rm| >> imm
 * |rm| and |rd| must be low registers
 * 1 cycle
 */
Instruction arithmeticShiftRightImm(Register rd, Register rm, uint8_t imm);

/**
 * ASR (2) A7-17
 * |rd| = |rd| >> |rs|
 * |rd| and |rs| must be low registers
 * 1 cycle
 */
Instruction arithmeticShiftRightRegister(Register rd, Register rs);

/**
 * B (1) A7-19
 * Conditional branch by signed |imm| if |c| holds
 * @see unconditionalBranch for branch jumping information
 * 1 cycle if the branch is not taken or 3 if it is
 */
Instruction conditionalBranch(Condition c, int32_t imm);

/**
 * Alternative version of B (1)
 */
Instruction conditionalBranchNatural(Condition c, int32_t imm);

/**
 * B (2) A7-21
 * Unconditional branch by |imm| (signed)
 *
 * The value of |imm| is an offset from the adress of the branch instruction +
 * 4, all divided by two. This is effectively the same as computing the offset
 * from not the instruction that immediately follows the branch, but the one
 * after that. So if you want to branch to the next instruction, do -1, two
 * instructions, do 0, the branch instruction itself, do -2
 * 
 * 3 cycles
 */
Instruction unconditionalBranch(int32_t imm);

/**
 * Alternative implementation of B (2) where |imm| is the offset, by instruction
 * count, from the current instruction
 */
Instruction unconditionalBranchNatural(int32_t imm);

/**
 * BIC A7-23
 * |rd| = |rd| & !|rm|
 * |rd| and |rs| must be low registers
 * 1 cycle
 */
Instruction bitClear(Register rd, Register rm);

/**
 * BL (1) A7-26
 * Used for calling another Thumb subroutine
 * Offset is from instruction address + 4, as per convention. See other branch
 * instructions.
 * Returns a pair of instructions for branching and linking by |offset|
 * instructions
 * 4 cycles
 */
InstructionPair branchAndLink(int32_t offset);

InstructionPair branchAndLinkNatural(int32_t offset);

/**
 * BLX (1) A7-26
 * Used for calling another ARM subroutine
 * See BL (1)
 * 3 cycles
 */
InstructionPair branchLinkAndExchange(int32_t offset);

/**
 * BLX (2) A7-30
 * Branches to the address stored in |rm|
 * 3 cycles
 */
Instruction branchLinkExchangeToRegister(Register rm);

/**
 * BX A7-32
 * Branches between ARM code and Thumb code
 * 3 cycles
 */
Instruction branchAndExchange(Register rm);

/**
 * CMP (1) A7-35
 * Comparison between a low register |rn| and an immediate 8-bit value |imm|
 * 1 cycle
 */
Instruction compareImmediate(Register rn, uint8_t imm);

/**
 * CMP (2) A7-36
 * Comparison between two low registers
 * 1 cycle
 */
Instruction compareLowRegisters(Register rn, Register rm);

/**
 * CMP (3) A7-37
 * Comparison between two registers
 * 1 cycle
 */
Instruction compareRegistersGeneral(Register rn, Register rm);

/**
 * EOR A7-43
 * |rd| = |rd| EOR |rm|
 * |rd| and |rm| must be low registers
 * 1 cycle
 */
Instruction eor(Register rd, Register rm);

/**
 * LDMIA A7-44
 * Loads values from subsequent addresses into the registers in |regs| starting
 * from the address in |rn| and then increments |rn| by the number of addresses
 * loaded (* 4) if |rn| is not in |regs|. Can be used in conjunction with STMIA
 * for efficient block copy.
 *
 * |rn| must be a low register
 * 
 * 1 + N where N is the number of registers to be loaded
 */
Instruction loadMultipleIncrementAfter(Register rn, RegisterList regs);

/**
 * LDR (1) A7-47
 * Loads 32-bit words from memory
 * 2 cycles
 */
Instruction loadWordWithOffset(Register rd, Register rn, uint8_t offset);

/**
 * LDR (2) A7-49
 * 2 cycles
 */
Instruction loadWordWithRegisterOffset(Register rd, Register rn, Register rm);

/**
 * LDR (3) A7-51
 * Loads PC-relative data
 * 2 cycles
 */
Instruction loadWordWithPCOffset(Register rd, uint8_t offset);

/**
 * LDR (4) A7-53
 * SP relative data
 * 2 cycles
 */
Instruction loadWordWithStackPointerOffset(Register rd, uint8_t offset);

/**
 * LDRB (1) A7-55
 * Loads a byte and zero-extends it; useful for accessing record files
 * 2 cycles
 */
Instruction loadByteWithOffset(Register rd, Register rn, uint8_t offset);

/**
 * LDRB (2) A7-56
 * 2 cycles
 */
Instruction loadByteWithRegisterOffset(Register rd, Register rn, Register rm);

/**
 * LDRH (1) A7-57
 * Loads a half word and zero-extends it; useful for accessing record files
 * 2 cycles
 */
Instruction loadHalfWordWithOffset(Register rd, Register rn, uint8_t offset);

/**
 * LDRH (2) A7-59
 * 2 cycles
 */
Instruction loadHalfWordWithRegisterOffset(Register rd, Register rn, Register rm);

/**
 * LDRSB A7-61
 * 2 cycles
 */
Instruction loadSignedByteWithRegisterOffset(Register rd, Register rn, Register rm);

/**
 * LDRSH A7-62
 * 2 cycles
 */
Instruction loadSignedHalfWordWithRegisterOffset(Register rd, Register rn, Register rm);

/**
 * LSL (1) A7-64
 * |rm| and |rd| must be low registers
 * 0 <= |imm| < 32
 * 1 cycle
 */
Instruction logicalShiftLeftImmediate(Register rd, Register rm, uint8_t imm);

/**
 * LSL (2) A7-66
 * |rd| = |rd| << |rs|
 * |rd| and |rs| must be low registers
 * 1 cycle
 */
Instruction leftShiftLogicalRegister(Register rd, Register rs);

/**
 * LSR (2) A7-70
 * |rd| = |rd| >> |rs|
 * |rd| and |rs| must be low registers
 * 1 cycle
 */
Instruction rightShiftLogicalRegister(Register rd, Register rs);

/**
 * MOV (1) A7-72
 * Moves an immediate unsigned 8-bit value to |rd|
 * 1 cycle
 */
Instruction moveImmediate(Register rd, uint8_t x);

/**
 * MOV (2) A7-73
 * |rd| = |rn|
 * 1 cycle
 */
Instruction moveLowToLow(Register rd, Register rn);

/**
 * MOV (3) A7-75
 * Moves from any register to any other register
 * Doesn't change the flags, unlike MOV (2)
 * 1 cycle, or 2 if |rd| is PC
 */
Instruction moveGeneral(Register rd, Register rm);

/**
 * MUL A7-77
 * |rd| = |rd| * |rm|
 * |rd| and |rm| must be low registers
 * 1 cycle (as the micro:bit/nrf51 has a 'fast' multiplier --- the Cortex-M0+ supports either fast/small multipliers)
 */
Instruction mul(Register rd, Register rm);

/**
 * MVN A7-79
 * |rd| = !|rm|
 * |rd| and |rm| must be low registers
 * 1 cycle
 */
Instruction moveNot(Register rd, Register rm);

/**
 * NEG A7-80
 * |rd| = 0 - |rm|
 * |rd| and |rm| must be low registers
 * 1 cycle
 */
Instruction neg(Register rd, Register rm);

/**
 * ORR A7-81
 * |rd| = |rd| | |rm|
 * |rd| and |rm| must be low registers
 * 1 cycle
 */
Instruction orBitwise(Register rd, Register rm);

/**
 * POP A7-82
 * Pops multiple registers in r0 to r7 or PC from the stack - i.e. if PC is
 * popped then a branch occurs
 * Written as POP in the THUMB and LDMIA in ARM
 * 1 + N cycles if |pc| is false, or 3 + N if |pc| is true where N is the number of registers in |regs|
 */
Instruction popMultiple(bool pc, RegisterList regs);

/**
 * PUSH A7-85
 * Pushes multiple registers in r0 to r7 or PC to the stack
 * Note that this is written in ARM as STMDB, and Thumb as PUSH
 * 1 + N cycles where N is the number of registers in |regs|
 */
Instruction pushMultiple(bool lr, RegisterList regs);

/**
 * STR (1) A7-99
 * Stores 32-bit data from a general-purpose register to memory
 * 2 cycles
 */
Instruction storeWordWithOffset(Register rd, Register rn, uint8_t imm);

/**
 * STR (2) A7-101
 * 2 cycles
 */
Instruction storeWordWithRegisterOffset(Register rd, Register rn, Register rm);

/**
 * STR (3) A7-103
 * 2 cycles
 */
Instruction storeWordWithStackPointerOffset(Register rd, uint8_t imm);

/**
 * STRB (1) A7-105
 * 2 cycles
 */
Instruction storeByteWithOffset(Register rd, Register rn, uint8_t imm);

/**
 * STRB (2) A7-107
 * 2 cycles
 */
Instruction storeByteWithRegisterOffset(Register rd, Register rn, Register rm);

/**
 * STRH (1) A7-109
 * 2 cycles
 */
Instruction storeHalfWordWithOffset(Register rd, Register rn, uint8_t imm);

/**
 * STRH (2) A7-111
 * 2 cycles
 */
Instruction storeHalfWordWithRegisterOffset(Register rd, Register rn, Register rm);

/**
 * ROR A7-92
 * |rd| = |rd| rotated right by |rm|
 * |rd| and |rm| must be low registers
 * TODO Find a nice equivalent in C
 * 1 cycle
 */
Instruction ror(Register rd, Register rs);

/**
 * SBC A7-94
 * |rd| = |rd| - |rm| - NOT(C FLAG)
 * |rd| and |rm| must be low registers
 * 1 cycle
 */
Instruction subtractWithCarry(Register rd, Register rm);

/**
 * STMIA A7-96
 * Stores a set of registers |regs| from the address starting in |rn|
 * |rn| is a low register and it will be incremented to four times the number of
 * registers in |regs|
 * 1 + N cycles, where N is the number of registers in |regs|
 */
Instruction storeMultipleIncrementAfter(Register rn, RegisterList regs);

/**
 * SUB (1) A7-113
 * |rd| = |rn| - |imm|
 * 0 <= |imm| < 8, |rd| and |rn| must be low registers
 * 1 cycle
 */
Instruction subSmallImm(Register rd, Register rn, uint8_t imm);

/**
 * SUB (2) A7-114
 * Subtract an eight bit unsigned immediate value
 * |rd| must be a low register
 * 1 cycle
 */
Instruction subLargeImm(Register rd, uint8_t imm);

/**
 * SUB (3) A7-115
 * |rd| = |rn| - |rm|
 * All registers must be low registers
 * 1 cycle
 */
Instruction subReg(Register rd, Register rn, Register rm);

/**
 * SUB (4) A7-116
 * Decrements the SP by four times a 7-bit immediate
 * 1 cycle
 */
Instruction subSP(uint8_t imm);

/**
 * SXTB A7-120
 * |rd| = SignExtend(rm[7:0])
 * Extracts the least significant eight bits of |rm| and sign-extends them to 32 bits
 * |rd| and |rm| must be low registers
 * 1 cycle
 */
Instruction signExtendByte(Register rd, Register rm);

/**
 * SXTH A7-121
 * |rd| = SignExtend(rm[15:0])
 * Extracts the least significant sixteen bits of |rm| and sign-extends them to 32 bits
 * |rd| and |rm| must be low registers
 * 1 cycle
 */
Instruction signExtendHalfWord(Register rd, Register rm);

/** Pseudo-instructions */

/**
 * Equivalent to mov r8, r8
 * 1 cycle
 */
Instruction nop();

/**
 * Returns from a procedure
 * Equivalent to blx lr
 * 1 cycle
 */
Instruction ret();
}