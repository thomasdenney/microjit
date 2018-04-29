#include "Decoder.h"

#include "Bit/Bit.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace ARM {

/** SPECIFIC INSTRUCTION DECODING */

void decodeArithmeticRegister(ARM::Instruction instruction, char* buffer);

/** ADD (1) A7-5 */
void decodeAddSmallImm(ARM::Instruction instruction, char* buffer);

/** ADD (2) A7-6 */
void decodeAddLargeImm(ARM::Instruction instruction, char* buffer);

/** ADD (3) A7-7 */
void decodeAddReg(ARM::Instruction instruction, char* buffer);

/** ADD (4) A7-8 */
void decodeAddGeneral(ARM::Instruction instruction, char* buffer);

/** ADD (5) A7-10 */
void decodeAddPCRelativeAddress(ARM::Instruction instruction, char* buffer);

/** ADD (6) A7-10 */
void decodeAddSPRelativeAddress(ARM::Instruction instruction, char* buffer);

/** ADD (7) A7-12 */
void decodeAddSP(ARM::Instruction instruction, char* buffer);

/** ASR (1) A7-15, LSL (1) A7-64, LSR (1) A7-68 */
void decodeImmediateShift(ARM::Instruction instruction, char* buffer);

/** B (1) A7-19 */
void decodeConditionalBranch(ARM::Instruction instruction, char* buffer, uint16_t* address);

/** B (2) A7-21 */
void decodeUnconditionalBranch(ARM::Instruction instruction, char* buffer, uint16_t* address);

/** BL, BLX (1) A7-26 declared in header file */

/** BLX (2) A7-30 */
void decodeBranchLinkExchangeToRegister(ARM::Instruction instruction, char* buffer);

/** BX A7-32 */
void decodeBranchAndExchange(ARM::Instruction instruction, char* buffer);

/** CMP (1) A7-35 */
void decodeCompareImmediate(ARM::Instruction instruction, char* buffer);

/** CMP (3) A7-37 */
void decodeCompareRegistersGeneral(ARM::Instruction instruction, char* buffer);

/** LDMIA A7-44, STMIA A7-96 */
void decodeLoadOrStoreMultipleIncrementAfter(ARM::Instruction instruction, char* buffer);

/** LDR (1) A7-47, LDRB (1) A7-55, LDRH (1) A7-57, STR (1) A7-99, STRB (1)
 * A67-105, STRH (1) A7-109 */
void decodeLoadOrStoreWithOffset(ARM::Instruction instruction, char* buffer);

/** LDR (2) A7-49, LDRB (2) A7-56, LDRH (2) A7-58, STR (2) A7-101, STRB (2)
 * A7-107, STRH (2) A7-111 */
void decodeLoadOrStoreWithRegisterOffset(ARM::Instruction instruction, char* buffer);

/** LDR (3) A7-51 */
void decodeLoadWithPCOffset(ARM::Instruction instruction, char* buffer, uint16_t* address);

/** LDR (4) A7-53 */
void decodeLoadOrStoreWordWithStackPointerOffset(ARM::Instruction instruction, char* buffer);

/** MOV (1) A7-72 */
void decodeMoveImmediate(ARM::Instruction instruction, char* buffer);

/** MOV (2) A7-73 */
void decodeMoveLowToLow(ARM::Instruction instruction, char* buffer);

/** MOV (3) A7-75 */
void decodeMoveGeneral(ARM::Instruction instruction, char* buffer);

/** POP A7-82, PUSH A7-85 */
void decodePopOrPush(ARM::Instruction instruction, char* buffer);

/** SUB (1) A7-113 */
void decodeSubSmallImm(ARM::Instruction instruction, char* buffer);

/** SUB (2) A7-114 */
void decodeSubLargeImm(ARM::Instruction instruction, char* buffer);

/** SUB (3) A7-115 */
void decodeSubReg(ARM::Instruction instruction, char* buffer);

/** SUB (4) A7-116 */
void decodeSubSP(ARM::Instruction instruction, char* buffer);

/** SXTB A7-120 */
void decodeSignExtendByte(ARM::Instruction instruction, char* buffer);

/** SXTH A7-121 */
void decodeSignExtendHalfWord(ARM::Instruction instruction, char* buffer);

/** UTILITIES */

