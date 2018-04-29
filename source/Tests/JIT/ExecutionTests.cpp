#include "Tests.h"

#include "ARM/Decoder.h"
#include "Device/MicroBitDevice.h"
#include "Device/OptionalInstructions.h"
#include "JIT/Compiler.h"
#include "JIT/DynamicCompilation.h"
#include "JIT/Interpreter.h"
#include "MarksExecutionTests.h"
#include "Support/Memory.h"
#include "Tests/Utilities.h"
#include <cstddef>
#include <cstdio>

namespace JIT {

static const int MAX_STACK_DEPTH = 128;
static const size_t NumberOfCanaryValues = 4;

class CodeTest {
public:
    // If in fuzzed mode the stack will be cleared before this is called and filled with the fuzz test values
    virtual void preTest(Environment::VM& state) = 0;
    virtual bool postTest(Environment::VM& state) = 0;

    CodeTest(const Code::Instruction* code, int32_t length)
        : stack(stackStorage, sizeof(stackStorage) / sizeof(int32_t))
        , code(code, length)
        , m_inCanaryMode(false)
    {
    }

    bool test();

    int32_t stackStorage[MAX_STACK_DEPTH];
    Environment::Stack stack;
    Code::Array code;

    /**
     * The canary mode will run the same test but automatically insert a small number of values
     * on the stack before the test is configured. Then, after the test executes it verifies
     * that these canary values are still in the correct order on the stack.
     *
     * The only changes required to implement canary tests are to check the number of values on the stack
     * is numberOfCanaryValues() + expected rather than just expected and to use clearIfNotInCanaryMode to clear the stack
     * in preTest.
     *
     * To run a canary test use the CANARY_CODE_TEST and CANARY_OP_TEST macros
     */
    CodeTest& canaryMode();

    size_t numberOfCanaryValues();

    void clearIfNotInCanaryMode(Environment::VM& state);

private:
    bool correctnessTest();
    void performanceTest();

    bool m_inCanaryMode;

    void configureCanaryValues(Environment::VM& state);
    bool verifyCanaryValuesAreInTact(Environment::VM& state);

    int canaryValueForIndex(int index);
};

CodeTest& CodeTest::canaryMode()
{
    m_inCanaryMode = true;
    return *this;
}

void CodeTest::configureCanaryValues(Environment::VM& state)
{
    if (m_inCanaryMode) {
        state.m_stack.clear();
        for (size_t i = 0; i < NumberOfCanaryValues; ++i) {
            state.m_stack.push(canaryValueForIndex(i));
        }
    }
}

void CodeTest::clearIfNotInCanaryMode(Environment::VM& state)
{
    if (!m_inCanaryMode) {
        state.m_stack.clear();
    }
}

size_t CodeTest::numberOfCanaryValues()
{
    return m_inCanaryMode ? NumberOfCanaryValues : 0;
}

int CodeTest::canaryValueForIndex(int index)
{
    // This can be arbitrary
    return (2 * index + 1) * (index + 3);
}

bool CodeTest::verifyCanaryValuesAreInTact(Environment::VM& state)
{
    if (m_inCanaryMode) {
        if (state.m_stack.size() < NumberOfCanaryValues) {
            return false;
        }

        auto len = state.m_stack.size();

        for (size_t i = 0; i < NumberOfCanaryValues; ++i) {
            if (state.m_stack.peek((int32_t)(len - i - 1)) != canaryValueForIndex((int)i)) {
                return false;
            }
        }
    }

    return true;
}

bool CodeTest::test()
{
    if (ProfilingEnabled) {
        performanceTest();
        return true;
    } else {
        return correctnessTest();
    }
}

bool CodeTest::correctnessTest()
{
    Environment::VM state(stack, code);
    state.m_compileOrInterpretFunction = (Environment::VMFunction)&JIT::compileFunctionDynamically;

    int32_t* originalBase = state.m_stack.m_base;
    int32_t* originalEnd = state.m_stack.m_end;

    configureCanaryValues(state);
    preTest(state);
    execute(&state);
    bool interpretSuccess = verifyCanaryValuesAreInTact(state) && postTest(state);

    if (!interpretSuccess) {
        printf("Interpreter failure (state is %s) at %ld, end stack state:\n", VMStatusString(state.m_status), state.m_programCounter);
        state.m_stack.print();
    }

    configureCanaryValues(state);
    preTest(state);
    state.reset();
    ARM::Functor func;

    // Strictly this doesn't need to be a unique pointer but I might later take advantage of that
    auto compiler = Support::make_unique<JIT::Compiler>(state.m_code, &Device::MicroBitDevice::singleton());
    state.m_compiler = compiler.get();
    auto result = compiler->compile(func);

    bool compileSuccess = false;
    bool invariantsHold = false;

    if (result == Compiler::Status::Success) {
        if (AlwaysPrintCompilation) {
            printf("Compiled code:\n");
            compiler->prettyPrintCode(func);
        }

        auto newState = state.call(func);

        invariantsHold = &state == newState && originalBase == state.m_stack.m_base && originalEnd == state.m_stack.m_end;
        if (!invariantsHold) {
            printf("Invariants do not hold, these should be equal:\n");
            printf("State pointer:  %p %p\n", (void*)&state, (void*)&newState);
            printf("Base pointer:   %p %p\n", (void*)originalBase, (void*)state.m_stack.m_base);
            printf("End pointer:    %p %p\n", (void*)originalEnd, (void*)state.m_stack.m_end);
        }

        compileSuccess = verifyCanaryValuesAreInTact(state) && postTest(state);

        if (!compileSuccess) {
            printf("Execution code status = %s(%08x)\n", Environment::VMStatusString(state.m_status), (int)newState->m_errorPC);
            if (!AlwaysPrintCompilation) {
                printf("Compiled code:\n");
                compiler->prettyPrintCode(func);
            }

            printf("End stack state (depth %d):\n", (int)state.m_stack.size());
            state.m_stack.print();

            printf("Original Stack code:\n");
            state.m_code.print();
        }
    } else {
        printf("Compilation error: %s\n", Compiler::statusString(result));
    }

    return interpretSuccess && invariantsHold && compileSuccess;
}

void CodeTest::performanceTest()
{
    Environment::VM state(stack, code);
    state.m_compileOrInterpretFunction = (Environment::VMFunction)&JIT::compileFunctionDynamically;

    // Round 1a: Timing for interpreter set up
    Device::sendStartTimingInterpreterSignal();
    for (int i = 0; i < TestExecutionCount; ++i) {
        state.reset();
        configureCanaryValues(state);
        preTest(state);
    }
    Device::sendEndTimingInterpreterSignal();

    // Round 1b: Actually time the interpreter
    Device::sendStartTimingInterpreterSignal();
    for (int i = 0; i < TestExecutionCount; ++i) {
        state.reset();
        configureCanaryValues(state);
        preTest(state);
        execute(&state);
    }
    Device::sendEndTimingInterpreterSignal();

    // Do the compilation itself
    ARM::Functor func;
    auto compiler = Support::make_unique<JIT::Compiler>(state.m_code, &Device::MicroBitDevice::singleton());
    state.m_compiler = compiler.get();
    auto result = compiler->compile(func);

    // Round 2a: Timing for compiler set up
    Device::sendStartTimingCompilerSignal();
    for (int i = 0; i < TestExecutionCount; ++i) {
        state.reset();
        configureCanaryValues(state);
        preTest(state);
    }
    Device::sendEndTimingCompilerSignal();

    // Round 2b: Actual time for compiled code
    Device::sendStartTimingCompilerSignal();
    for (int i = 0; i < TestExecutionCount; ++i) {
        state.reset();
        configureCanaryValues(state);
        preTest(state);
        state.call(func);
    }
    Device::sendEndTimingCompilerSignal();
}

#define CODE_TEST(NAME) (printResult(NAME().test(), #NAME))
#define CANARY_CODE_TEST(NAME) (printResult(NAME().canaryMode().test(), "Canary" #NAME))
#define OP_TEST(NAME, INSTANCE) (printResult(INSTANCE.test(), NAME))
#define CANARY_OP_TEST(NAME, INSTANCE) (printResult(INSTANCE.canaryMode().test(), "Canary" NAME))

class EmptyCodeTest : public CodeTest {
public:
    EmptyCodeTest()
        : CodeTest(nullptr, 0)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == numberOfCanaryValues();
    }
};

class SingleInstructionTest : public CodeTest {
public:
    SingleInstructionTest(Code::Instruction instr)
        : CodeTest((const Code::Instruction*)&instruction, 1)
    {
        instruction = instr;
    }

    virtual void preTest(Environment::VM& state) = 0;
    virtual bool postTest(Environment::VM& state) = 0;

private:
    Code::Instruction instruction;
};

class SingleOperatorTest : public SingleInstructionTest {
public:
    SingleOperatorTest(Code::Instruction instr, int a, int b, int expected)
        : SingleInstructionTest(instr)
        , a(a)
        , b(b)
        , expected(expected)
        , hasTwoOperands(true)
    {
    }

