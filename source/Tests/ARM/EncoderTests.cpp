#include "Tests.h"

#include "ARM/Decoder.h"
#include "ARM/Encoder.h"
#include "ARM/Functor.h"
#include "Tests/Utilities.h"
#include <cstdio>

namespace ARM {

typedef Instruction (*ArithmeticEncoder)(Register, Register);

/** Test helpers */

bool testArithmetic(int a, int b, int expected, ArithmeticEncoder encoder)
{
    Functor func;

    func.add(encoder(Register::r0, Register::r1));
    func.add(ret());

    int res = func.call<int, int>(a, b);

    if (res != expected) {
        printf("%d != %d\n", res, expected);
        return false;
    }
    return true;
}

/**
 * Does a comparison |c| between |a| and |b| that is expected to succeed
 */
bool testConditionalImmediate(int a, int b, Condition c)
{
    Functor func;

    func.add(compareImmediate(Register::r0, b));
    func.add(conditionalBranchNatural(c, 3));
    func.add(moveImmediate(Register::r0, 0));
    func.add(ret());
    func.add(moveImmediate(Register::r0, 1));
    func.add(ret());

    return func.call<int, int>(a) == 1;
}

/** Actual instruction tests */

bool testAddWithCarry()
{
    // The carry function does rd = rd  + rm + c flag, so we need to ensure that the C flag is set
    // For ADD (1) C flag = CarryFrom(rn + imm3)
    Functor func;

    func.add(addSmallImm(Register::r0, Register::r0, 0));
    func.add(addWithCarry(Register::r0, Register::r1));
    func.add(ret());

    return func.call<int, int, int>(37, 42) == 37 + 42;
}

bool testAddWithCarryFail()
{
    setAllowedToPrintStatusFlags(false);

    resetEncodingStatusFlags();
    addWithCarry(Register::r8, Register::r0);
    bool success = !checkEncodingStatusFlags();

    resetEncodingStatusFlags();
    addWithCarry(Register::r0, Register::r8);
    success &= !checkEncodingStatusFlags();

    setAllowedToPrintStatusFlags(true);

    return success;
}

bool testAddSmallImm()
{
    Functor func;

    func.add(addSmallImm(Register::r0, Register::r0, 1));
    func.add(ret());

    return func.call<int, int>(1) == 2;
}

bool testAddSmallImmFail()
{
    setAllowedToPrintStatusFlags(false);

    resetEncodingStatusFlags();
    addSmallImm(Register::r0, Register::r0, 9);
    bool success = !checkEncodingStatusFlags();

    resetEncodingStatusFlags();
    addSmallImm(Register::r9, Register::r0, 0);
    success &= !checkEncodingStatusFlags();

    resetEncodingStatusFlags();
    addSmallImm(Register::r0, Register::r9, 0);
    success &= !checkEncodingStatusFlags();

    setAllowedToPrintStatusFlags(true);

    return success;
}

bool testAddLargeImm()
{
    const int inc = 42;
    Functor func;

    func.add(addLargeImm(Register::r0, inc));
    func.add(ret());

    int result = func.call<int, int>(0);
    return result == inc;
}

bool testAddReg()
{
    Functor func;
    func.add(addReg(Register::r0, Register::r0, Register::r1));
    func.add(ret());

    return func.call<int, int, int>(2, 3) == 5;
}

bool testAdd3Reg()
{
    Functor func;
    func.add(addReg(Register::r0, Register::r0, Register::r1));
    func.add(addReg(Register::r0, Register::r0, Register::r2));
    func.add(ret());
    return func.call<int, int, int, int>(3, 5, 7) == 15;
}

bool testAddGeneral()
{
    Functor func;
    func.add(addGeneral(Register::r0, Register::pc));
    func.add(ret());

    // The PC in ARM is always the address of the current instruction + 4
    return func.call<int, int>(0) == (int)&func.buffer()[0] + 4;
}

bool testAnd()
{
    return testArithmetic(37, 42, 37 & 42, &andBitwise);
}

bool testASRImmediate()
{
    Functor func;
    func.add(arithmeticShiftRightImm(Register::r0, Register::r0, 3));
    func.add(ret());

    return func.call<int, int>(256) == 256 / 8;
}

bool testASRRegister()
{
    return testArithmetic(12345678, 4, 12345678 >> 4, &arithmeticShiftRightRegister);
}

bool testConditionalBranchEncoding()
{
    auto x = conditionalBranch(Condition::eq, 0);
    return x == 0b1101000000000000;
}

bool testUnconditionalBranchNop()
{
    Functor func;

    func.add(unconditionalBranchNatural(1));
    func.add(ret());

    func.call<void>();

    return true;
}

bool testUnconditionalBranchSideEffect()
{
    Functor func;

    func.add(unconditionalBranchNatural(5)); // 1
    func.add(moveImmediate(Register::r0, 1));
    func.add(ret());
    func.add(moveImmediate(Register::r0, 2));
    func.add(ret());
    func.add(moveImmediate(Register::r0, 3)); // 2
    func.add(ret()); // 3

    int res = func.call<int, int>(0);
    if (res != 3) {
        printf("%d != 3\n", res);
    }

    return res == 3;
}

bool testUnconditionalBranchBackwards()
{
    Functor func;

    func.add(unconditionalBranchNatural(2)); // 1
    func.add(unconditionalBranchNatural(3)); // 4
    func.add(moveImmediate(Register::r0, 42)); // 2
    func.add(unconditionalBranchNatural(-2)); // 3
    func.add(ret()); // 5

    return func.call<int, int>(37) == 42;
}

bool testConditionalImmediateEqual()
{
    return testConditionalImmediate(37, 37, Condition::eq);
}

bool testConditionalImmediateNotEqual()
{
    return testConditionalImmediate(37, 42, Condition::ne);
}

bool testConditionalImmediateUnsignedHigherOrSame()
{
    return testConditionalImmediate(42, 37, Condition::hs);
}

bool testConditionalImmediateUnsignedLower()
{
    return testConditionalImmediate(37, 42, Condition::lo);
}

bool testConditionalImmediateMinus()
{
    return testConditionalImmediate(37, 42, Condition::mi);
}

bool testConditionalImmediatePositiveOrZero()
{
    return testConditionalImmediate(42, 42, Condition::pl);
}

bool testConditionalImmediateNoOverflow()
{
    return testConditionalImmediate(0, 0, Condition::vc);
}

bool testConditionalImmediateUnsignedHigher()
{
    return testConditionalImmediate(42, 37, Condition::hi);
}

bool testConditionalImmediateUnsignedLowerOrSame()
{
    return testConditionalImmediate(37, 37, Condition::ls);
}

bool testConditionalImmediateSignedGreaterThanOrEqual()
{
    return testConditionalImmediate(42, 37, Condition::ge);
}

bool testConditionalImmediateSignedLessThan()
{
    return testConditionalImmediate(-20, 0, Condition::lt);
}

bool testConditionalImmediateSignedGreaterThan()
{
    return testConditionalImmediate(42, 41, Condition::gt);
}

bool testConditionalImmediateSignedLessThanOrEqual()
{
    return testConditionalImmediate(42, 42, Condition::le);
}

bool testConditionalLowRegisters()
{
    Functor func;

    func.add(compareLowRegisters(Register::r0, Register::r1));
    func.add(conditionalBranchNatural(Condition::eq, 3));
    func.add(moveImmediate(Register::r0, 0));
    func.add(ret());
    func.add(moveImmediate(Register::r0, 1));
    func.add(ret());

    int res = func.call<int, int>(42, 42);
    return res == 1;
}

bool testConditionalGeneralRegisters()
{
    Functor func;

    func.add(moveGeneral(Register::r0, Register::lr));
    func.add(compareRegistersGeneral(Register::r0, Register::lr));
    func.add(conditionalBranchNatural(Condition::eq, 3));
    func.add(moveImmediate(Register::r0, 0));
    func.add(ret());
    func.add(moveImmediate(Register::r0, 1));
    func.add(ret());

    return func.call<int>() == 1;
}

bool testBitwiseClear()
{
    return testArithmetic(12345678, 87654321, 12345678 & ~(87654321), &bitClear);
}

bool testBranchLink()
{
    Functor func;
    func.add(pushMultiple(true, RegisterList::empty));
    func.add(branchAndLink(1));
    func.add(popMultiple(true, RegisterList::empty));
    func.add(moveImmediate(Register::r0, 42));
    func.add(ret());

    return func.call<int>() == 42;
}

__attribute((__section__(".data"), noinline)) void callTest(int* a)
{
    *a = 42;
}

bool testBranchLinkExchangeRegister()
{
    Functor func;

    func.add(pushMultiple(true, RegisterList::empty));
    func.add(branchLinkExchangeToRegister(Register::r1));
    func.add(popMultiple(true, RegisterList::empty));

    int a = 0;
    typedef void (*CallTestType)(int*);
    func.call<int, int*, CallTestType>(&a, &callTest);
    return a == 42;
}

bool testEor()
{
    return testArithmetic(1234, 5678, 1234 ^ 5678, &eor);
}

bool testLoadMultipleIncrementAfter()
{
    int buffer[] = { 1, 2, 3, 4 };
    Functor func;

    func.add(loadMultipleIncrementAfter(Register::r0, RegisterList::r0 | RegisterList::r1 | RegisterList::r2 | RegisterList::r3));
    func.add(addReg(Register::r0, Register::r0, Register::r1));
    func.add(addReg(Register::r0, Register::r0, Register::r2));
    func.add(addReg(Register::r0, Register::r0, Register::r3));
    func.add(ret());

    return func.call<int, int*>(&buffer[0]) == 1 + 2 + 3 + 4;
}

bool testLoadWordWithOffset()
{
    int buffer[] = { 1, 2, 3, 4 };

    Functor func;
    func.add(loadWordWithOffset(Register::r0, Register::r0, 2));
    func.add(ret());

    return func.call<int, int*>(&buffer[0]) == buffer[2];
}

bool testLoadWordWithRegisterOffset()
{
    int buffer[] = { 1, 2, 3, 4 };

    Functor func;
    func.add(loadWordWithRegisterOffset(Register::r0, Register::r0, Register::r1));
    func.add(ret());

    return func.call<int, int*, int>(&buffer[0], 8) == buffer[2];
}

bool testLoadWordWithStackPointerOffset()
{
    Functor func;
    func.add(moveImmediate(Register::r0, 7));
    func.add(pushMultiple(false, RegisterList::r0));
    func.add(loadWordWithStackPointerOffset(Register::r0, 0));
    func.add(popMultiple(false, RegisterList::r1)); // Doesn't matter where
    func.add(ret());

    return func.call<int>() == 7;
}

bool testLoadByteWithOffset()
{
    const char word[] = "hello";
    Functor func;
    func.add(loadByteWithOffset(Register::r0, Register::r0, 1));
    func.add(ret());

    return func.call<char, const char*>(&word[0]) == 'e';
}

bool testLoadByteWithRegisterOffset()
{
    const char word[] = "hello";
    Functor func;
    func.add(loadByteWithRegisterOffset(Register::r0, Register::r0, Register::r1));
    func.add(ret());

    return func.call<char, const char*, int>(&word[0], 1) == 'e';
}

bool testLoadHalfWordWithOffset()
{
    int16_t buffer[] = { 1, 2, 3, 4 };

    Functor func;
    func.add(loadHalfWordWithOffset(Register::r0, Register::r0, 2));
    func.add(ret());

    return func.call<int16_t, int16_t*>(&buffer[0]) == buffer[2];
}

bool testLoadHalfWordWithRegisterOffset()
{
    int16_t buffer[] = { 1, 2, 3, 4 };

    Functor func;
    func.add(loadHalfWordWithRegisterOffset(Register::r0, Register::r0, Register::r1));
    func.add(ret());

    return func.call<int16_t, int16_t*, int>(&buffer[0], 4) == buffer[2];
}

bool testLoadSignedByteWithRegisterOffset()
{
    int8_t a = -1;

    Functor func;
    func.add(loadSignedByteWithRegisterOffset(Register::r0, Register::r0, Register::r1));
    func.add(ret());

    return func.call<int8_t, int8_t*, int>(&a, 0) == a;
}

bool testLoadSignedHalfWordWithRegisterOffset()
{
    int16_t a = -1;

    Functor func;
    func.add(loadSignedHalfWordWithRegisterOffset(Register::r0, Register::r0, Register::r1));
    func.add(ret());

    return func.call<int16_t, int16_t*, int>(&a, 0) == a;
}

bool testLogicalShiftLeftImmediate()
{
    Functor func;
    func.add(logicalShiftLeftImmediate(Register::r0, Register::r0, 2));
    func.add(ret());

    return func.call<int, int>(42) == 42 * 4;
}

bool testLSLRegister()
{
    return testArithmetic(1234, 7, 1234 << 7, &leftShiftLogicalRegister);
}

bool testLSRRegister()
{
    return testArithmetic(1234 << 3, 3, 1234, &rightShiftLogicalRegister);
}

bool testMoveImmediate()
{
    const uint8_t x = 42;
    Functor func;

    func.add(moveImmediate(Register::r0, x));
    func.add(ret());

    int result = func.call<int, int>(0);
    return result == x;
}

bool testMoveLowToLow()
{
    const uint8_t x = 42;
    const uint8_t y = 37;
    Functor func;

    func.add(moveLowToLow(Register::r0, Register::r1));
    func.add(ret());

    int result = func.call<int, int, int>(x, y);
    return result == y;
}

bool testMoveGeneral()
{
    const uint8_t x = 42;
    const uint8_t y = 37;
    Functor func;

    func.add(moveGeneral(Register::r2, Register::r8));
    func.add(moveGeneral(Register::r8, Register::r1));
    func.add(moveGeneral(Register::r0, Register::r8));
    // Need to preserve the value in r8 but can mess with r0-3
    func.add(moveGeneral(Register::r8, Register::r2));
    func.add(ret());

    int result = func.call<int, int, int>(x, y);
    return result == y;
}

bool testMul()
{
    return testArithmetic(1234, 5678, 1234 * 5678, &mul);
}

bool testMoveNot()
{
    return testArithmetic(1234, 5678, ~5678, &moveNot);
}

bool testNegate()
{
    return testArithmetic(0, 123, -123, &neg);
}

bool testOr()
{
    return testArithmetic(123123, 456456, 123123 | 456456, &orBitwise);
}

bool testPushPopNop()
{
    Functor func;

    auto regs = RegisterList::r0 | RegisterList::r1 | RegisterList::r2 | RegisterList::r3;

    func.add(pushMultiple(false, regs));
    func.add(moveLowToLow(Register::r0, Register::r1));
    func.add(popMultiple(false, regs));
    func.add(ret());

    return func.call<int, int, int>(10, 20) == 10;
}

bool testRor()
{
    // TODO Avoid using a constant expected value here
    return testArithmetic(123123, 2, -1073711044, &ror);
}

bool testStoreMultipleIncrementAfter()
{
    int buffer[] = { 0, 0, 0, 0 };
    Functor func;

    func.add(moveImmediate(Register::r1, 1));
    func.add(moveImmediate(Register::r2, 2));
    func.add(moveImmediate(Register::r3, 3));
    func.add(moveImmediate(Register::r4, 4));
    func.add(storeMultipleIncrementAfter(Register::r0, RegisterList::r1 | RegisterList::r2 | RegisterList::r3 | RegisterList::r4));
    func.add(ret());

    int res = func.call<int, int*>(&buffer[0]);

    return res == (int)(&buffer[4]) && buffer[0] == 1 && buffer[1] == 2 && buffer[2] == 3 && buffer[3] == 4;
}

bool testStoreWordWithOffset()
{
    int a = 0;
    Functor func;
    func.add(storeWordWithOffset(Register::r0, Register::r1, 0));
    func.add(ret());
    func.call<void, int, int*>(42, &a);
    return a == 42;
}

bool testStoreWordWithRegisterOffset()
{
    int a = 0;
    Functor func;
    func.add(storeWordWithRegisterOffset(Register::r0, Register::r1, Register::r2));
    func.add(ret());
    func.call<void, int, int*, int>(42, &a, 0);
    return a == 42;
}

bool testStoreWordWithStackPointerOffset()
{
    Functor func;
    func.add(pushMultiple(false, RegisterList::r0));
    func.add(moveImmediate(Register::r0, 7));
    func.add(storeWordWithStackPointerOffset(Register::r0, 0));
    func.add(moveImmediate(Register::r0, 0));
    func.add(loadWordWithStackPointerOffset(Register::r0, 0));
    func.add(popMultiple(false, RegisterList::r1)); // Doesn't matter where
    func.add(ret());

    return func.call<int>() == 7;
}

bool testStoreByteWithOffset()
{
    int8_t a = 0;
    Functor func;
    func.add(storeByteWithOffset(Register::r0, Register::r1, 0));
    func.add(ret());
    func.call<void, int8_t, int8_t*>(42, &a);
    return a == 42;
}

bool testStoreByteWithRegisterOffset()
{
    int8_t a = 0;
    Functor func;
    func.add(storeByteWithRegisterOffset(Register::r0, Register::r1, Register::r2));
    func.add(ret());
    func.call<void, int8_t, int8_t*, int>(42, &a, 0);
    return a == 42;
}

bool testStoreHalfWordWithOffset()
{
    int16_t a = 0;
    Functor func;
    func.add(storeHalfWordWithOffset(Register::r0, Register::r1, 0));
    func.add(ret());
    func.call<void, int16_t, int16_t*>(42, &a);
    return a == 42;
}

bool testStoreHalfWordWithRegisterOffset()
{
    int16_t a = 0;
    Functor func;
    func.add(storeHalfWordWithRegisterOffset(Register::r0, Register::r1, Register::r2));
    func.add(ret());
    func.call<void, int16_t, int16_t*, int>(42, &a, 0);
    return a == 42;
}

bool testSubtractionWithCarry()
{
    Functor func;

    // Sets C flag to 0
    func.add(addSmallImm(Register::r0, Register::r0, 0));
    // SBC does rd - rm - not(c flag)
    func.add(subtractWithCarry(Register::r0, Register::r1));
    func.add(ret());

    int res = func.call<int, int, int>(37, 42);
    int expected = 37 - 42 - 1;
    return res == expected;
}

bool testSubSmallImm()
{
    Functor func;

    func.add(subSmallImm(Register::r0, Register::r0, 1));
    func.add(ret());

    return func.call<int, int>(1) == 0;
}

bool testSubLargeImm()
{
    const int inc = 42;
    Functor func;

    func.add(subLargeImm(Register::r0, inc));
    func.add(ret());

    int result = func.call<int, int>(inc * 2);
    return result == inc;
}

bool testSubReg()
{
    Functor func;
    func.add(subReg(Register::r0, Register::r0, Register::r1));
    func.add(ret());

    return func.call<int, int, int>(2, 3) == -1;
}

bool testSubSP()
{
    Functor func;
    func.add(pushMultiple(false, RegisterList::r0));
    func.add(subSP(1));
    func.add(popMultiple(false, RegisterList::r0));
    func.add(popMultiple(false, RegisterList::r0));
    func.add(ret());

    return func.call<int, int>(42) == 42;
}

bool testSignExtendByte()
{
    Functor func;
    func.add(signExtendByte(Register::r0, Register::r0));
    func.add(ret());
    return func.call<int, int>(0xFF) == -1;
}

bool testSignExtendHalfWord()
{
    Functor func;
    func.add(signExtendHalfWord(Register::r0, Register::r0));
    func.add(ret());
    return func.call<int, int>(0xFFFF) == -1;
}

/** Pseudoinstruction tests */

bool testNop()
{
    Functor func;

    func.add(nop());
    func.add(ret());

    if (func.buffer()[0] == 0x46C0) {
        func.call<void>();
        return true;
    } else {
        return false;
    }
}

bool testReturn()
{
    Functor func;
    func.add(ret());
    if (func.buffer()[0] == 0x4770) {
        func.call<void>();
        return true;
    } else {
        return false;
    }
}

int inc(int a)
{
    return a + 1;
}

bool testLongCallIntoC()
{
    Functor func;
    func.add(pushMultiple(false, RegisterList::r0));
    func.add(loadWordWithPCOffset(Register::r0, 2));
    func.add(moveGeneral(Register::ip, Register::r0));
    func.add(popMultiple(false, RegisterList::r0));
    func.add(branchAndExchange(Register::ip));
    func.add(nop());
    func.addData((int)&inc);

    return func.call<int, int>(42) == 43;
}

/**
 * Test framework
 */

bool testEncoder()
{
    bool success = true;

    printTestHeader("INSTRUCTION ENCODING AND EXECUTION TESTS");
    // Actual instruction tests are ordered by their page number in the ARM
    // reference manual
    success &= TEST(testAddWithCarry);
    success &= TEST(testAddWithCarryFail);
    success &= TEST(testAddSmallImm);
    success &= TEST(testAddSmallImmFail);
    success &= TEST(testAddLargeImm);
    success &= TEST(testAddReg);
    success &= TEST(testAdd3Reg);
    success &= TEST(testAddGeneral);
    success &= TEST(testAnd);
    success &= TEST(testASRImmediate);
    success &= TEST(testASRRegister);
    success &= TEST(testConditionalBranchEncoding);
    success &= TEST(testUnconditionalBranchNop);
    success &= TEST(testUnconditionalBranchSideEffect);
    success &= TEST(testUnconditionalBranchBackwards);
    success &= TEST(testBranchLink);
    success &= TEST(testBitwiseClear);
    success &= TEST(testConditionalImmediateEqual);
    success &= TEST(testConditionalImmediateNotEqual);
    success &= TEST(testConditionalImmediateUnsignedHigherOrSame);
    success &= TEST(testConditionalImmediateUnsignedLower);
    success &= TEST(testConditionalImmediateMinus);
    success &= TEST(testConditionalImmediatePositiveOrZero);
    // success &= TEST(testConditionalImmediateOverflow);
    success &= TEST(testConditionalImmediateNoOverflow);
    success &= TEST(testConditionalImmediateUnsignedHigher);
    success &= TEST(testConditionalImmediateUnsignedLowerOrSame);
    success &= TEST(testConditionalImmediateSignedGreaterThanOrEqual);
    success &= TEST(testConditionalImmediateSignedLessThan);
    success &= TEST(testConditionalImmediateSignedGreaterThan);
    success &= TEST(testConditionalImmediateSignedLessThanOrEqual);
    success &= TEST(testConditionalLowRegisters);
    success &= TEST(testConditionalGeneralRegisters);
    success &= TEST(testBranchLinkExchangeRegister);
    success &= TEST(testEor);
    success &= TEST(testLoadMultipleIncrementAfter);
    success &= TEST(testLoadWordWithOffset);
    success &= TEST(testLoadWordWithRegisterOffset);
    success &= TEST(testLoadWordWithStackPointerOffset);
    success &= TEST(testLoadByteWithOffset);
    success &= TEST(testLoadByteWithRegisterOffset);
    success &= TEST(testLoadHalfWordWithOffset);
    success &= TEST(testLoadHalfWordWithRegisterOffset);
    success &= TEST(testLoadSignedByteWithRegisterOffset);
    success &= TEST(testLoadSignedHalfWordWithRegisterOffset);
    success &= TEST(testLogicalShiftLeftImmediate);
    success &= TEST(testLSLRegister);
    success &= TEST(testLSRRegister);
    success &= TEST(testMoveImmediate);
    success &= TEST(testMoveLowToLow);
    success &= TEST(testMoveGeneral);
    success &= TEST(testMul);
    success &= TEST(testMoveNot);
    success &= TEST(testNegate);
    success &= TEST(testOr);
    success &= TEST(testRor);
    success &= TEST(testPushPopNop);
    success &= TEST(testStoreMultipleIncrementAfter);
    success &= TEST(testStoreWordWithOffset);
    success &= TEST(testStoreWordWithRegisterOffset);
    success &= TEST(testStoreWordWithStackPointerOffset);
    success &= TEST(testStoreByteWithOffset);
    success &= TEST(testStoreByteWithRegisterOffset);
    success &= TEST(testStoreHalfWordWithOffset);
    success &= TEST(testStoreHalfWordWithRegisterOffset);
    success &= TEST(testSubtractionWithCarry);
    success &= TEST(testSubSmallImm);
    success &= TEST(testSubLargeImm);
    success &= TEST(testSubReg);
    success &= TEST(testSubSP);
    success &= TEST(testSignExtendByte);
    success &= TEST(testSignExtendHalfWord);

    printTestHeader("PSEUDOINSTRUCTION TESTS");

    // Pseudoinstruction tests are ordered alphabetically
    success &= TEST(testNop);
    success &= TEST(testReturn);
    success &= TEST(testLongCallIntoC);

    return success;
}
}