static const char* conditionStrings[] = {
    "eq",
    "ne",
    "cs",
    "cc",
    "mi",
    "pl",
    "vs",
    "vc",
    "hi",
    "ls",
    "ge",
    "lt",
    "gt",
    "le"
};

const char* decodeCondition(ARM::Condition c)
{
    return conditionStrings[(size_t)c];
}

static const char* registerStrings[] = {
    "r0",
    "r1",
    "r2",
    "r3",
    "r4",
    "r5",
    "r6",
    "r7",
    "r8",
    "r9",
    "r10",
    "r11",
    "ip",
    "sp",
    "lr",
    "pc"
};

const char* decodeRegister(ARM::Register r)
{
    return registerStrings[(int)r];
}

void decodeRegisterList(ARM::RegisterList list, char* buffer)
{
    int listInt = (int)list;
    for (int i = 0; i < 8; i++) {
        if (listInt & (1 << i)) {
            buffer += sprintf(buffer, "%s", decodeRegister((ARM::Register)i));
            if (listInt > (1 << (i + 1)) - 1) {
                buffer += sprintf(buffer, ", ");
            }
        }
    }
}

// TODO constexpr inline
int unTwosComplement(uint32_t value, uint8_t bits)
{
    // Sign-bit is set at the front
    if (value & (1 << (bits - 1))) {
        // TODO Be smarter about this
        return value - (1 << bits);
    }
    return (int)value;
}

void printFunction(void (*func)(void), size_t length)
{
    printf("-------\n");
    printf("Position\tOffset\tHex\tBin\t\t\tInstruction\n");

    // Ensures that the last bit is zero, so that regardless of whether we were
    // handed a real function pointer or an address to a block of memory we
    // always read the first instruction
    uint16_t* jumpTable = (uint16_t*)((int)func & 0xFFFFFFFE);

    char buffer[17]; // For binary strings
    char instructionBuffer[32];

    for (size_t i = 0; i < length; i++) {
        memset(instructionBuffer, 0, 32);
        if (isLongCall(jumpTable[i])) {
            decodeBranchLong(jumpTable[i], jumpTable[i + 1], instructionBuffer, &jumpTable[i]);
            printf("%08x\t%d\t%04x\t%s\t%s\n", (unsigned int)&jumpTable[i], i,
                jumpTable[i], Bit::itob(jumpTable[i], buffer), instructionBuffer);
            i++;
            // Print the encoding of the subsequent instruction that contains
            // the remainder of the address
            printf("%08x\t%d\t%04x\t%s\t\n", (unsigned int)&jumpTable[i], i, (unsigned int)jumpTable[i], Bit::itob(jumpTable[i], buffer));
        } else {
            printf("%08x\t%d\t%04x\t%s\t%s\n", (unsigned int)&jumpTable[i], i, (unsigned int)jumpTable[i], Bit::itob(jumpTable[i], buffer),
                decode(jumpTable[i], instructionBuffer, &jumpTable[i]));
        }
    }
    printf("-------\n");
}

char* decode(ARM::Instruction instruction, char* buffer)
{
    return decode(instruction, buffer, (uint16_t*)-4);
}