    SingleOperatorTest(Code::Instruction instr, int a, int expected)
        : SingleInstructionTest(instr)
        , a(a)
        , b(0)
        , expected(expected)
        , hasTwoOperands(false)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(a);
        if (hasTwoOperands) {
            state.m_stack.push(b);
        }
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == expected;
    }

private:
    int a, b, expected;
    bool hasTwoOperands;
};

class SingleOperatorPushTest : public CodeTest {
public:
    SingleOperatorPushTest(Code::Instruction instr, int8_t a, int8_t b, int expected)
        : CodeTest(&m_code[0], sizeof(m_code) / sizeof(Code::Instruction))
        , m_code{ Code::Instruction::Push8, (Code::Instruction)a, Code::Instruction::Push8, (Code::Instruction)b, instr }
        , m_expected(expected)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == m_expected;
    }

private:
    Code::Instruction m_code[5];
    int m_expected;
};

/**
 * Performs a OP1 (b OP2 c)
 */
class TwoOperatorTest : public CodeTest {
public:
    TwoOperatorTest(Code::Instruction op1, Code::Instruction op2, int a, int b, int c, int expected)
        : CodeTest(&m_code[0], 2)
        , m_code{ op1, op2 }
        , m_operands{ a, b, c }
        , m_expected(expected)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(m_operands[0]);
        state.m_stack.push(m_operands[1]);
        state.m_stack.push(m_operands[2]);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == m_expected;
    }

private:
    Code::Instruction m_code[2];
    int m_operands[3];
    int m_expected;
};

class DupTest : public SingleInstructionTest {
public:
    DupTest()
        : SingleInstructionTest(Code::Instruction::Dup)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(1);
        state.m_stack.push(2);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 3 + numberOfCanaryValues() && state.m_stack.peek() == 2 && state.m_stack.peek(1) == 2 && state.m_stack.peek(2) == 1;
    }
};

const static Code::Instruction dupManyCode[] = {
    Code::Instruction::Dup,
    Code::Instruction::Dup,
    Code::Instruction::Dup,
    Code::Instruction::Dup,
    Code::Instruction::Dup,

    Code::Instruction::Dup,
    Code::Instruction::Dup,
    Code::Instruction::Dup,
    Code::Instruction::Dup,
    Code::Instruction::Dup,

    Code::Instruction::Push8, (Code::Instruction)37,
    Code::Instruction::Dup,
    Code::Instruction::Dup,
    Code::Instruction::Dup,
    Code::Instruction::Dup
};
class DupManyTest : public CodeTest {
public:
    DupManyTest()
        : CodeTest(dupManyCode, sizeof(dupManyCode) / sizeof(Code::Instruction))
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(42);
    }

    bool postTest(Environment::VM& state)
    {
        if (state.m_stack.size() == 1 + 15 + numberOfCanaryValues()) {
            for (int i = 0; i < 5; ++i) {
                if (state.m_stack.peek(i) != 37) {
                    return false;
                }
            }
            for (int i = 5; i < 16; ++i) {
                if (state.m_stack.peek(i) != 42) {
                    return false;
                }
            }
        }
        return true;
    }
};

class NdupTest : public SingleInstructionTest {
public:
    NdupTest()
        : SingleInstructionTest(Code::Instruction::Ndup)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(37);
        state.m_stack.push(42);
        state.m_stack.push(2);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 3 + numberOfCanaryValues() && state.m_stack.peek(0) == 37 && state.m_stack.peek(1) == 42 && state.m_stack.peek(2) == 37;
    }
};

class SwapTest : public SingleInstructionTest {
public:
    SwapTest()
        : SingleInstructionTest(Code::Instruction::Swap)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(37);
        state.m_stack.push(42);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 2 + numberOfCanaryValues() && state.m_stack.peek(0) == 37 && state.m_stack.peek(1) == 42;
    }
};

class RotTest : public SingleInstructionTest {
public:
    RotTest()
        : SingleInstructionTest(Code::Instruction::Rot)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(1);
        state.m_stack.push(2);
        state.m_stack.push(3);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 3 + numberOfCanaryValues() && state.m_stack.peek(0) == 1 && state.m_stack.peek(1) == 3 && state.m_stack.peek(2) == 2;
    }
};

class NrotTest : public SingleInstructionTest {
public:
    NrotTest()
        : SingleInstructionTest(Code::Instruction::Nrot)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(0);
        state.m_stack.push(1);
        state.m_stack.push(2);
        state.m_stack.push(3);
        state.m_stack.push(3);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 4 + numberOfCanaryValues() && state.m_stack.peek(0) == 1 && state.m_stack.peek(1) == 3 && state.m_stack.peek(2) == 2 && state.m_stack.peek(3) == 0;
    }
};

class TuckTest : public SingleInstructionTest {
public:
    TuckTest()
        : SingleInstructionTest(Code::Instruction::Tuck)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(1);
        state.m_stack.push(2);
        state.m_stack.push(3);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 3 + numberOfCanaryValues() && state.m_stack.peek(0) == 2 && state.m_stack.peek(1) == 1 && state.m_stack.peek(2) == 3;
    }
};

class NtuckTest : public SingleInstructionTest {
public:
    NtuckTest()
        : SingleInstructionTest(Code::Instruction::Ntuck)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(0);
        state.m_stack.push(1);
        state.m_stack.push(2);
        state.m_stack.push(3);
        state.m_stack.push(3);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 4 + numberOfCanaryValues() && state.m_stack.peek(0) == 2 && state.m_stack.peek(1) == 1 && state.m_stack.peek(2) == 3 && state.m_stack.peek(3) == 0;
    }
};

class SizeTest : public SingleInstructionTest {
public:
    SizeTest()
        : SingleInstructionTest(Code::Instruction::Size)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(0);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 2 + numberOfCanaryValues() && state.m_stack.peek(0) == (int32_t)(1 + numberOfCanaryValues()) && state.m_stack.peek(1) == 0;
    }
};

class RandomTest : public SingleInstructionTest {
public:
    RandomTest()
        : SingleInstructionTest(Code::Instruction::Nrnd)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(10);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() >= 0 && state.m_stack.peek() < 10;
    }
};

class Push8Test : public CodeTest {
public:
    Push8Test(int8_t value)
        : CodeTest((const Code::Instruction*)instructions, 2)
        , value(value)
    {
        instructions[0] = Code::Instruction::Push8;
        instructions[1] = (Code::Instruction)value;
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == value;
    }

private:
    Code::Instruction instructions[2];
    int8_t value;
};

static const Code::Instruction manyPushes[] = {
    Code::Instruction::Push8,
    (Code::Instruction)42,
    Code::Instruction::Push8,
    (Code::Instruction)-42,
    Code::Instruction::Push8,
    (Code::Instruction)42,
    Code::Instruction::Push8,
    (Code::Instruction)-42,
    Code::Instruction::Push8,
    (Code::Instruction)42,
    Code::Instruction::Push8,
    (Code::Instruction)-42,
    Code::Instruction::Push8,
    (Code::Instruction)42,
    Code::Instruction::Push8,
    (Code::Instruction)-42,
    Code::Instruction::Push8,
    (Code::Instruction)42,
    Code::Instruction::Push8,
    (Code::Instruction)-42,
    Code::Instruction::Add,
    Code::Instruction::Add,
    Code::Instruction::Add,
    Code::Instruction::Add,
    Code::Instruction::Add,
    Code::Instruction::Add,
    Code::Instruction::Add,
    Code::Instruction::Add,
    Code::Instruction::Add,
};
class Push8ManyTest : public CodeTest {
public:
    Push8ManyTest()
        : CodeTest(manyPushes, sizeof(manyPushes) / sizeof(Code::Instruction))
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == 0;
    }
};

class Push16Test : public CodeTest {
public:
    Push16Test(int16_t value)
        : CodeTest((const Code::Instruction*)instructions, 3)
        , value(value)
    {
        instructions[0] = Code::Instruction::Push16;
        instructions[1] = (Code::Instruction)((unsigned)value & 0xFF);
        instructions[2] = (Code::Instruction)((unsigned)value >> 8);
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == value;
    }

private:
    Code::Instruction instructions[3];
    int16_t value;
};

class WaitTest : public SingleInstructionTest {
public:
    WaitTest()
        : SingleInstructionTest(Code::Instruction::Wait)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(10);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == numberOfCanaryValues();
    }
};

static const Code::Instruction jumpInstructions[] = { Code::Instruction::Push8, (Code::Instruction)5, Code::Instruction::Jmp,
    Code::Instruction::Push8, (Code::Instruction)2,
    Code::Instruction::Push8, (Code::Instruction)1 };
class JumpTest : public CodeTest {
public:
    JumpTest()
        : CodeTest(jumpInstructions, 7)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == 1;
    }
};

static const Code::Instruction cJumpInstructions[] = { Code::Instruction::Push8, (Code::Instruction)5, Code::Instruction::Cjmp,
    Code::Instruction::Push8, (Code::Instruction)42,
    Code::Instruction::Push8, (Code::Instruction)37 };
