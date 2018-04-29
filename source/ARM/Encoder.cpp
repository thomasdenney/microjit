#include "encoder.h"

#include "Bit/Bit.h"

namespace ARM {

/** Encoding status */

bool IncorrectUseOfLowRegister = false;
bool TooBigBranchOffset = false;
bool TooBigImmediate = false;

bool AllowedToPrintDebugMessages = true;

void assertLowRegister(Register r)
{
    if (!(r == Register::r0 || r == Register::r1 || r == Register::r2 || r == Register::r3 || r == Register::r4 || r == Register::r5 || r == Register::r6 || r == Register::r7)) {
        IncorrectUseOfLowRegister = true;
        if (AllowedToPrintDebugMessages) {
            printf("Used register r%d, which is not a low register\n", (uint8_t)r);
        }
    }
}

/** Bit fiddling */

inline Instruction shift(uint16_t value, uint16_t offset, uint8_t maxBits)
{
    uint16_t value2 = value & ((1 << maxBits) - 1);
    if (value2 != value) {
        if (AllowedToPrintDebugMessages) {
            printf("Warning: shifted value %lu too big for %lu bits\n", (unsigned long)value, (unsigned long)maxBits);
        }
        TooBigImmediate = true;
    }
    return value2 << offset;
}

inline Instruction shiftReg(Register value, uint16_t offset, uint8_t maxBits)
{
    return shift((uint16_t)value, offset, maxBits);
}

constexpr inline unsigned highBit(Register r)
{
    return ((unsigned)r & 0b1000) >> 3;
}

void setAllowedToPrintStatusFlags(bool allowedToPrint)
{
    AllowedToPrintDebugMessages = allowedToPrint;
}

void resetEncodingStatusFlags()
{
    IncorrectUseOfLowRegister = false;
    TooBigBranchOffset = false;
    TooBigImmediate = false;
}

bool checkEncodingStatusFlags()
{
    return !(IncorrectUseOfLowRegister || TooBigBranchOffset || TooBigImmediate);
}

void printEncodingStatusFlags()
{
    if (checkEncodingStatusFlags()) {
        printf("Encoding flags OK\n");
    } else {
        printf("Encoding flags not OK:\n");
        if (IncorrectUseOfLowRegister) {
            printf(" * Incorrect use of low register\n");
        }
        if (TooBigBranchOffset) {
            printf(" * Too big branch offset\n");
        }
        if (TooBigImmediate) {
            printf(" * Too big immediate\n");
        }
    }
}

Condition InvertCondition(Condition cond)
{
    switch (cond) {
    case Condition::ge:
        return Condition::lt;
    case Condition::gt:
        return Condition::le;
    case Condition::le:
        return Condition::gt;
    case Condition::lt:
        return Condition::ge;
    case Condition::ne:
        return Condition::eq;
    case Condition::eq:
        return Condition::ne;
    default:
        return (Condition)0b1111; // Unused
    }
}

/** Generalised instructions */

/**
 * All arithmetic/logical operations are generally of the form
 * |rd| = |rd| OP |rm|
 * OP sometimes ignores rd
 * Both |rd| and |rm| must be low registers
 */
Instruction arithmeticOperation(Register rd, Register rm, ARM::ArithmeticLogicOperation opcode)
{
    assertLowRegister(rd);
    assertLowRegister(rm);

    return shift(0b010000, 10, 6) | shift((unsigned)opcode, 6, 4) | shiftReg(rm, 3, 3) | shiftReg(rd, 0, 3);
}

/** Real instructions */

Instruction addWithCarry(Register rd, Register rm)
{
    return arithmeticOperation(rd, rm, ARM::ArithmeticLogicOperation::adc);
}

Instruction addSmallImm(Register rd, Register rn, uint8_t imm)
{
    assertLowRegister(rd);
    assertLowRegister(rn);
    return shift(0b0001110, 9, 7) | shift(imm, 6, 3) | shiftReg(rn, 3, 3) | shiftReg(rd, 0, 3);
}

Instruction addLargeImm(Register rd, uint8_t imm)
{
    assertLowRegister(rd);

    return shift(0b00110, 11, 5) | shiftReg(rd, 8, 3) | shift(imm, 0, 8);
}

Instruction addReg(Register rd, Register rn, Register rm)
{
    assertLowRegister(rd);
    assertLowRegister(rn);
    assertLowRegister(rm);

    return shift(0b0001100, 9, 7) | shiftReg(rm, 6, 3) | shiftReg(rn, 3, 3) | shiftReg(rd, 0, 3);
}

Instruction addGeneral(Register rd, Register rm)
{
    uint8_t h1 = highBit(rd);
    uint8_t h2 = highBit(rm);
    uint8_t l1 = (uint8_t)rd & 0b111;
    uint8_t l2 = (uint8_t)rm & 0b111;
    return shift(0b01000100, 8, 9) | shift(h1, 7, 1) | shift(h2, 6, 1) | shift(l2, 3, 3) | shift(l1, 0, 3);
}

Instruction addPCRelativeAddress(Register rd, uint8_t imm)
{
    assertLowRegister(rd);

    return shift(0b10100, 11, 5) | shiftReg(rd, 8, 3) | shift(imm, 0, 8);
}

Instruction addSPRelativeAddress(Register rd, uint8_t imm)
{
    assertLowRegister(rd);

    return shift(0b10101, 11, 5) | shiftReg(rd, 8, 3) | shift(imm, 0, 8);
}

Instruction addSP(uint8_t imm7)
{
    return shift(0b101100000, 7, 9) | shift(imm7, 0, 7);
}

Instruction andBitwise(Register rd, Register rm)
{
    return arithmeticOperation(rd, rm, ARM::ArithmeticLogicOperation::andB);
}

Instruction arithmeticShiftRightImm(Register rd, Register rm, uint8_t imm)
{
    assertLowRegister(rd);
    assertLowRegister(rm);

    return shift(0b00010, 11, 5) | shift(imm, 6, 5) | shiftReg(rm, 3, 3) | shiftReg(rd, 0, 3);
}

Instruction arithmeticShiftRightRegister(Register rd, Register rs)
{
    return arithmeticOperation(rd, rs, ARM::ArithmeticLogicOperation::asr2);
}

Instruction conditionalBranch(Condition c, int32_t imm)
{
    if (imm < -128 || imm > 127) {
        if (AllowedToPrintDebugMessages) {
            printf("WARNING: Trying to encode conditional branch with immediate %d\n", (int)imm);
            printf("The conditional branch can only encode -128 to +127 instructions (-256 to +254 bytes)\n");
        }
        TooBigBranchOffset = true;
    }
    return shift(0b1101, 12, 4) | shift((uint32_t)c, 8, 4) | shift(Bit::twosComplement(imm, 8), 0, 8);
}

Instruction conditionalBranchNatural(Condition c, int32_t imm)
{
    return conditionalBranch(c, imm - 2);
}

Instruction unconditionalBranch(int32_t imm)
{
    return shift(0b11100, 11, 5) | shift(Bit::twosComplement(imm, 11), 0, 11);
}

Instruction unconditionalBranchNatural(int32_t imm)
{
    return unconditionalBranch(imm - 2);
}

Instruction bitClear(Register rd, Register rm)
{
    return arithmeticOperation(rd, rm, ARM::ArithmeticLogicOperation::bic);
}

InstructionPair blOrBlx(int32_t offset, bool isBlx)
{
    uint32_t twentyTwoBits = Bit::twosComplement(offset, 22);
    Instruction instruction1 = shift(0b111, 13, 3) | shift(0b10, 11, 2) | shift(Bit::uintRegion(twentyTwoBits, 11, 11), 0, 11);
    Instruction instruction2 = shift(0b111, 13, 3) | shift(isBlx ? 0b01 : 0b11, 11, 2) | shift(Bit::uintRegion(twentyTwoBits, 0, 11), 0, 11);
    return InstructionPair{ instruction1, instruction2 };
}

InstructionPair branchAndLink(int32_t offset)
{
    return blOrBlx(offset, false);
}

InstructionPair branchAndLinkNatural(int32_t offset)
{
    return branchAndLink(offset - 2);
}

InstructionPair branchLinkAndExchange(int32_t offset)
{
    return blOrBlx(offset, true);
}

Instruction branchLinkExchangeToRegister(Register rm)
{
    return shift(0b010001111, 7, 9) | shiftReg(rm, 3, 4) | shift(0, 0, 3);
}

Instruction branchAndExchange(Register rm)
{
    return shift(0b010001110, 7, 9) | shiftReg(rm, 3, 4);
}

Instruction compareImmediate(Register rn, uint8_t imm)
{
    return shift(0b00101, 11, 5) | shiftReg(rn, 8, 3) | shift(imm, 0, 8);
}

Instruction compareLowRegisters(Register rn, Register rm)
{
    return arithmeticOperation(rn, rm, ARM::ArithmeticLogicOperation::cmp2);
}

Instruction compareRegistersGeneral(Register rn, Register rm)
{
    uint8_t h1 = highBit(rn);
    uint8_t h2 = highBit(rm);
    uint8_t l1 = (uint8_t)rn & 0b111;
    uint8_t l2 = (uint8_t)rm & 0b111;
    return shift(0b01000101, 8, 8) | shift(h1, 7, 1) | shift(h2, 6, 1) | shift(l2, 3, 3) | shift(l1, 0, 3);
}

Instruction eor(Register rd, Register rm)
{
    return arithmeticOperation(rd, rm, ARM::ArithmeticLogicOperation::eor);
}

Instruction loadMultipleIncrementAfter(Register rn, RegisterList regs)
{
    assertLowRegister(rn);
    return shift(0b11001, 11, 5) | shiftReg(rn, 8, 3) | shift((unsigned)regs, 0, 8);
}

Instruction loadOrStoreWithOffset(uint8_t op, Register rd, Register rn, uint8_t offset)
{
    assertLowRegister(rd);
    assertLowRegister(rn);
    return shift(op, 11, 5) | shift(offset, 6, 5) | shiftReg(rn, 3, 3) | shiftReg(rd, 0, 3);
}

Instruction loadOrStoreWithRegisterOffset(uint8_t op, Register rd, Register rn, Register rm)
{
    assertLowRegister(rd);
    assertLowRegister(rn);
    assertLowRegister(rm);
    return shift(op, 9, 7) | shiftReg(rm, 6, 3) | shiftReg(rn, 3, 3) | shiftReg(rd, 0, 3);
}

Instruction loadWordWithOffset(Register rd, Register rn, uint8_t offset)
{
    return loadOrStoreWithOffset(0b01101, rd, rn, offset);
}

Instruction loadWordWithRegisterOffset(Register rd, Register rn, Register rm)
{
    return loadOrStoreWithRegisterOffset(0b0101100, rd, rn, rm);
}

Instruction loadWordWithPCOffset(Register rd, uint8_t offset)
{
    assertLowRegister(rd);
    return shift(0b01001, 11, 5) | shiftReg(rd, 8, 3) | shift(offset, 0, 8);
}

Instruction loadWordWithStackPointerOffset(Register rd, uint8_t offset)
{
    assertLowRegister(rd);
    return shift(0b10011, 11, 5) | shiftReg(rd, 8, 3) | shift(offset, 0, 8);
}

Instruction loadByteWithOffset(Register rd, Register rn, uint8_t offset)
{
    return loadOrStoreWithOffset(0b01111, rd, rn, offset);
}

Instruction loadByteWithRegisterOffset(Register rd, Register rn, Register rm)
{
    return loadOrStoreWithRegisterOffset(0b0101110, rd, rn, rm);
}

Instruction loadHalfWordWithOffset(Register rd, Register rn, uint8_t offset)
{
    return loadOrStoreWithOffset(0b10001, rd, rn, offset);
}

Instruction loadHalfWordWithRegisterOffset(Register rd, Register rn, Register rm)
{
    return loadOrStoreWithRegisterOffset(0b0101101, rd, rn, rm);
}

Instruction loadSignedByteWithRegisterOffset(Register rd, Register rn, Register rm)
{
    return loadOrStoreWithRegisterOffset(0b0101011, rd, rn, rm);
}

Instruction loadSignedHalfWordWithRegisterOffset(Register rd, Register rn, Register rm)
{
    return loadOrStoreWithRegisterOffset(0b0101111, rd, rn, rm);
}

Instruction logicalShiftLeftImmediate(Register rd, Register rm, uint8_t imm)
{
    assertLowRegister(rd);
    assertLowRegister(rm);

    return shift(0b00000, 11, 5) | shift(imm, 6, 5) | shiftReg(rm, 3, 3) | shiftReg(rd, 0, 3);
}

Instruction leftShiftLogicalRegister(Register rd, Register rs)
{
    return arithmeticOperation(rd, rs, ARM::ArithmeticLogicOperation::lsl2);
}

Instruction rightShiftLogicalRegister(Register rd, Register rs)
{
    return arithmeticOperation(rd, rs, ARM::ArithmeticLogicOperation::lsr2);
}

Instruction moveImmediate(Register rd, uint8_t x)
{
    assertLowRegister(rd);

    return shift(0b00100, 11, 5) | shiftReg(rd, 8, 3) | shift((uint16_t)x, 0, 8);
}

Instruction moveLowToLow(Register rd, Register rn)
{
    assertLowRegister(rd);
    assertLowRegister(rn);

    return shift(0b0001110, 9, 7) | shift(0b000, 6, 3) | shiftReg(rn, 3, 3) | shiftReg(rd, 0, 3);
}

Instruction moveGeneral(Register rd, Register rm)
{
    // This lays out H1 for Rd and H2 for Rn slightly awkwardly
    unsigned h1 = highBit(rd);
    unsigned h2 = highBit(rm);
    return shift(0b01000110, 8, 8) | shift(h1, 7, 1) | shift(h2, 6, 1) | shift((unsigned)rm & 0b111, 3, 3) | shift((unsigned)rd & 0b111, 0, 3);
}

Instruction mul(Register rd, Register rm)
{
    return arithmeticOperation(rd, rm, ARM::ArithmeticLogicOperation::mul);
}

Instruction moveNot(Register rd, Register rm)
{
    return arithmeticOperation(rd, rm, ARM::ArithmeticLogicOperation::mvn);
}

Instruction neg(Register rd, Register rm)
{
    return arithmeticOperation(rd, rm, ARM::ArithmeticLogicOperation::neg);
}

Instruction orBitwise(Register rd, Register rm)
{
    return arithmeticOperation(rd, rm, ARM::ArithmeticLogicOperation::orr);
}

Instruction popMultiple(bool pc, RegisterList regs)
{
    return shift(0b1011110, 9, 7) | shift(pc ? 1 : 0, 8, 1) | // Important that this is exactly one bit
        shift((unsigned)regs, 0, 8);
}

Instruction pushMultiple(bool lr, RegisterList regs)
{
    return shift(0b1011010, 9, 7) | shift(lr ? 1 : 0, 8, 1) | shift((unsigned)regs, 0, 8);
}

Instruction ror(Register rd, Register rs)
{
    return arithmeticOperation(rd, rs, ARM::ArithmeticLogicOperation::ror);
}

Instruction subtractWithCarry(Register rd, Register rm)
{
    return arithmeticOperation(rd, rm, ARM::ArithmeticLogicOperation::sbc);
}

Instruction storeMultipleIncrementAfter(Register rn, RegisterList regs)
{
    return shift(0b11000, 11, 5) | shiftReg(rn, 8, 3) | shift((unsigned)regs, 0, 8);
}

Instruction storeWordWithOffset(Register rd, Register rn, uint8_t imm)
{
    return loadOrStoreWithOffset(0b01100, rd, rn, imm);
}

Instruction storeWordWithRegisterOffset(Register rd, Register rn, Register rm)
{
    return loadOrStoreWithRegisterOffset(0b0101000, rd, rn, rm);
}

Instruction storeWordWithStackPointerOffset(Register rd, uint8_t imm)
{
    assertLowRegister(rd);
    return shift(0b10010, 11, 5) | shiftReg(rd, 8, 3) | shift(imm, 0, 8);
}

Instruction storeByteWithOffset(Register rd, Register rn, uint8_t imm)
{
    return loadOrStoreWithOffset(0b01110, rd, rn, imm);
}

Instruction storeByteWithRegisterOffset(Register rd, Register rn, Register rm)
{
    return loadOrStoreWithRegisterOffset(0b0101010, rd, rn, rm);
}

Instruction storeHalfWordWithOffset(Register rd, Register rn, uint8_t imm)
{
    return loadOrStoreWithOffset(0b10000, rd, rn, imm);
}

Instruction storeHalfWordWithRegisterOffset(Register rd, Register rn, Register rm)
{
    return loadOrStoreWithRegisterOffset(0b0101001, rd, rn, rm);
}

Instruction subSmallImm(Register rd, Register rn, uint8_t imm)
{
    assertLowRegister(rd);
    assertLowRegister(rn);
    return shift(0b0001111, 9, 7) | shift(imm, 6, 3) | shiftReg(rn, 3, 3) | shiftReg(rd, 0, 3);
}

Instruction subLargeImm(Register rd, uint8_t imm)
{
    assertLowRegister(rd);

    return shift(0b00111, 11, 5) | shiftReg(rd, 8, 3) | shift(imm, 0, 8);
}

Instruction subReg(Register rd, Register rn, Register rm)
{
    assertLowRegister(rd);
    assertLowRegister(rn);
    assertLowRegister(rm);

    return shift(0b0001101, 9, 7) | shiftReg(rm, 6, 3) | shiftReg(rn, 3, 3) | shiftReg(rd, 0, 3);
}

Instruction subSP(uint8_t imm)
{
    return shift(0b101100001, 7, 9) | shift(imm, 0, 7);
}

Instruction signExtendByte(Register rd, Register rm)
{
    assertLowRegister(rd);
    assertLowRegister(rm);

    return shift(0b1011001001, 6, 10) | shiftReg(rm, 3, 3) | shiftReg(rd, 0, 3);
}

Instruction signExtendHalfWord(Register rd, Register rm)
{
    assertLowRegister(rd);
    assertLowRegister(rm);

    return shift(0b1011001000, 6, 10) | shiftReg(rm, 3, 3) | shiftReg(rd, 0, 3);
}

/** Pseudoinstructions */

Instruction nop()
{
    return moveGeneral(Register::r8, Register::r8);
}

Instruction ret()
{
    return branchAndExchange(Register::lr);
}
}