char* decode(ARM::Instruction instruction, char* buffer, uint16_t* address)
{
    // These are sorted lexicographically
    if (Bit::uintRegion(instruction, 11, 5) == 0b00000 || Bit::uintRegion(instruction, 11, 5) == 0b00001 || Bit::uintRegion(instruction, 11, 5) == 0b00010) {
        decodeImmediateShift(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 9, 7) == 0b0001100) {
        decodeAddReg(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 9, 7) == 0b0001101) {
        decodeSubReg(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 9, 7) == 0b0001110 && Bit::uintRegion(instruction, 6, 3) == 0b000) {
        decodeMoveLowToLow(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 9, 7) == 0b0001110) {
        decodeAddSmallImm(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 9, 7) == 0b0001111) {
        decodeSubSmallImm(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 11, 5) == 0b00100) {
        decodeMoveImmediate(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 11, 5) == 0b00101) {
        decodeCompareImmediate(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 11, 5) == 0b00110) {
        decodeAddLargeImm(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 11, 5) == 0b00111) {
        decodeSubLargeImm(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 10, 6) == 0b010000) {
        decodeArithmeticRegister(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 8, 8) == 0b01000100) {
        decodeAddGeneral(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 8, 8) == 0b01000101) {
        decodeCompareRegistersGeneral(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 8, 8) == 0b01000110) {
        decodeMoveGeneral(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 7, 9) == 0b010001110) {
        decodeBranchAndExchange(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 7, 9) == 0b010001111) {
        decodeBranchLinkExchangeToRegister(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 9, 7) == 0b0101100 || Bit::uintRegion(instruction, 9, 7) == 0b0101000 || Bit::uintRegion(instruction, 9, 7) == 0b0101010 || Bit::uintRegion(instruction, 9, 7) == 0b0101001 || Bit::uintRegion(instruction, 9, 7) == 0b0101110 || Bit::uintRegion(instruction, 9, 7) == 0b0101101 || Bit::uintRegion(instruction, 9, 7) == 0b0101011 || Bit::uintRegion(instruction, 9, 7) == 0b0101111) {
        decodeLoadOrStoreWithRegisterOffset(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 11, 5) == 0b01101 || Bit::uintRegion(instruction, 11, 5) == 0b01100 || Bit::uintRegion(instruction, 11, 5) == 0b01111 || Bit::uintRegion(instruction, 11, 5) == 0b01110 || Bit::uintRegion(instruction, 11, 5) == 0b10001 || Bit::uintRegion(instruction, 11, 5) == 0b10000) {
        decodeLoadOrStoreWithOffset(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 11, 5) == 0b10011 || Bit::uintRegion(instruction, 11, 5) == 0b10010) {
        decodeLoadOrStoreWordWithStackPointerOffset(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 11, 5) == 0b01001) {
        decodeLoadWithPCOffset(instruction, buffer, address);
    } else if (Bit::uintRegion(instruction, 7, 9) == 0b101100000) {
        decodeAddSP(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 7, 9) == 0b101100001) {
        decodeSubSP(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 11, 5) == 0b010100) {
        decodeAddPCRelativeAddress(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 11, 5) == 0b010101) {
        decodeAddSPRelativeAddress(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 12, 4) == 0b1101) {
        decodeConditionalBranch(instruction, buffer, address);
    } else if (Bit::uintRegion(instruction, 6, 10) == 0b1011001001) {
        decodeSignExtendByte(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 6, 10) == 0b1011001000) {
        decodeSignExtendHalfWord(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 9, 7) == 0b1011010 || Bit::uintRegion(instruction, 9, 7) == 0b1011110) {
        decodePopOrPush(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 11, 5) == 0b11000 || Bit::uintRegion(instruction, 11, 5) == 0b11001) {
        decodeLoadOrStoreMultipleIncrementAfter(instruction, buffer);
    } else if (Bit::uintRegion(instruction, 11, 5) == 0b11100) {
        decodeUnconditionalBranch(instruction, buffer, address);
    }
    // else {
    //     char bin[17] = {0};
    //     itob(instruction, bin);
    //     printf("%s unknown\n", bin);
    // }
    return buffer;
}