class CjmpTest : public CodeTest {
public:
    CjmpTest(bool shouldJump)
        : CodeTest(cJumpInstructions, 7)
        , shouldJump(shouldJump)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(shouldJump ? 1 : 0);
    }

    bool postTest(Environment::VM& state)
    {
        if (shouldJump) {
            return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == 37;
        } else {
            return state.m_stack.size() == 2 + numberOfCanaryValues() && state.m_stack.peek(1) == 42 && state.m_stack.peek() == 37;
        }
    }

private:
    bool shouldJump;
};

static const Code::Instruction backwardsCjumpInstructions[] = {
    (Code::Instruction)0x18, (Code::Instruction)0x06,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x18, (Code::Instruction)0x2A,
    (Code::Instruction)0x20,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x0A,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x20
};
class CjmpBackwardsTest : public CodeTest {
public:
    CjmpBackwardsTest()
        : CodeTest(backwardsCjumpInstructions, sizeof(backwardsCjumpInstructions) / sizeof(Code::Instruction))
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == 42;
    }
};

static const Code::Instruction fetchInstructions[] = {
    Code::Instruction::Push8, (Code::Instruction)4,
    Code::Instruction::Fetch,
    Code::Instruction::Halt,
    (Code::Instruction)0xFF, (Code::Instruction)0xFF
};
class FetchTest : public CodeTest {
public:
    FetchTest()
        : CodeTest(fetchInstructions, 6)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == -1;
    }
};

static const Code::Instruction functionTestInstructions[] = {
    // 0: Call 4()
    Code::Instruction::Push8, (Code::Instruction)4, Code::Instruction::Call,
    // 3
    Code::Instruction::Halt,
    // 4: Mul by 2
    Code::Instruction::Push8, (Code::Instruction)2, Code::Instruction::Mul,
    // 7: Ret
    Code::Instruction::Ret
};
class FunctionTest : public CodeTest {
public:
    FunctionTest()
        : CodeTest(functionTestInstructions, 8)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(42);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == 84;
    }
};

static const Code::Instruction haltCode[] = { Code::Instruction::Halt };
class HaltTest : public CodeTest {
public:
    HaltTest()
        : CodeTest(haltCode, 1)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    bool postTest(Environment::VM& state)
    {
        return true;
    }
#pragma GCC diagnostic pop
};

static const Code::Instruction haltInFunctionCall[] = {
    // 0:
    Code::Instruction::Push8, (Code::Instruction)4, Code::Instruction::Call,
    // 3
    Code::Instruction::Ret,
    // 4
    Code::Instruction::Push8, (Code::Instruction)8, Code::Instruction::Call,
    // 7
    Code::Instruction::Ret,
    // 8
    Code::Instruction::Halt
};
class RecursiveHaltTest : public CodeTest {
public:
    RecursiveHaltTest()
        : CodeTest(haltInFunctionCall, sizeof(haltInFunctionCall) / sizeof(Code::Instruction))
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
    bool postTest(Environment::VM& state)
    {
        return true;
    }
#pragma GCC diagnostic pop
};

static const Code::Instruction haltSideEffectCode[] = {
    // 0
    Code::Instruction::Push8, (Code::Instruction)1,
    // 2
    Code::Instruction::Push8, (Code::Instruction)2,
    // 4
    Code::Instruction::Push8, (Code::Instruction)3,
    // 6
    Code::Instruction::Push8, (Code::Instruction)4,
    // 8
    Code::Instruction::Push8, (Code::Instruction)4,
    // 10
    Code::Instruction::Inc,
    // 11
    Code::Instruction::Halt
};
class HaltSideEffectTest : public CodeTest {
public:
    HaltSideEffectTest()
        : CodeTest(haltSideEffectCode, sizeof(haltSideEffectCode) / sizeof(Code::Instruction))
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 5 + numberOfCanaryValues() && state.m_stack.peek(0) == 5 && state.m_stack.peek(1) == 4 && state.m_stack.peek(2) == 3 && state.m_stack.peek(3) == 2 && state.m_stack.peek(4) == 1;
    }
};

static const Code::Instruction boundedRecursionCode[] = {
    // 0: Call 4
    Code::Instruction::Push8, (Code::Instruction)4, Code::Instruction::Call,
    // 3: Return
    Code::Instruction::Ret,
    // 4: Call 8
    Code::Instruction::Push8, (Code::Instruction)8, Code::Instruction::Call,
    // 7: Return
    Code::Instruction::Ret,
    // 8: Call 12
    Code::Instruction::Push8, (Code::Instruction)12, Code::Instruction::Call,
    // 11: Return
    Code::Instruction::Ret,
    // 12: Call 16
    Code::Instruction::Push8, (Code::Instruction)16, Code::Instruction::Call,
    // 15: Return
    Code::Instruction::Ret,
    // 16: Call 20
    Code::Instruction::Push8, (Code::Instruction)20, Code::Instruction::Call,
    // 19: Return
    Code::Instruction::Ret,
    // 20: Call 24
    Code::Instruction::Push8, (Code::Instruction)24, Code::Instruction::Call,
    // 23: Return
    Code::Instruction::Ret,
    // 24: Return
    Code::Instruction::Ret
};
class BoundedRecursionTest : public CodeTest {
public:
    BoundedRecursionTest()
        : CodeTest(boundedRecursionCode, sizeof(boundedRecursionCode) / sizeof(Code::Instruction))
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == numberOfCanaryValues();
    }
};

static const Code::Instruction unboundedRecursionCode[] = {
    // 0: Decrement
    Code::Instruction::Dec,
    // 1: Duplicate the top entry on the stack
    Code::Instruction::Dup,
    // 2: Compare with 0
    Code::Instruction::Push8,
    (Code::Instruction)0,
    Code::Instruction::Eq,
    // 5: Conditional jump
    Code::Instruction::Push8,
    (Code::Instruction)11,
    Code::Instruction::Cjmp,
    // 8: Call 0
    Code::Instruction::Push8,
    (Code::Instruction)0,
    Code::Instruction::Call,
    // 11: Return
    Code::Instruction::Ret
};
class UnboundedRecursionTest : public CodeTest {
public:
    UnboundedRecursionTest()
        : CodeTest(unboundedRecursionCode, sizeof(unboundedRecursionCode) / sizeof(Code::Instruction))
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(20);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == 0;
    }
};

int fib(int n)
{
    int a = 0, b = 1;
    while (n != 0) {
        int c = a + b;
        a = b;
        b = c;
        n--;
    }
    return a;
}

static const Code::Instruction recursiveFibonacciCode[] = {
    // 0
    Code::Instruction::Push8, (Code::Instruction)15,
    // 2
    Code::Instruction::Push8, (Code::Instruction)6,
    // 4
    Code::Instruction::Call,
    // 5
    Code::Instruction::Halt,
    // 6
    Code::Instruction::Dup,
    // 7
    Code::Instruction::Push8, (Code::Instruction)1,
    // 9
    Code::Instruction::Gt,
    // 10
    Code::Instruction::Push8, (Code::Instruction)14,
    // 12
    Code::Instruction::Cjmp,
    // 13
    Code::Instruction::Ret,
    // 14
    Code::Instruction::Dup,
    // 15
    Code::Instruction::Push8, (Code::Instruction)1,
    // 17
    Code::Instruction::Sub,
    // 18
    Code::Instruction::Push8, (Code::Instruction)6,
    // 20
    Code::Instruction::Call,
    // 21
    Code::Instruction::Swap,
    // 22
    Code::Instruction::Push8, (Code::Instruction)2,
    // 24
    Code::Instruction::Sub,
    // 25
    Code::Instruction::Push8, (Code::Instruction)6,
    // 27
    Code::Instruction::Call,
    // 28
    Code::Instruction::Add,
    // 29
    Code::Instruction::Ret
};

class RecursiveFibonacciTest : public CodeTest {
public:
    RecursiveFibonacciTest()
        : CodeTest(recursiveFibonacciCode, 30)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == fib((int)recursiveFibonacciCode[1]);
    }
};

static const Code::Instruction iterativeFibonacciCode[] = {
    // Push 46
    (Code::Instruction)0x18, (Code::Instruction)12,
    // Push fibonacci
    (Code::Instruction)0x18, (Code::Instruction)0x06,
    // Call
    (Code::Instruction)0x1B,
    // Halt
    (Code::Instruction)0x20,
    // fibonacci: Dup
    (Code::Instruction)0x0F,
    // Push 1
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    // >
    (Code::Instruction)0x0D,
    // Push isGreaterThanOne
    (Code::Instruction)0x18, (Code::Instruction)0x0E,
    // Cjmp
    (Code::Instruction)0x1E,
    // Ret
    (Code::Instruction)0x1C,
    // isGreaterThanOne: Push 0
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    // Push 1
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    // loop: dup
    (Code::Instruction)0x0F,
    // tuck
    (Code::Instruction)0x14,
    // +
    (Code::Instruction)0x00,
    // rot
    (Code::Instruction)0x12,
    // Push 1
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    // -
    (Code::Instruction)0x01,
    // dup
    (Code::Instruction)0x0F,
    // Push 4
    (Code::Instruction)0x18, (Code::Instruction)0x04,
    // Ntuck
    (Code::Instruction)0x15,
    // Push 1
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    // >
    (Code::Instruction)0x0D,
    // Push loop
    (Code::Instruction)0x18, (Code::Instruction)0x12,
    // Cjmp
    (Code::Instruction)0x1E,
    // Rot
    (Code::Instruction)0x12,
    // Drop
    (Code::Instruction)0x0E,
    // Swap
    (Code::Instruction)0x11,
    // Drop
    (Code::Instruction)0x0E,
    // Ret
    (Code::Instruction)0x1C
};
class IterativeFibonacciTest : public CodeTest {
public:
    IterativeFibonacciTest()
        : CodeTest(iterativeFibonacciCode, sizeof(iterativeFibonacciCode) / sizeof(iterativeFibonacciCode[0]))
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == fib((int)iterativeFibonacciCode[1]);
    }
};

