#include "Tests.h"

#include "ARM/Decoder.h"
#include "ARM/Encoder.h"
#include "Bit/Bit.h"
#include "Tests/Utilities.h"
#include <cstdio>
#include <cstring>

namespace ARM {

bool testDecoder(Instruction result, const char* expected)
{
    char actual[32] = { 0 };
    decode(result, actual);
    if (strcmp(expected, actual) == 0) {
        incSuccess();
        printf("\u2705 %s\n", expected);
        return true;
    } else {
        incFailure();
        printf("\u274C '%s'\t'%s'\n", expected, actual);
        return false;
    }
}

bool testDecoder(InstructionPair pair, const char* expected)
{
    char actual[32] = { 0 };
    decodeBranchLong(pair.instruction1, pair.instruction2, actual, nullptr);
    if (strcmp(expected, actual) == 0) {
        incSuccess();
        printf("\u2705 %s\n", expected);
        return true;
    } else {
        incFailure();
        printf("\u274C '%s'\t'%s'\n", expected, actual);
        return false;
    }
}

bool testDecoder()
{
    bool res = true;

    printTestHeader("INSTRUCTION DECODING TESTS");

    res &= testDecoder(addWithCarry(Register::r0, Register::r1), "adc r0, r1");
    res &= testDecoder(addSmallImm(Register::r0, Register::r1, 7), "add r0, r1, #7");
    res &= testDecoder(addLargeImm(Register::r3, 42), "add r3, #42");
    res &= testDecoder(addReg(Register::r0, Register::r2, Register::r7), "add r0, r2, r7");
    res &= testDecoder(addGeneral(Register::r8, Register::r15), "add r8, pc");
    res &= testDecoder(andBitwise(Register::r0, Register::r1), "and r0, r1");
    res &= testDecoder(arithmeticShiftRightImm(Register::r0, Register::r1, 17), "asr r0, r1, #17");
    res &= testDecoder(arithmeticShiftRightRegister(Register::r0, Register::r1), "asr r0, r1");
    res &= testDecoder(conditionalBranch(Condition::eq, 0), "beq 00000000 /* by 0 */");
    res &= testDecoder(conditionalBranch(Condition::ne, 1), "bne 00000002 /* by 2 */");
    res &= testDecoder(conditionalBranch(Condition::cs, 2), "bcs 00000004 /* by 4 */");
    res &= testDecoder(conditionalBranch(Condition::cc, 3), "bcc 00000006 /* by 6 */");
    res &= testDecoder(conditionalBranch(Condition::mi, 4), "bmi 00000008 /* by 8 */");
    res &= testDecoder(conditionalBranch(Condition::pl, 5), "bpl 0000000a /* by 10 */");
    res &= testDecoder(conditionalBranch(Condition::vs, 6), "bvs 0000000c /* by 12 */");
    res &= testDecoder(conditionalBranch(Condition::vc, -1), "bvc fffffffe /* by -2 */");
    res &= testDecoder(conditionalBranch(Condition::hi, -2), "bhi fffffffc /* by -4 */");
    res &= testDecoder(conditionalBranch(Condition::ls, -3), "bls fffffffa /* by -6 */");
    res &= testDecoder(conditionalBranch(Condition::ge, -4), "bge fffffff8 /* by -8 */");
    res &= testDecoder(conditionalBranch(Condition::lt, -5), "blt fffffff6 /* by -10 */");
    res &= testDecoder(conditionalBranch(Condition::gt, -6), "bgt fffffff4 /* by -12 */");
    res &= testDecoder(conditionalBranch(Condition::le, -7), "ble fffffff2 /* by -14 */");
    res &= testDecoder(unconditionalBranch(0), "b 00000000 /* by 0 */");
    res &= testDecoder(unconditionalBranch(-1), "b fffffffe /* by -2 */");
    res &= testDecoder(unconditionalBranch(10), "b 00000014 /* by 20 */");
    res &= testDecoder(bitClear(Register::r0, Register::r1), "bic r0, r1");
    res &= testDecoder(branchAndLink(0), "bl 00000004 /* by 0 */");
    res &= testDecoder(branchAndLink(-1), "bl 00000002 /* by -1 */");
    res &= testDecoder(branchLinkAndExchange(0), "blx 00000004 /* by 0 */");
    res &= testDecoder(branchLinkAndExchange(42), "blx 00000058 /* by 42 */");
    res &= testDecoder(branchLinkExchangeToRegister(Register::pc), "blx pc");
    res &= testDecoder(branchAndExchange(Register::pc), "bx pc");
    res &= testDecoder(branchAndExchange(Register::lr), "ret");
    res &= testDecoder(compareImmediate(Register::r0, 42), "cmp r0, #42");
    res &= testDecoder(compareLowRegisters(Register::r0, Register::r1), "cmp r0, r1");
    res &= testDecoder(compareRegistersGeneral(Register::r0, Register::pc), "cmp r0, pc");
    res &= testDecoder(eor(Register::r0, Register::r1), "eor r0, r1");
    res &= testDecoder(loadMultipleIncrementAfter(Register::r0, RegisterList::r0 | RegisterList::r3 | RegisterList::r7), "ldmia r0!, r0, r3, r7");
    res &= testDecoder(loadWordWithOffset(Register::r0, Register::r1, 5), "ldr r0, [r1, #5 * 4]");
    res &= testDecoder(loadWordWithRegisterOffset(Register::r0, Register::r0, Register::r1), "ldr r0, [r0, r1]");
    res &= testDecoder(loadWordWithPCOffset(Register::r0, 4), "ldr r0, [pc, #4 * 4] /* 00000010 */");
    res &= testDecoder(loadWordWithStackPointerOffset(Register::r7, 42), "ldr r7, [sp, #42 * 4]");
    res &= testDecoder(loadByteWithOffset(Register::r0, Register::r1, 8), "ldrb r0, [r1, #8]");
    res &= testDecoder(loadByteWithRegisterOffset(Register::r0, Register::r3, Register::r7), "ldrb r0, [r3, r7]");
    res &= testDecoder(loadHalfWordWithOffset(Register::r0, Register::r1, 31), "ldrh r0, [r1, #31 * 2]");
    res &= testDecoder(loadHalfWordWithRegisterOffset(Register::r0, Register::r3, Register::r7), "ldrh r0, [r3, r7]");
    res &= testDecoder(loadSignedByteWithRegisterOffset(Register::r0, Register::r3, Register::r7), "ldrsb r0, [r3, r7]");
    res &= testDecoder(loadSignedHalfWordWithRegisterOffset(Register::r0, Register::r3, Register::r7), "ldrsh r0, [r3, r7]");
    res &= testDecoder(logicalShiftLeftImmediate(Register::r3, Register::r7, 27), "lsl r3, r7, #27");
    res &= testDecoder(leftShiftLogicalRegister(Register::r0, Register::r1), "lsl r0, r1");
    res &= testDecoder(rightShiftLogicalRegister(Register::r0, Register::r1), "lsr r0, r1");
    res &= testDecoder(moveImmediate(Register::r0, 42), "mov r0, #42");
    res &= testDecoder(moveLowToLow(Register::r0, Register::r7), "mov r0, r7");
    res &= testDecoder(moveGeneral(Register::r0, Register::lr), "mov r0, lr");
    res &= testDecoder(mul(Register::r0, Register::r3), "mul r0, r3");
    res &= testDecoder(moveNot(Register::r0, Register::r3), "mvn r0, r3");
    res &= testDecoder(neg(Register::r0, Register::r3), "neg r0, r3");
    res &= testDecoder(orBitwise(Register::r0, Register::r3), "orr r0, r3");
    res &= testDecoder(popMultiple(true, RegisterList::empty), "pop pc");
    res &= testDecoder(popMultiple(false, RegisterList::r0 | RegisterList::r3 | RegisterList::r7), "pop r0, r3, r7");
    res &= testDecoder(popMultiple(true, RegisterList::r0 | RegisterList::r3 | RegisterList::r7), "pop pc, r0, r3, r7");
    res &= testDecoder(pushMultiple(true, RegisterList::empty), "push lr");
    res &= testDecoder(pushMultiple(false, RegisterList::r0 | RegisterList::r3 | RegisterList::r7), "push r0, r3, r7");
    res &= testDecoder(pushMultiple(true, RegisterList::r0 | RegisterList::r3 | RegisterList::r7), "push lr, r0, r3, r7");
    res &= testDecoder(ror(Register::r0, Register::r3), "ror r0, r3");
    res &= testDecoder(subtractWithCarry(Register::r0, Register::r3), "sbc r0, r3");
    res &= testDecoder(storeMultipleIncrementAfter(Register::r0, RegisterList::r3 | RegisterList::r4 | RegisterList::r7), "stmia r0!, r3, r4, r7");
    res &= testDecoder(storeWordWithOffset(Register::r0, Register::r3, 31), "str r0, [r3, #31 * 4]");
    res &= testDecoder(storeWordWithRegisterOffset(Register::r0, Register::r3, Register::r7), "str r0, [r3, r7]");
    res &= testDecoder(storeWordWithStackPointerOffset(Register::r0, 42), "str r0, [sp, #42 * 4]");
    res &= testDecoder(storeByteWithOffset(Register::r0, Register::r3, 31), "strb r0, [r3, #31]");
    res &= testDecoder(storeByteWithRegisterOffset(Register::r0, Register::r3, Register::r7), "strb r0, [r3, r7]");
    res &= testDecoder(storeHalfWordWithOffset(Register::r0, Register::r3, 17), "strh r0, [r3, #17 * 2]");
    res &= testDecoder(storeHalfWordWithRegisterOffset(Register::r0, Register::r1, Register::r2), "strh r0, [r1, r2]");
    res &= testDecoder(subSmallImm(Register::r0, Register::r1, 7), "sub r0, r1, #7");
    res &= testDecoder(subLargeImm(Register::r3, 42), "sub r3, #42");
    res &= testDecoder(subReg(Register::r3, Register::r0, Register::r7), "sub r3, r0, r7");
    res &= testDecoder(subSP(101), "sub sp, #101 * 4");
    res &= testDecoder(signExtendByte(Register::r0, Register::r1), "sxtb r0, r1");
    res &= testDecoder(signExtendHalfWord(Register::r0, Register::r1), "sxth r0, r1");
    res &= testDecoder(nop(), "nop");

    return res;
}
}