void decodeArithmeticRegister(ARM::Instruction instruction, char* buffer)
{
    ARM::ArithmeticLogicOperation op = (ARM::ArithmeticLogicOperation)Bit::uintRegion(instruction, 6, 4);
    ARM::Register rm = (ARM::Register)Bit::uintRegion(instruction, 3, 3);
    ARM::Register rd = (ARM::Register)Bit::uintRegion(instruction, 0, 3);

    switch (op) {
    case ARM::ArithmeticLogicOperation::adc:
        sprintf(buffer, "adc %s, %s", decodeRegister(rd), decodeRegister(rm));
        break;
    case ARM::ArithmeticLogicOperation::andB:
        sprintf(buffer, "and %s, %s", decodeRegister(rd), decodeRegister(rm));
        break;
    case ARM::ArithmeticLogicOperation::asr2:
        sprintf(buffer, "asr %s, %s", decodeRegister(rd), decodeRegister(rm));
        break;
    case ARM::ArithmeticLogicOperation::bic:
        sprintf(buffer, "bic %s, %s", decodeRegister(rd), decodeRegister(rm));
        break;
    case ARM::ArithmeticLogicOperation::cmn:
        sprintf(buffer, "cmn %s, %s", decodeRegister(rd), decodeRegister(rm));
        break;
    case ARM::ArithmeticLogicOperation::cmp2:
        sprintf(buffer, "cmp %s, %s", decodeRegister(rd), decodeRegister(rm));
        break;
    case ARM::ArithmeticLogicOperation::eor:
        sprintf(buffer, "eor %s, %s", decodeRegister(rd), decodeRegister(rm));
        break;
    case ARM::ArithmeticLogicOperation::lsl2:
        sprintf(buffer, "lsl %s, %s", decodeRegister(rd), decodeRegister(rm));
        break;
    case ARM::ArithmeticLogicOperation::lsr2:
        sprintf(buffer, "lsr %s, %s", decodeRegister(rd), decodeRegister(rm));
        break;
    case ARM::ArithmeticLogicOperation::mul:
        sprintf(buffer, "mul %s, %s", decodeRegister(rd), decodeRegister(rm));
        break;
    case ARM::ArithmeticLogicOperation::mvn:
        sprintf(buffer, "mvn %s, %s", decodeRegister(rd), decodeRegister(rm));
        break;
    case ARM::ArithmeticLogicOperation::neg:
        sprintf(buffer, "neg %s, %s", decodeRegister(rd), decodeRegister(rm));
        break;
    case ARM::ArithmeticLogicOperation::orr:
        sprintf(buffer, "orr %s, %s", decodeRegister(rd), decodeRegister(rm));
        break;
    case ARM::ArithmeticLogicOperation::ror:
        sprintf(buffer, "ror %s, %s", decodeRegister(rd), decodeRegister(rm));
        break;
    case ARM::ArithmeticLogicOperation::sbc:
        sprintf(buffer, "sbc %s, %s", decodeRegister(rd), decodeRegister(rm));
        break;
    case ARM::ArithmeticLogicOperation::tst:
        sprintf(buffer, "tst %s, %s", decodeRegister(rd), decodeRegister(rm));
        break;
    default:
        break;
    }
}

void decodeAddLargeImm(ARM::Instruction instruction, char* buffer)
{
    auto rd = (ARM::Register)Bit::uintRegion(instruction, 8, 3);
    int imm = Bit::uintRegion(instruction, 0, 8);
    sprintf(buffer, "add %s, #%d", decodeRegister(rd), imm);
}

void decodeAddReg(ARM::Instruction instruction, char* buffer)
{
    auto rm = (ARM::Register)Bit::uintRegion(instruction, 6, 3);
    auto rn = (ARM::Register)Bit::uintRegion(instruction, 3, 3);
    auto rd = (ARM::Register)Bit::uintRegion(instruction, 0, 3);
    sprintf(buffer, "add %s, %s, %s", decodeRegister(rd), decodeRegister(rn), decodeRegister(rm));
}

void decodeAddGeneral(ARM::Instruction instruction, char* buffer)
{
    auto rd = (ARM::Register)(Bit::uintRegion(instruction, 0, 3) + Bit::uintRegion(instruction, 7, 1) * 0b1000);
    auto rm = (ARM::Register)Bit::uintRegion(instruction, 3, 4);
    sprintf(buffer, "add %s, %s", decodeRegister(rd), decodeRegister(rm));
}

void decodeAddSmallImm(ARM::Instruction instruction, char* buffer)
{
    int imm = Bit::uintRegion(instruction, 6, 3);
    auto rn = (ARM::Register)Bit::uintRegion(instruction, 3, 3);
    auto rd = (ARM::Register)Bit::uintRegion(instruction, 0, 3);
    sprintf(buffer, "add %s, %s, #%d", decodeRegister(rd), decodeRegister(rn), imm);
}

void decodeAddPCRelativeAddress(ARM::Instruction instruction, char* buffer)
{
    int imm = Bit::uintRegion(instruction, 0, 8);
    auto rd = (ARM::Register)Bit::uintRegion(instruction, 8, 3);
    sprintf(buffer, "add %s, PC, #%d * 4", decodeRegister(rd), imm);
}

void decodeAddSPRelativeAddress(ARM::Instruction instruction, char* buffer)
{
    int imm = Bit::uintRegion(instruction, 0, 8);
    auto rd = (ARM::Register)Bit::uintRegion(instruction, 8, 3);
    sprintf(buffer, "add %s, SP, #%d * 4", decodeRegister(rd), imm);
}