/// Just the inner loop of the iterative fibonacci program
static const Code::Instruction iterativeFibonacciPartial[] = {
    Code::Instruction::Dup,
    Code::Instruction::Tuck,
    Code::Instruction::Add,
    Code::Instruction::Rot,
    Code::Instruction::Push8, (Code::Instruction)1,
    Code::Instruction::Sub,
    Code::Instruction::Dup,
    Code::Instruction::Push8, (Code::Instruction)4
};
class IterativeFibonacciPartialTest : public CodeTest {
    int m_A, m_B, m_C;

public:
    IterativeFibonacciPartialTest()
        : CodeTest(iterativeFibonacciPartial, sizeof(iterativeFibonacciPartial) / sizeof(iterativeFibonacciPartial[0]))
        , m_A(10)
        , m_B(17)
        , m_C(5)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(m_C); // c
        state.m_stack.push(m_A); // a
        state.m_stack.push(m_B); // b
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 5 + numberOfCanaryValues() && state.m_stack.peek(0) == 4 && state.m_stack.peek(1) == m_C - 1 && state.m_stack.peek(2) == (m_C - 1) && state.m_stack.peek(3) == (m_A + m_B) && state.m_stack.peek(4) == m_B;
    }
};

/**
 * This is a somewhat awkward test that is designed to verify that jumps to the start of
 * functions behave correctly
 */
static const Code::Instruction jumpToFunctionCode[] = {
    // 0: Call the function that calls decToZeroFunction
    Code::Instruction::Push8, (Code::Instruction)21, Code::Instruction::Call,
    // 3: End
    Code::Instruction::Ret,
    // 4: Label(4) decToZeroFunction
    Code::Instruction::Push8, (Code::Instruction)19, Code::Instruction::Call,
    // 7
    Code::Instruction::Dup, Code::Instruction::Push8, (Code::Instruction)0,
    // 10
    Code::Instruction::Eq,
    // 11: CJMP to dec before return if done
    Code::Instruction::Push8, (Code::Instruction)17, Code::Instruction::Cjmp,
    // 14: Unconditonal jump to decFunction
    Code::Instruction::Push8, (Code::Instruction)4, Code::Instruction::Jmp,
    // 17: Label return
    Code::Instruction::Dec, Code::Instruction::Ret,
    // 19: Dec Function
    Code::Instruction::Dec, Code::Instruction::Ret,
    // 21: Function that calls decToZeroFunction
    Code::Instruction::Push8, (Code::Instruction)4, Code::Instruction::Call,
    // 22: Ret
    Code::Instruction::Inc, Code::Instruction::Ret
};
class JumpToFunctionTest : public CodeTest {
public:
    JumpToFunctionTest()
        : CodeTest(jumpToFunctionCode, sizeof(jumpToFunctionCode) / sizeof(Code::Instruction))
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(5);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == 0;
    }
};

/**
 * Verifies that jumps to the start of non-recursive functions also behave correctly
 * Needed because non-recursive functions don't push LR
 */
static const Code::Instruction jumpToNonRecFunctionCode[] = {
    Code::Instruction::Push8, (Code::Instruction)4, Code::Instruction::Call,
    Code::Instruction::Halt,
    // 4: decToZero function
    Code::Instruction::Dec,
    // 5: == 0
    Code::Instruction::Dup, Code::Instruction::Push8, (Code::Instruction)0, Code::Instruction::Eq,
    // 9: Jump to
    Code::Instruction::Push8, (Code::Instruction)15, Code::Instruction::Cjmp,
    // 12: Jump to start of loop
    Code::Instruction::Push8, (Code::Instruction)4, Code::Instruction::Jmp,
    // 15: Return
    Code::Instruction::Ret
};
class JumpToNonRecFunctionTest : public CodeTest {
public:
    JumpToNonRecFunctionTest()
        : CodeTest(jumpToNonRecFunctionCode, sizeof(jumpToNonRecFunctionCode) / sizeof(Code::Instruction))
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(5);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == 0;
    }
};

static Code::Instruction conditionalCode[] = {
    // 0
    Code::Instruction::Push8, (Code::Instruction)37,
    // 2
    Code::Instruction::Push8, (Code::Instruction)42,
    // 4
    Code::Instruction::Eq,
    // 5
    Code::Instruction::Push8, (Code::Instruction)11,
    // 7
    Code::Instruction::Cjmp,
    // 8
    Code::Instruction::Push8, (Code::Instruction)10,
    // 10
    Code::Instruction::Halt,
    // 11
    Code::Instruction::Push8, (Code::Instruction)9,
    // 13
    Code::Instruction::Halt
};
class ConditionalCodeTest : public CodeTest {
private:
    bool m_expectation;

public:
    ConditionalCodeTest(int a, int b, Code::Instruction cond, bool expectedOutcome)
        : CodeTest((const Code::Instruction*)conditionalCode, sizeof(conditionalCode) / sizeof(Code::Instruction))
        , m_expectation(expectedOutcome)
    {
        conditionalCode[1] = (Code::Instruction)a;
        conditionalCode[3] = (Code::Instruction)b;
        conditionalCode[4] = cond;
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        if (state.m_stack.size() == 1 + numberOfCanaryValues()) {
            if (m_expectation && state.m_stack.peek() == (int)conditionalCode[12]) {
                return true;
            } else if (!m_expectation && state.m_stack.peek() == (int)conditionalCode[9]) {
                return true;
            }
        }
        return false;
    }
};

class OptionalEncodingTest : public CodeTest {
public:
    OptionalEncodingTest(int push, int pop)
        : CodeTest((const Code::Instruction*)instrs, 2)
        , push(push)
        , pop(pop)
    {
        instrs[0] = (Code::Instruction)0xFF;
        // Ideally use safe shifts here?
        instrs[1] = (Code::Instruction)((push << 4) | pop);
    }

    OptionalEncodingTest(Code::Instruction op, Code::Instruction effect)
        : CodeTest((const Code::Instruction*)instrs, 2)
    {
        instrs[0] = op;
        instrs[1] = effect;
        push = (unsigned)effect >> 4;
        pop = (unsigned)effect & 0xF;
    }

    void preTest(Environment::VM& state)
    {
        state.m_stack.clear();
        for (int i = 0; i < 20; i++) {
            state.m_stack.push(i);
        }
    }

    bool postTest(Environment::VM& state)
    {
        if (state.m_stack.size() == (unsigned)(20 - pop + push)) {
            for (int i = 0; i < push; i++) {
                if (state.m_stack.peek(i) != 0) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

private:
    Code::Instruction instrs[2];
    int push, pop;
};

static const Code::Instruction dynamicCallCode[] = {
    // 0: Push address of function to call
    Code::Instruction::Push8, (Code::Instruction)8,
    // 2: Call a function that calls the top of stack
    Code::Instruction::Push8, (Code::Instruction)6, Code::Instruction::Call,
    // 5
    Code::Instruction::Halt,
    // 6: Call the top of stack
    Code::Instruction::Call,
    // 7
    Code::Instruction::Ret,
    // 8: Function that gets called
    Code::Instruction::Push8, (Code::Instruction)41,
    // 10
    Code::Instruction::Ret
};
class DynamicCallTest : public CodeTest {
public:
    DynamicCallTest()
        : CodeTest(dynamicCallCode, sizeof(dynamicCallCode) / sizeof(Code::Instruction))
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == (int)dynamicCallCode[9];
    }
};

static const Code::Instruction dynamicCall2Code[] = {
    // 0:
    Code::Instruction::Push8, (Code::Instruction)7, Code::Instruction::Call,
    // 3:
    Code::Instruction::Push8, (Code::Instruction)7, Code::Instruction::Call,
    // 6:
    Code::Instruction::Halt,
    // 7: Call the top of stack
    Code::Instruction::Call, Code::Instruction::Ret,
    // 9
    Code::Instruction::Ret,
    // 10: Function that gets called
    Code::Instruction::Push8, (Code::Instruction)13,
    // 12
    Code::Instruction::Ret,
    // 13: Second function to be called
    Code::Instruction::Push8, (Code::Instruction)42,
    // 15:
    Code::Instruction::Ret
};
class DynamicCall2Test : public CodeTest {
public:
    DynamicCall2Test()
        : CodeTest(dynamicCall2Code, sizeof(dynamicCall2Code) / sizeof(Code::Instruction))
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(10);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == (int)dynamicCall2Code[14];
    }
};

class MarksEnumTest : public CodeTest {
public:
    MarksEnumTest()
        : CodeTest(EnumTestCode, EnumTestCodeLength)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 8 + numberOfCanaryValues() && state.m_stack.peek(0) == 1 && state.m_stack.peek(1) == 0 && state.m_stack.peek(2) == -1 && state.m_stack.peek(3) == 3 && state.m_stack.peek(4) == 3 && state.m_stack.peek(5) == 4 && state.m_stack.peek(6) == 1000 && state.m_stack.peek(7) == 0;
    }
};