void decodeAddSP(ARM::Instruction instruction, char* buffer)
{
    int imm = Bit::uintRegion(instruction, 0, 7);
    sprintf(buffer, "add SP, #%d * 4", imm);
}

void decodeImmediateShift(ARM::Instruction instruction, char* buffer)
{
    int imm = Bit::uintRegion(instruction, 6, 5);
    auto rm = (ARM::Register)Bit::uintRegion(instruction, 3, 3);
    auto rd = (ARM::Register)Bit::uintRegion(instruction, 0, 3);
    int op = Bit::uintRegion(instruction, 11, 2);
    sprintf(buffer, "%s %s, %s, #%d", op == 0 ? "lsl" : op == 1 ? "lsr" : "asr", decodeRegister(rd), decodeRegister(rm), imm);
}

void decodeConditionalBranch(ARM::Instruction instruction, char* buffer, uint16_t* address)
{
    auto c = (ARM::Condition)Bit::uintRegion(instruction, 8, 4);
    int offset = unTwosComplement(Bit::uintRegion(instruction, 0, 8) * 2, 9);
    unsigned int destination = (uint32_t)address + 4 + offset;
    sprintf(buffer, "b%s %08x /* by %d */", decodeCondition(c), destination, offset);
}

void decodeUnconditionalBranch(ARM::Instruction instruction, char* buffer, uint16_t* address)
{
    int offset = unTwosComplement(Bit::uintRegion(instruction, 0, 11) * 2, 12);
    unsigned int destination = (uint32_t)address + 4 + offset;
    sprintf(buffer, "b %08x /* by %d */", destination, offset);
}

bool isLongCall(ARM::Instruction instruction)
{
    return Bit::uintRegion(instruction, 13, 3) == 0b111 && Bit::uintRegion(instruction, 11, 5) != 0b11100;
}

void decodeBranchLong(ARM::Instruction instruction1, ARM::Instruction instruction2, char* buffer, uint16_t* address)
{
    bool isBlx = Bit::uintRegion(instruction2, 11, 2) == 0b01; // 0b11 otherwise
    unsigned int pc = (unsigned int)address + 4;
    int offset = unTwosComplement(Bit::uintRegion(instruction1, 0, 11), 11) << 12;
    offset += Bit::uintRegion(instruction2, 0, 11) << 1;
    unsigned int destination = pc + offset;
    if (isBlx) {
        destination = destination & 0xFFFFFFFC;
        sprintf(buffer, "blx %08x /* by %d */", destination, offset / 2);
    } else {
        sprintf(buffer, "bl %08x /* by %d */", destination, offset / 2);
    }
}

void decodeBranchLinkExchangeToRegister(ARM::Instruction instruction, char* buffer)
{
    sprintf(buffer, "blx %s", decodeRegister((ARM::Register)Bit::uintRegion(instruction, 3, 4)));
}

void decodeBranchAndExchange(ARM::Instruction instruction, char* buffer)
{
    auto rm = (ARM::Register)Bit::uintRegion(instruction, 3, 4);
    if (rm == ARM::Register::lr) {
        sprintf(buffer, "ret");
    } else {
        sprintf(buffer, "bx %s", decodeRegister(rm));
    }
}

void decodeCompareImmediate(ARM::Instruction instruction, char* buffer)
{
    auto rn = (ARM::Register)Bit::uintRegion(instruction, 8, 3);
    int imm = Bit::uintRegion(instruction, 0, 8);
    sprintf(buffer, "cmp %s, #%d", decodeRegister(rn), imm);
}

void decodeCompareRegistersGeneral(ARM::Instruction instruction, char* buffer)
{
    auto rn = (ARM::Register)(Bit::uintRegion(instruction, 0, 3) + Bit::uintRegion(instruction, 7, 1) * 0b1000);
    auto rm = (ARM::Register)Bit::uintRegion(instruction, 3, 4);
    sprintf(buffer, "cmp %s, %s", decodeRegister(rn), decodeRegister(rm));
}

void decodeLoadOrStoreMultipleIncrementAfter(ARM::Instruction instruction, char* buffer)
{
    bool isStore = Bit::uintRegion(instruction, 11, 5) == 0b11000;
    auto rn = (ARM::Register)Bit::uintRegion(instruction, 8, 3);
    buffer += sprintf(buffer, "%s %s!, ", isStore ? "stmia" : "ldmia", decodeRegister(rn));
    decodeRegisterList((ARM::RegisterList)Bit::uintRegion(instruction, 0, 8), buffer);
}