class MarksLinkTest2 : public CodeTest {
public:
    MarksLinkTest2()
        : CodeTest(LinkTest2Code, LinkTest2Length)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 8 + numberOfCanaryValues() && state.m_stack.peek(0) == 1 && state.m_stack.peek(1) == 0 && state.m_stack.peek(2) == -1 && state.m_stack.peek(3) == 0 && state.m_stack.peek(4) == 1 && state.m_stack.peek(5) == 7 && state.m_stack.peek(6) == 2425 && state.m_stack.peek(7) == 10;
    }
};

class MarksLoopExitTest : public CodeTest {
public:
    MarksLoopExitTest()
        : CodeTest(LoopExitTestCode, LoopExitTestLength)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 4 + numberOfCanaryValues() && state.m_stack.peek(0) == 3 && state.m_stack.peek(1) == 6 && state.m_stack.peek(2) == 2 && state.m_stack.peek(3) == 2;
    }
};

class MarksUsingTest : public CodeTest {
public:
    MarksUsingTest()
        : CodeTest(UsingTestCode, UsingTestLength)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        // Uses the accelerometer so the top three elements on the stack could be any garbage
        return state.m_stack.size() == 5 + numberOfCanaryValues() && state.m_stack.peek(3) == 10 && state.m_stack.peek(4) == 5;
    }
};

/// Computes the greatest common divisor of 610 and 987 (Fibonacci numbers, GCD performs worse for these)
static const Code::Instruction gcdTest[] = {
    (Code::Instruction)0x19, (Code::Instruction)0x62, (Code::Instruction)0x02, // 610
    (Code::Instruction)0x19, (Code::Instruction)0xDB, (Code::Instruction)0x03, // 987
    (Code::Instruction)0x18, (Code::Instruction)0x0A, // gcd
    (Code::Instruction)0x1B, // call
    (Code::Instruction)0x20, // halt
    (Code::Instruction)0x0F, // gcd: dup
    (Code::Instruction)0x18, (Code::Instruction)0x00, // 0
    (Code::Instruction)0x0B, // =
    (Code::Instruction)0x18, (Code::Instruction)0x19, // ret0
    (Code::Instruction)0x1E, // cjmp
    (Code::Instruction)0x0F, // dup
    (Code::Instruction)0x12, // rot
    (Code::Instruction)0x11, // swap
    (Code::Instruction)0x04, // mod
    (Code::Instruction)0x18, (Code::Instruction)0x0A, // gcd
    (Code::Instruction)0x1B, // call
    (Code::Instruction)0x1C, // ret
    (Code::Instruction)0x0E, // drop
    (Code::Instruction)0x1C, // ret
};
class GCDTest : public CodeTest {
public:
    GCDTest()
        : CodeTest(gcdTest, sizeof(gcdTest) / sizeof(gcdTest[0]))
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == 1;
    }
};

/// Should be possible to implement this just as a loop
static const Code::Instruction tailRecTestCode[] = {
    // 0: main: push 10
    Code::Instruction::Push8, (Code::Instruction)10,
    // 2: push downToZero
    Code::Instruction::Push8, (Code::Instruction)6,
    // 4
    Code::Instruction::Call,
    // 5
    Code::Instruction::Halt,
    // 6: downToZero
    Code::Instruction::Dup,
    // 7: push 0
    Code::Instruction::Push8, (Code::Instruction)0,
    // 9
    Code::Instruction::Eq,
    // 10: push end
    Code::Instruction::Push8, (Code::Instruction)18,
    // 12: cjmp
    Code::Instruction::Cjmp,
    // 13
    Code::Instruction::Dec,
    // 14: push downToZero
    Code::Instruction::Push8, (Code::Instruction)6,
    // 16
    Code::Instruction::Call,
    // 17
    Code::Instruction::Ret,
    // 18: end
    Code::Instruction::Ret
};
class TailRecTest : public CodeTest {
public:
    TailRecTest()
        : CodeTest(tailRecTestCode, sizeof(tailRecTestCode) / sizeof(tailRecTestCode[0]))
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == numberOfCanaryValues() + 1 && state.m_stack.peek() == 0;
    }
};

/// Used for verifying weird behaviour in the COW allocator
static const Code::Instruction rotArithmetic[] = {
    Code::Instruction::Dup,
    Code::Instruction::Dup,
    Code::Instruction::Dup,
    Code::Instruction::Dup,
    Code::Instruction::Dup,
    Code::Instruction::Add,
    Code::Instruction::Add,
    Code::Instruction::Add,
    Code::Instruction::Add,
    Code::Instruction::Add,
    Code::Instruction::Push8,
    (Code::Instruction)1,
    Code::Instruction::Dup,
    Code::Instruction::Inc,
    Code::Instruction::Dup,
    Code::Instruction::Inc,
    Code::Instruction::Dup,
    Code::Instruction::Inc,
    Code::Instruction::Dup,
    Code::Instruction::Inc,
    Code::Instruction::Rot,
    Code::Instruction::Rot,
    Code::Instruction::Add,
    Code::Instruction::Add,
    Code::Instruction::Add,
    Code::Instruction::Add,
    Code::Instruction::Add,
};
class RotArithmeticTest : public CodeTest {
public:
    RotArithmeticTest()
        : CodeTest(rotArithmetic, sizeof(rotArithmetic) / sizeof(rotArithmetic[0]))
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        state.m_stack.push(1);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == 21;
    }
};

/// Used for verifying weird behaviour in the COW allocator
static const Code::Instruction rotPushArithmetic[] = {
    Code::Instruction::Push8, (Code::Instruction)1,
    Code::Instruction::Dup,
    Code::Instruction::Dup,
    Code::Instruction::Rot,
    Code::Instruction::Add,
    Code::Instruction::Add
};
class RotPushArithmeticTest : public CodeTest {
public:
    RotPushArithmeticTest()
        : CodeTest(rotPushArithmetic, sizeof(rotPushArithmetic) / sizeof(rotPushArithmetic[0]))
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_stack.size() == 1 + numberOfCanaryValues() && state.m_stack.peek() == 3;
    }
};

/// This test doesn't support canaries
class UnderflowTest : public CodeTest {
public:
    UnderflowTest(Code::Instruction instruction, int stackValues = 0)
        : CodeTest(&m_instruction, 1)
        , m_instruction(instruction)
        , m_stackValues(stackValues)
    {
    }

    void preTest(Environment::VM& state)
    {
        clearIfNotInCanaryMode(state);
        while ((int)state.m_stack.size() < m_stackValues) {
            state.m_stack.push(0);
        }
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_status == Environment::VMStatus::StackUnderflow;
    }

private:
    Code::Instruction m_instruction;
    int m_stackValues;
};

/// This test doesn't support canaries
class OverflowTest : public CodeTest {
public:
    OverflowTest(Code::Instruction instruction)
        : CodeTest(m_instructions, instruction == Code::Instruction::Push8 ? 2 : (instruction == Code::Instruction::Push16 ? 3 : 1))
        , m_instructions{ instruction, (Code::Instruction)0, (Code::Instruction)0 }
    {
    }

    void preTest(Environment::VM& state)
    {
        state.m_stack.m_stackPointer = state.m_stack.m_base;
    }

    bool postTest(Environment::VM& state)
    {
        return state.m_status == Environment::VMStatus::StackOverflow;
    }

private:
    Code::Instruction m_instructions[3];
};