void decodeLoadOrStoreWithOffset(ARM::Instruction instruction, char* buffer)
{
    int imm = Bit::uintRegion(instruction, 6, 5);
    auto rn = (ARM::Register)Bit::uintRegion(instruction, 3, 3);
    auto rd = (ARM::Register)Bit::uintRegion(instruction, 0, 3);
    uint8_t op = Bit::uintRegion(instruction, 11, 5);
    const char* opStr = op & 1 ? "ld" : "st";
    op = op >> 1; // Ignore last bit

    if (op == 0b0110) {
        sprintf(buffer, "%sr %s, [%s, #%d * 4]", opStr, decodeRegister(rd), decodeRegister(rn), imm);
    } else if (op == 0b0111) {
        sprintf(buffer, "%srb %s, [%s, #%d]", opStr, decodeRegister(rd), decodeRegister(rn), imm);
    } else if (op == 0b1000) {
        sprintf(buffer, "%srh %s, [%s, #%d * 2]", opStr, decodeRegister(rd), decodeRegister(rn), imm);
    }
}

void decodeLoadOrStoreWithRegisterOffset(ARM::Instruction instruction, char* buffer)
{
    auto rm = (ARM::Register)Bit::uintRegion(instruction, 6, 3);
    auto rn = (ARM::Register)Bit::uintRegion(instruction, 3, 3);
    auto rd = (ARM::Register)Bit::uintRegion(instruction, 0, 3);
    uint8_t op = Bit::uintRegion(instruction, 9, 7);
    if (op == 0b0101100) {
        sprintf(buffer, "ldr %s, [%s, %s]", decodeRegister(rd), decodeRegister(rn), decodeRegister(rm));
    } else if (op == 0b0101000) {
        sprintf(buffer, "str %s, [%s, %s]", decodeRegister(rd), decodeRegister(rn), decodeRegister(rm));
    } else if (op == 0b0101110) {
        sprintf(buffer, "ldrb %s, [%s, %s]", decodeRegister(rd), decodeRegister(rn), decodeRegister(rm));
    } else if (op == 0b0101010) {
        sprintf(buffer, "strb %s, [%s, %s]", decodeRegister(rd), decodeRegister(rn), decodeRegister(rm));
    } else if (op == 0b0101011) {
        sprintf(buffer, "ldrsb %s, [%s, %s]", decodeRegister(rd), decodeRegister(rn), decodeRegister(rm));
    } else if (op == 0b0101101) {
        sprintf(buffer, "ldrh %s, [%s, %s]", decodeRegister(rd), decodeRegister(rn), decodeRegister(rm));
    } else if (op == 0b0101001) {
        sprintf(buffer, "strh %s, [%s, %s]", decodeRegister(rd), decodeRegister(rn), decodeRegister(rm));
    } else if (op == 0b0101111) {
        sprintf(buffer, "ldrsh %s, [%s, %s]", decodeRegister(rd), decodeRegister(rn), decodeRegister(rm));
    }
}

void decodeLoadWithPCOffset(ARM::Instruction instruction, char* buffer, uint16_t* address)
{
    auto rd = (ARM::Register)Bit::uintRegion(instruction, 8, 3);
    int imm = Bit::uintRegion(instruction, 0, 8);
    int destination = (((int)address + 4) & 0xFFFFFFFC) + imm * 4;
    sprintf(buffer, "ldr %s, [pc, #%d * 4] /* %08x */", decodeRegister(rd), imm, (unsigned int)destination);
}

void decodeLoadOrStoreWordWithStackPointerOffset(ARM::Instruction instruction, char* buffer)
{
    auto rd = (ARM::Register)Bit::uintRegion(instruction, 8, 3);
    int imm = Bit::uintRegion(instruction, 0, 8);
    uint8_t op = Bit::uintRegion(instruction, 11, 5);
    const char* opStr = op & 1 ? "ld" : "st";
    sprintf(buffer, "%sr %s, [sp, #%d * 4]", opStr, decodeRegister(rd), imm);
}