bool testCodeExecution()
{
    printTestHeader("COMPILER AND INTERPRETER TESTS");

    bool success = true;

    success &= CODE_TEST(EmptyCodeTest);
    success &= CANARY_CODE_TEST(EmptyCodeTest);

    success &= OP_TEST("AdditionTest", SingleOperatorTest(Code::Instruction::Add, 1, 42, 43));
    success &= OP_TEST("AdditionTestPush", SingleOperatorPushTest(Code::Instruction::Add, 1, 42, 43));
    success &= OP_TEST("AdditionTestTwoOp", TwoOperatorTest(Code::Instruction::Add, Code::Instruction::Add, 1, 42, 43, 86));
    success &= OP_TEST("SubtractionTest", SingleOperatorTest(Code::Instruction::Sub, 42, 37, 5));
    success &= OP_TEST("SubtractionTestPush", SingleOperatorPushTest(Code::Instruction::Sub, 42, 37, 5));
    success &= OP_TEST("SubtractionTestTwoOp", TwoOperatorTest(Code::Instruction::Sub, Code::Instruction::Sub, 100, 50, 25, 75));
    success &= OP_TEST("MultiplicationTest", SingleOperatorTest(Code::Instruction::Mul, 2, 21, 42));
    success &= OP_TEST("MultiplicationPushTest", SingleOperatorPushTest(Code::Instruction::Mul, 2, 21, 42));
    success &= OP_TEST("MultiplicationTestTwoOp", TwoOperatorTest(Code::Instruction::Mul, Code::Instruction::Mul, 2, 2, 2, 8));
    success &= OP_TEST("DivTest", SingleOperatorTest(Code::Instruction::Div, 42, 2, 21));
    success &= OP_TEST("DivPushTest", SingleOperatorPushTest(Code::Instruction::Div, 42, 2, 21));
    success &= OP_TEST("ModTest", SingleOperatorTest(Code::Instruction::Mod, 50, 7, 1));
    success &= OP_TEST("ModPushTest", SingleOperatorPushTest(Code::Instruction::Mod, 50, 7, 1));

    success &= CANARY_OP_TEST("AdditionTest", SingleOperatorTest(Code::Instruction::Add, 1, 42, 43));
    success &= CANARY_OP_TEST("AdditionTestPush", SingleOperatorPushTest(Code::Instruction::Add, 1, 42, 43));
    success &= CANARY_OP_TEST("AdditionTestTwoOp", TwoOperatorTest(Code::Instruction::Add, Code::Instruction::Add, 1, 42, 43, 86));
    success &= CANARY_OP_TEST("SubtractionTest", SingleOperatorTest(Code::Instruction::Sub, 42, 37, 5));
    success &= CANARY_OP_TEST("SubtractionTestPush", SingleOperatorPushTest(Code::Instruction::Sub, 42, 37, 5));
    success &= CANARY_OP_TEST("SubtractionTestTwoOp", TwoOperatorTest(Code::Instruction::Sub, Code::Instruction::Sub, 100, 50, 25, 75));
    success &= CANARY_OP_TEST("MultiplicationTest", SingleOperatorTest(Code::Instruction::Mul, 2, 21, 42));
    success &= CANARY_OP_TEST("MultiplicationPushTest", SingleOperatorPushTest(Code::Instruction::Mul, 2, 21, 42));
    success &= CANARY_OP_TEST("MultiplicationTestTwoOp", TwoOperatorTest(Code::Instruction::Mul, Code::Instruction::Mul, 2, 2, 2, 8));
    success &= CANARY_OP_TEST("DivTest", SingleOperatorTest(Code::Instruction::Div, 42, 2, 21));
    success &= CANARY_OP_TEST("DivPushTest", SingleOperatorPushTest(Code::Instruction::Div, 42, 2, 21));
    success &= CANARY_OP_TEST("ModTest", SingleOperatorTest(Code::Instruction::Mod, 50, 7, 1));
    success &= CANARY_OP_TEST("ModPushTest", SingleOperatorPushTest(Code::Instruction::Mod, 50, 7, 1));

    success &= OP_TEST("IncTest", SingleOperatorTest(Code::Instruction::Inc, 42, 43));
    success &= OP_TEST("DecTest", SingleOperatorTest(Code::Instruction::Dec, 42, 41));

    success &= CANARY_OP_TEST("IncTest", SingleOperatorTest(Code::Instruction::Inc, 42, 43));
    success &= CANARY_OP_TEST("DecTest", SingleOperatorTest(Code::Instruction::Dec, 42, 41));

    success &= OP_TEST("MaxTest", SingleOperatorTest(Code::Instruction::Max, 37, 42, 42));
    success &= OP_TEST("MinTest", SingleOperatorTest(Code::Instruction::Min, 37, 42, 37));
    success &= OP_TEST("MaxPushTest", SingleOperatorPushTest(Code::Instruction::Max, 37, 42, 42));
    success &= OP_TEST("MinPushTest", SingleOperatorPushTest(Code::Instruction::Min, 37, 42, 37));

    success &= CANARY_OP_TEST("MaxTest", SingleOperatorTest(Code::Instruction::Max, 37, 42, 42));
    success &= CANARY_OP_TEST("MinTest", SingleOperatorTest(Code::Instruction::Min, 37, 42, 37));
    success &= CANARY_OP_TEST("MaxPushTest", SingleOperatorPushTest(Code::Instruction::Max, 37, 42, 42));
    success &= CANARY_OP_TEST("MinPushTest", SingleOperatorPushTest(Code::Instruction::Min, 37, 42, 37));

    success &= OP_TEST("LtTest", SingleOperatorTest(Code::Instruction::Lt, 37, 42, 1));
    success &= OP_TEST("LeTest", SingleOperatorTest(Code::Instruction::Le, 37, 42, 1));
    success &= OP_TEST("EqTest", SingleOperatorTest(Code::Instruction::Eq, 37, 42, 0));
    success &= OP_TEST("GeTest", SingleOperatorTest(Code::Instruction::Ge, 37, 42, 0));
    success &= OP_TEST("GtTest", SingleOperatorTest(Code::Instruction::Gt, 37, 42, 0));

    success &= CANARY_OP_TEST("LtTest", SingleOperatorTest(Code::Instruction::Lt, 37, 42, 1));
    success &= CANARY_OP_TEST("LeTest", SingleOperatorTest(Code::Instruction::Le, 37, 42, 1));
    success &= CANARY_OP_TEST("EqTest", SingleOperatorTest(Code::Instruction::Eq, 37, 42, 0));
    success &= CANARY_OP_TEST("GeTest", SingleOperatorTest(Code::Instruction::Ge, 37, 42, 0));
    success &= CANARY_OP_TEST("GtTest", SingleOperatorTest(Code::Instruction::Gt, 37, 42, 0));

    success &= OP_TEST("LtPushTest", SingleOperatorPushTest(Code::Instruction::Lt, 37, 42, 1));
    success &= OP_TEST("LePushTest", SingleOperatorPushTest(Code::Instruction::Le, 37, 42, 1));
    success &= OP_TEST("EqPushTest", SingleOperatorPushTest(Code::Instruction::Eq, 37, 42, 0));
    success &= OP_TEST("GePushTest", SingleOperatorPushTest(Code::Instruction::Ge, 37, 42, 0));
    success &= OP_TEST("GtPushTest", SingleOperatorPushTest(Code::Instruction::Gt, 37, 42, 0));

    success &= CANARY_OP_TEST("LtPushTest", SingleOperatorPushTest(Code::Instruction::Lt, 37, 42, 1));
    success &= CANARY_OP_TEST("LePushTest", SingleOperatorPushTest(Code::Instruction::Le, 37, 42, 1));
    success &= CANARY_OP_TEST("EqPushTest", SingleOperatorPushTest(Code::Instruction::Eq, 37, 42, 0));
    success &= CANARY_OP_TEST("GePushTest", SingleOperatorPushTest(Code::Instruction::Ge, 37, 42, 0));
    success &= CANARY_OP_TEST("GtPushTest", SingleOperatorPushTest(Code::Instruction::Gt, 37, 42, 0));

    success &= OP_TEST("LtTest2", SingleOperatorTest(Code::Instruction::Lt, 42, 37, 0));
    success &= OP_TEST("LeTest2", SingleOperatorTest(Code::Instruction::Le, 42, 37, 0));
    success &= OP_TEST("EqTest2", SingleOperatorTest(Code::Instruction::Eq, 37, 37, 1));
    success &= OP_TEST("GeTest2", SingleOperatorTest(Code::Instruction::Ge, 42, 37, 1));
    success &= OP_TEST("GtTest2", SingleOperatorTest(Code::Instruction::Gt, 42, 37, 1));

    success &= CANARY_OP_TEST("LtTest2", SingleOperatorTest(Code::Instruction::Lt, 42, 37, 0));
    success &= CANARY_OP_TEST("LeTest2", SingleOperatorTest(Code::Instruction::Le, 42, 37, 0));
    success &= CANARY_OP_TEST("EqTest2", SingleOperatorTest(Code::Instruction::Eq, 37, 37, 1));
    success &= CANARY_OP_TEST("GeTest2", SingleOperatorTest(Code::Instruction::Ge, 42, 37, 1));
    success &= CANARY_OP_TEST("GtTest2", SingleOperatorTest(Code::Instruction::Gt, 42, 37, 1));

    success &= OP_TEST("LtPushTest2", SingleOperatorPushTest(Code::Instruction::Lt, 42, 37, 0));
    success &= OP_TEST("LePushTest2", SingleOperatorPushTest(Code::Instruction::Le, 42, 37, 0));
    success &= OP_TEST("EqPushTest2", SingleOperatorPushTest(Code::Instruction::Eq, 37, 37, 1));
    success &= OP_TEST("GePushTest2", SingleOperatorPushTest(Code::Instruction::Ge, 42, 37, 1));
    success &= OP_TEST("GtPushTest2", SingleOperatorPushTest(Code::Instruction::Gt, 42, 37, 1));

    success &= CANARY_OP_TEST("LtPushTest2", SingleOperatorPushTest(Code::Instruction::Lt, 42, 37, 0));
    success &= CANARY_OP_TEST("LePushTest2", SingleOperatorPushTest(Code::Instruction::Le, 42, 37, 0));
    success &= CANARY_OP_TEST("EqPushTest2", SingleOperatorPushTest(Code::Instruction::Eq, 37, 37, 1));
    success &= CANARY_OP_TEST("GePushTest2", SingleOperatorPushTest(Code::Instruction::Ge, 42, 37, 1));
    success &= CANARY_OP_TEST("GtPushTest2", SingleOperatorPushTest(Code::Instruction::Gt, 42, 37, 1));

    success &= OP_TEST("DropTest", SingleOperatorTest(Code::Instruction::Drop, 37, 42, 37));
    success &= CANARY_OP_TEST("DropTest", SingleOperatorTest(Code::Instruction::Drop, 37, 42, 37));

    success &= CODE_TEST(DupTest);
    success &= CODE_TEST(DupManyTest);
    success &= CODE_TEST(NdupTest);
    success &= CODE_TEST(SwapTest);
    success &= CODE_TEST(RotTest);
    success &= CODE_TEST(NrotTest);
    success &= CODE_TEST(TuckTest);
    success &= CODE_TEST(NtuckTest);
    success &= CODE_TEST(SizeTest);

    success &= CANARY_CODE_TEST(DupTest);
    success &= CANARY_CODE_TEST(DupManyTest);
    success &= CANARY_CODE_TEST(NdupTest);
    success &= CANARY_CODE_TEST(SwapTest);
    success &= CANARY_CODE_TEST(RotTest);
    success &= CANARY_CODE_TEST(NrotTest);
    success &= CANARY_CODE_TEST(TuckTest);
    success &= CANARY_CODE_TEST(NtuckTest);
    success &= CANARY_CODE_TEST(SizeTest);

    success &= CODE_TEST(RotArithmeticTest);
    success &= CANARY_CODE_TEST(RotArithmeticTest);
    success &= CODE_TEST(RotPushArithmeticTest);
    success &= CANARY_CODE_TEST(RotPushArithmeticTest);

    success &= CODE_TEST(RandomTest);
    success &= CANARY_CODE_TEST(RandomTest);

    success &= OP_TEST("Push8Test(-1)", Push8Test(-1));
    success &= OP_TEST("Push8Test(0)", Push8Test(0));
    success &= OP_TEST("Push8Test(1)", Push8Test(1));
    success &= OP_TEST("Push8Test(42)", Push8Test(42));
    success &= OP_TEST("Push8Test(127)", Push8Test(127));
    success &= OP_TEST("Push8Test(-128)", Push8Test(-128));

    success &= CANARY_OP_TEST("Push8Test(-1)", Push8Test(-1));
    success &= CANARY_OP_TEST("Push8Test(0)", Push8Test(0));
    success &= CANARY_OP_TEST("Push8Test(1)", Push8Test(1));
    success &= CANARY_OP_TEST("Push8Test(42)", Push8Test(42));
    success &= CANARY_OP_TEST("Push8Test(127)", Push8Test(127));
    success &= CANARY_OP_TEST("Push8Test(-128)", Push8Test(-128));

    success &= CODE_TEST(Push8ManyTest);
    success &= CANARY_CODE_TEST(Push8ManyTest);

    success &= OP_TEST("Push16Test(-1)", Push16Test(-1));
    success &= OP_TEST("Push16Test(0)", Push16Test(0));
    success &= OP_TEST("Push16Test(1)", Push16Test(1));
    success &= OP_TEST("Push16Test(42)", Push16Test(42));
    success &= OP_TEST("Push16Test(32767)", Push16Test(32767)); // 2^15 - 1
    success &= OP_TEST("Push16Test(-32768)", Push16Test(-32768));

    success &= CANARY_OP_TEST("Push16Test(-1)", Push16Test(-1));
    success &= CANARY_OP_TEST("Push16Test(0)", Push16Test(0));
    success &= CANARY_OP_TEST("Push16Test(1)", Push16Test(1));
    success &= CANARY_OP_TEST("Push16Test(42)", Push16Test(42));
    success &= CANARY_OP_TEST("Push16Test(32767)", Push16Test(32767)); // 2^15 - 1
    success &= CANARY_OP_TEST("Push16Test(-32768)", Push16Test(-32768));

    success &= CODE_TEST(WaitTest);
    success &= CANARY_CODE_TEST(WaitTest);

    success &= CODE_TEST(JumpTest);
    success &= OP_TEST("CjmpTest(false)", CjmpTest(false));
    success &= OP_TEST("CjmpTest(true)", CjmpTest(true));
    success &= CODE_TEST(CjmpBackwardsTest);

    success &= CANARY_CODE_TEST(JumpTest);
    success &= CANARY_OP_TEST("CjmpTest(false)", CjmpTest(false));
    success &= CANARY_OP_TEST("CjmpTest(true)", CjmpTest(true));
    success &= CANARY_CODE_TEST(CjmpBackwardsTest);

    success &= OP_TEST("EqCjmpTest1", ConditionalCodeTest(37, 42, Code::Instruction::Eq, false));
    success &= OP_TEST("EqCjmpTest2", ConditionalCodeTest(37, 37, Code::Instruction::Eq, true));

    success &= OP_TEST("LeCjmpTest1", ConditionalCodeTest(37, 42, Code::Instruction::Le, true));
    success &= OP_TEST("LeCjmpTest2", ConditionalCodeTest(37, 37, Code::Instruction::Le, true));
    success &= OP_TEST("LeCjmpTest3", ConditionalCodeTest(42, 37, Code::Instruction::Le, false));

    success &= OP_TEST("LtCjmpTest1", ConditionalCodeTest(37, 42, Code::Instruction::Lt, true));
    success &= OP_TEST("LtCjmpTest2", ConditionalCodeTest(37, 37, Code::Instruction::Lt, false));
    success &= OP_TEST("LtCjmpTest3", ConditionalCodeTest(42, 37, Code::Instruction::Lt, false));

    success &= OP_TEST("GeCjmpTest1", ConditionalCodeTest(37, 42, Code::Instruction::Ge, false));
    success &= OP_TEST("GeCjmpTest2", ConditionalCodeTest(37, 37, Code::Instruction::Ge, true));
    success &= OP_TEST("GeCjmpTest3", ConditionalCodeTest(42, 37, Code::Instruction::Ge, true));

    success &= OP_TEST("GtCjmpTest1", ConditionalCodeTest(37, 42, Code::Instruction::Gt, false));
    success &= OP_TEST("GtCjmpTest2", ConditionalCodeTest(37, 37, Code::Instruction::Gt, false));
    success &= OP_TEST("GtCjmpTest3", ConditionalCodeTest(42, 37, Code::Instruction::Gt, true));

    success &= CANARY_OP_TEST("EqCjmpTest1", ConditionalCodeTest(37, 42, Code::Instruction::Eq, false));
    success &= CANARY_OP_TEST("EqCjmpTest2", ConditionalCodeTest(37, 37, Code::Instruction::Eq, true));

    success &= CANARY_OP_TEST("LeCjmpTest1", ConditionalCodeTest(37, 42, Code::Instruction::Le, true));
    success &= CANARY_OP_TEST("LeCjmpTest2", ConditionalCodeTest(37, 37, Code::Instruction::Le, true));
    success &= CANARY_OP_TEST("LeCjmpTest3", ConditionalCodeTest(42, 37, Code::Instruction::Le, false));

    success &= CANARY_OP_TEST("LtCjmpTest1", ConditionalCodeTest(37, 42, Code::Instruction::Lt, true));
    success &= CANARY_OP_TEST("LtCjmpTest2", ConditionalCodeTest(37, 37, Code::Instruction::Lt, false));
    success &= CANARY_OP_TEST("LtCjmpTest3", ConditionalCodeTest(42, 37, Code::Instruction::Lt, false));

    success &= CANARY_OP_TEST("GeCjmpTest1", ConditionalCodeTest(37, 42, Code::Instruction::Ge, false));
    success &= CANARY_OP_TEST("GeCjmpTest2", ConditionalCodeTest(37, 37, Code::Instruction::Ge, true));
    success &= CANARY_OP_TEST("GeCjmpTest3", ConditionalCodeTest(42, 37, Code::Instruction::Ge, true));

    success &= CANARY_OP_TEST("GtCjmpTest1", ConditionalCodeTest(37, 42, Code::Instruction::Gt, false));
    success &= CANARY_OP_TEST("GtCjmpTest2", ConditionalCodeTest(37, 37, Code::Instruction::Gt, false));
    success &= CANARY_OP_TEST("GtCjmpTest3", ConditionalCodeTest(42, 37, Code::Instruction::Gt, true));

    success &= CODE_TEST(FetchTest);
    success &= CANARY_CODE_TEST(FetchTest);

    success &= CODE_TEST(FunctionTest);
    success &= CODE_TEST(BoundedRecursionTest);
    success &= CODE_TEST(HaltTest);
    success &= CODE_TEST(HaltSideEffectTest);
    success &= CODE_TEST(IterativeFibonacciTest);
    success &= CODE_TEST(IterativeFibonacciPartialTest);
    success &= CODE_TEST(RecursiveFibonacciTest);
    success &= CODE_TEST(RecursiveHaltTest);
    success &= CODE_TEST(JumpToFunctionTest);
    success &= CODE_TEST(JumpToNonRecFunctionTest);
    success &= CODE_TEST(GCDTest);
    success &= CODE_TEST(TailRecTest);

    success &= CANARY_CODE_TEST(FunctionTest);
    success &= CANARY_CODE_TEST(BoundedRecursionTest);
    success &= CANARY_CODE_TEST(HaltTest);
    success &= CANARY_CODE_TEST(HaltSideEffectTest);
    success &= CANARY_CODE_TEST(IterativeFibonacciTest);
    success &= CANARY_CODE_TEST(IterativeFibonacciPartialTest);
    success &= CANARY_CODE_TEST(RecursiveFibonacciTest);
    success &= CANARY_CODE_TEST(RecursiveHaltTest);
    success &= CANARY_CODE_TEST(JumpToFunctionTest);
    success &= CANARY_CODE_TEST(JumpToNonRecFunctionTest);
    success &= CANARY_CODE_TEST(GCDTest);
    success &= CANARY_CODE_TEST(TailRecTest);

    success &= CODE_TEST(DynamicCallTest);
    success &= CODE_TEST(DynamicCall2Test);

    success &= CANARY_CODE_TEST(DynamicCallTest);
    success &= CANARY_CODE_TEST(DynamicCall2Test);

    success &= CODE_TEST(MarksEnumTest);
    success &= CODE_TEST(MarksLoopExitTest);
    // success &= CODE_TEST(MarksUsingTest);

    success &= CANARY_CODE_TEST(MarksEnumTest);
    success &= CANARY_CODE_TEST(MarksLoopExitTest);
    // success &= CANARY_CODE_TEST(MarksUsingTest);

    if (StackCheckMode != StackCheck::None) {
        // All the instructions that have to access at least one element from the stack
        success &= OP_TEST("AddUnderflow", UnderflowTest(Code::Instruction::Add));
        success &= OP_TEST("SubUnderflow", UnderflowTest(Code::Instruction::Sub));
        success &= OP_TEST("MulUnderflow", UnderflowTest(Code::Instruction::Mul));
        success &= OP_TEST("DivUnderflow", UnderflowTest(Code::Instruction::Div));
        success &= OP_TEST("ModUnderflow", UnderflowTest(Code::Instruction::Mod));
        success &= OP_TEST("MaxUnderflow", UnderflowTest(Code::Instruction::Max));
        success &= OP_TEST("MinUnderflow", UnderflowTest(Code::Instruction::Min));
        success &= OP_TEST("LtUnderflow", UnderflowTest(Code::Instruction::Lt));
        success &= OP_TEST("LeUnderflow", UnderflowTest(Code::Instruction::Le));
        success &= OP_TEST("EqUnderflow", UnderflowTest(Code::Instruction::Eq));
        success &= OP_TEST("GeUnderflow", UnderflowTest(Code::Instruction::Ge));
        success &= OP_TEST("GtUnderflow", UnderflowTest(Code::Instruction::Gt));

        success &= OP_TEST("AddUnderflow2", UnderflowTest(Code::Instruction::Add, 1));
        success &= OP_TEST("SubUnderflow2", UnderflowTest(Code::Instruction::Sub, 1));
        success &= OP_TEST("MulUnderflow2", UnderflowTest(Code::Instruction::Mul, 1));
        success &= OP_TEST("DivUnderflow2", UnderflowTest(Code::Instruction::Div, 1));
        success &= OP_TEST("ModUnderflow2", UnderflowTest(Code::Instruction::Mod, 1));
        success &= OP_TEST("MaxUnderflow2", UnderflowTest(Code::Instruction::Max, 1));
        success &= OP_TEST("MinUnderflow2", UnderflowTest(Code::Instruction::Min, 1));
        success &= OP_TEST("LtUnderflow2", UnderflowTest(Code::Instruction::Lt, 1));
        success &= OP_TEST("LeUnderflow2", UnderflowTest(Code::Instruction::Le, 1));
        success &= OP_TEST("EqUnderflow2", UnderflowTest(Code::Instruction::Eq, 1));
        success &= OP_TEST("GeUnderflow2", UnderflowTest(Code::Instruction::Ge, 1));
        success &= OP_TEST("GtUnderflow2", UnderflowTest(Code::Instruction::Gt, 1));

        success &= OP_TEST("IncUnderflow", UnderflowTest(Code::Instruction::Inc));
        success &= OP_TEST("DecUnderflow", UnderflowTest(Code::Instruction::Dec));

        success &= OP_TEST("DropUnderflow", UnderflowTest(Code::Instruction::Drop));
        success &= OP_TEST("DupUnderflow", UnderflowTest(Code::Instruction::Dup));
        success &= OP_TEST("NdupUnderflow", UnderflowTest(Code::Instruction::Ndup));

        success &= OP_TEST("SwapUnderflow", UnderflowTest(Code::Instruction::Swap));
        success &= OP_TEST("SwapUnderflow2", UnderflowTest(Code::Instruction::Swap, 1));

        success &= OP_TEST("RotUnderflow", UnderflowTest(Code::Instruction::Rot));
        success &= OP_TEST("RotUnderflow2", UnderflowTest(Code::Instruction::Rot, 1));
        success &= OP_TEST("RotUnderflow3", UnderflowTest(Code::Instruction::Rot, 2));

        success &= OP_TEST("NrotUnderflow", UnderflowTest(Code::Instruction::Nrot));

        success &= OP_TEST("TuckUnderflow", UnderflowTest(Code::Instruction::Tuck));
        success &= OP_TEST("TuckUnderflow2", UnderflowTest(Code::Instruction::Tuck, 1));
        success &= OP_TEST("TuckUnderflow3", UnderflowTest(Code::Instruction::Tuck, 2));

        success &= OP_TEST("NtuckUnderflow", UnderflowTest(Code::Instruction::Ntuck));
        success &= OP_TEST("NrndUnderflow", UnderflowTest(Code::Instruction::Nrnd));
        success &= OP_TEST("FetchUnderflow", UnderflowTest(Code::Instruction::Fetch));
        success &= OP_TEST("CallUnderflow", UnderflowTest(Code::Instruction::Call));
        success &= OP_TEST("WaitUnderflow", UnderflowTest(Code::Instruction::Wait));

        // All the instructions that overflow
        success &= OP_TEST("Push8Overflow", OverflowTest(Code::Instruction::Push8));
        success &= OP_TEST("Push16Overflow", OverflowTest(Code::Instruction::Push16));
        success &= OP_TEST("SizeOverflow", OverflowTest(Code::Instruction::Size));
    }

    return success;
}