void decodeMoveImmediate(ARM::Instruction instruction, char* buffer)
{
    auto rd = (ARM::Register)Bit::uintRegion(instruction, 8, 3);
    int imm = Bit::uintRegion(instruction, 0, 8);
    sprintf(buffer, "mov %s, #%d", decodeRegister(rd), imm);
}

void decodeMoveLowToLow(ARM::Instruction instruction, char* buffer)
{
    auto rn = (ARM::Register)Bit::uintRegion(instruction, 3, 3);
    auto rd = (ARM::Register)Bit::uintRegion(instruction, 0, 3);
    sprintf(buffer, "mov %s, %s", decodeRegister(rd), decodeRegister(rn));
}

void decodeMoveGeneral(ARM::Instruction instruction, char* buffer)
{
    auto rd = (ARM::Register)(Bit::uintRegion(instruction, 0, 3) + Bit::uintRegion(instruction, 7, 1) * 0b1000);
    auto rm = (ARM::Register)Bit::uintRegion(instruction, 3, 4);
    if (rd == ARM::Register::r8 && rm == ARM::Register::r8) {
        sprintf(buffer, "nop");
    } else {
        sprintf(buffer, "mov %s, %s", decodeRegister(rd), decodeRegister(rm));
    }
}

void decodePopOrPush(ARM::Instruction instruction, char* buffer)
{
    bool isPop = Bit::uintRegion(instruction, 9, 7) == 0b1011110;
    if (Bit::uintRegion(instruction, 8, 1)) {
        buffer += sprintf(buffer, "%s", isPop ? "pop pc" : "push lr");
    } else {
        buffer += sprintf(buffer, "%s", isPop ? "pop" : "push");
    }
    auto list = (ARM::RegisterList)Bit::uintRegion(instruction, 0, 8);
    if ((int)list > 0) {
        if (Bit::uintRegion(instruction, 8, 1)) {
            buffer += sprintf(buffer, ", ");
        } else {
            buffer += sprintf(buffer, " ");
        }
        decodeRegisterList((ARM::RegisterList)Bit::uintRegion(instruction, 0, 8), buffer);
    }
}

void decodeSubSmallImm(ARM::Instruction instruction, char* buffer)
{
    int imm = Bit::uintRegion(instruction, 6, 3);
    auto rn = (ARM::Register)Bit::uintRegion(instruction, 3, 3);
    auto rd = (ARM::Register)Bit::uintRegion(instruction, 0, 3);
    sprintf(buffer, "sub %s, %s, #%d", decodeRegister(rd), decodeRegister(rn), imm);
}

void decodeSubLargeImm(ARM::Instruction instruction, char* buffer)
{
    auto rd = (ARM::Register)Bit::uintRegion(instruction, 8, 3);
    int imm = Bit::uintRegion(instruction, 0, 8);
    sprintf(buffer, "sub %s, #%d", decodeRegister(rd), imm);
}

void decodeSubReg(ARM::Instruction instruction, char* buffer)
{
    auto rm = (ARM::Register)Bit::uintRegion(instruction, 6, 3);
    auto rn = (ARM::Register)Bit::uintRegion(instruction, 3, 3);
    auto rd = (ARM::Register)Bit::uintRegion(instruction, 0, 3);
    sprintf(buffer, "sub %s, %s, %s", decodeRegister(rd), decodeRegister(rn), decodeRegister(rm));
}

void decodeSubSP(ARM::Instruction instruction, char* buffer)
{
    int imm = Bit::uintRegion(instruction, 0, 7);
    sprintf(buffer, "sub sp, #%d * 4", imm);
}

void decodeSignExtendByte(ARM::Instruction instruction, char* buffer)
{
    auto rm = (ARM::Register)Bit::uintRegion(instruction, 3, 3);
    auto rd = (ARM::Register)Bit::uintRegion(instruction, 0, 3);
    sprintf(buffer, "sxtb %s, %s", decodeRegister(rd), decodeRegister(rm));
}

void decodeSignExtendHalfWord(ARM::Instruction instruction, char* buffer)
{
    auto rm = (ARM::Register)Bit::uintRegion(instruction, 3, 3);
    auto rd = (ARM::Register)Bit::uintRegion(instruction, 0, 3);
    sprintf(buffer, "sxth %s, %s", decodeRegister(rd), decodeRegister(rm));
}
}