bool testOptionalCode()
{
    bool success = true;

    success &= OP_TEST("Optional(0x00)", OptionalEncodingTest(0, 0));
    success &= OP_TEST("Optional(0xF0)", OptionalEncodingTest(15, 0));
    success &= OP_TEST("Optional(0x0F)", OptionalEncodingTest(0, 15));
    success &= OP_TEST("Optional(0xF4)", OptionalEncodingTest(15, 4));

    success &= OP_TEST("Sleep Effect", OptionalEncodingTest(Code::Instruction::Sleep, Code::Instruction::SleepEffect));
    success &= OP_TEST("Tone Effect", OptionalEncodingTest(Code::Instruction::Tone, Code::Instruction::ToneEffect));
    success &= OP_TEST("Beep Effect", OptionalEncodingTest(Code::Instruction::Beep, Code::Instruction::BeepEffect));
    success &= OP_TEST("Rgb Effect", OptionalEncodingTest(Code::Instruction::Rgb, Code::Instruction::RgbEffect));
    success &= OP_TEST("Colour Effect", OptionalEncodingTest(Code::Instruction::Colour, Code::Instruction::ColourEffect));
    success &= OP_TEST("Flash Effect", OptionalEncodingTest(Code::Instruction::Flash, Code::Instruction::FlashEffect));
    success &= OP_TEST("Temp Effect", OptionalEncodingTest(Code::Instruction::Temp, Code::Instruction::TempEffect));
    success &= OP_TEST("Accel Effect", OptionalEncodingTest(Code::Instruction::Accel, Code::Instruction::AccelEffect));
    success &= OP_TEST("Pixel Effect", OptionalEncodingTest(Code::Instruction::Pixel, Code::Instruction::PixelEffect));
    success &= OP_TEST("Show number Effect", OptionalEncodingTest(Code::Instruction::ShowNumber, Code::Instruction::ShowNumberEffect));

    return success;
}
}