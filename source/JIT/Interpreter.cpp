#include "Interpreter.h"

#include "Bit/Bit.h"
#include "Device/OptionalInstructions.h"
#include "Environment/Device.h"
#include "Environment/VM.h"
#include <cstdio>

namespace JIT {

static Environment::Device* s_interpreterDevice;

void setGlobalInterpreterDevice(Environment::Device* device)
{
    s_interpreterDevice = device;
}

Environment::VM* executeEffect(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack, unsigned push, unsigned pop)
{
    // Should always write stack to memory before using stack methods
    if (stackPointer != state->m_stack.m_end) {
        *stackPointer = topOfStack;
    }
    state->m_stack.m_stackPointer = stackPointer;

    UNDERFLOW_CHECK(pop);
    // TODO optimise
    for (unsigned i = 0; i < pop; i++) {
        state->m_stack.pop();
    }

    OVERFLOW_CHECK(push);
    for (unsigned i = 0; i < push; i++) {
        state->m_stack.push(0);
    }
    return state;
}

Environment::VM* executeEffect(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack, Code::Instruction effect)
{
    return executeEffect(state, stackPointer, topOfStack, Bit::uintRegion((unsigned)effect, 4, 4), Bit::uintRegion((unsigned)effect, 0, 4));
}

Environment::VM* execute(Environment::VM* state)
{
    state = execute(state, state->m_stack.m_stackPointer, state->m_stack.peek());

    // Ensure sounder and LEDs are off after execution
    s_interpreterDevice->programHalted();

    return state;
}

void doFunc(Environment::VM** state, int32_t** stackPointer, int32_t* topOfStack, Environment::VMFunction func)
{
    func(*state, *stackPointer, *topOfStack);
    *stackPointer = (*state)->m_stack.m_stackPointer;
    if (*stackPointer != (*state)->m_stack.m_end) {
        *topOfStack = **stackPointer;
    }
}

Environment::VM* execute(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    while ((size_t)state->m_programCounter != state->m_code.length() && state->m_status == Environment::VMStatus::Success) {
        Code::Instruction current = state->m_code[state->m_programCounter];
        state->m_programCounter = state->m_programCounter + 1;
        switch (current) {
        case Code::Instruction::Add:
            UNDERFLOW_CHECK(2);
            stackPointer++; // Pop
            topOfStack = topOfStack + *stackPointer;
            break;
        case Code::Instruction::Sub:
            UNDERFLOW_CHECK(2);
            stackPointer++;
            topOfStack = *stackPointer - topOfStack;
            break;
        case Code::Instruction::Mul:
            UNDERFLOW_CHECK(2);
            stackPointer++;
            topOfStack = *stackPointer * topOfStack;
            break;
        case Code::Instruction::Div:
            doFunc(&state, &stackPointer, &topOfStack, &executeDiv);
            break;
        case Code::Instruction::Mod:
            doFunc(&state, &stackPointer, &topOfStack, &executeMod);
            break;
        case Code::Instruction::Inc:
            UNDERFLOW_CHECK(1);
            topOfStack++;
            break;
        case Code::Instruction::Dec:
            UNDERFLOW_CHECK(1);
            topOfStack--;
            break;
        case Code::Instruction::Max:
            doFunc(&state, &stackPointer, &topOfStack, &executeMax);
            break;
        case Code::Instruction::Min:
            doFunc(&state, &stackPointer, &topOfStack, &executeMin);
            break;
        case Code::Instruction::Lt:
            UNDERFLOW_CHECK(2);
            stackPointer++; // Pop
            topOfStack = *stackPointer < topOfStack ? 1 : 0;
            break;
        case Code::Instruction::Le:
            UNDERFLOW_CHECK(2);
            stackPointer++; // Pop
            topOfStack = *stackPointer <= topOfStack ? 1 : 0;
            break;
        case Code::Instruction::Eq:
            UNDERFLOW_CHECK(2);
            stackPointer++; // Pop
            topOfStack = *stackPointer == topOfStack ? 1 : 0;
            break;
        case Code::Instruction::Ge:
            UNDERFLOW_CHECK(2);
            stackPointer++; // Pop
            topOfStack = *stackPointer >= topOfStack ? 1 : 0;
            break;
        case Code::Instruction::Gt:
            UNDERFLOW_CHECK(2);
            stackPointer++; // Pop
            topOfStack = *stackPointer > topOfStack ? 1 : 0;
            break;
        case Code::Instruction::Drop:
            UNDERFLOW_CHECK(1);
            stackPointer++;
            topOfStack = *stackPointer;
            break;
        case Code::Instruction::Dup:
            UNDERFLOW_CHECK(1);
            *stackPointer = topOfStack;
            stackPointer--;
            break;
        case Code::Instruction::Ndup:
            UNDERFLOW_CHECK(topOfStack + 1);
            topOfStack = stackPointer[topOfStack];
            break;
        case Code::Instruction::Swap: {
            UNDERFLOW_CHECK(2);
            int temp = stackPointer[1];
            stackPointer[1] = topOfStack;
            topOfStack = temp;
            break;
        }
        case Code::Instruction::Rot:
            doFunc(&state, &stackPointer, &topOfStack, &executeRot);
            break;
        case Code::Instruction::Nrot:
            doFunc(&state, &stackPointer, &topOfStack, &executeNrot);
            break;
        case Code::Instruction::Tuck:
            doFunc(&state, &stackPointer, &topOfStack, &executeTuck);
            break;
        case Code::Instruction::Ntuck:
            doFunc(&state, &stackPointer, &topOfStack, &executeNtuck);
            break;
        case Code::Instruction::Size:
            doFunc(&state, &stackPointer, &topOfStack, &executeSize);
            break;
        case Code::Instruction::Nrnd:
            doFunc(&state, &stackPointer, &topOfStack, s_interpreterDevice->resolveVirtualMachineFunction(Code::Instruction::Nrnd));
            break;
        case Code::Instruction::Push8:
            OVERFLOW_CHECK(1);
            *stackPointer = topOfStack;
            stackPointer--; // Push
            topOfStack = state->m_code.decodeSigned8BitValue(state->m_programCounter);
            state->m_programCounter = state->m_programCounter + 1;
            break;
        case Code::Instruction::Push16:
            OVERFLOW_CHECK(1);
            *stackPointer = topOfStack;
            stackPointer--; // Push
            topOfStack = state->m_code.decodeSigned16BitValue(state->m_programCounter);
            state->m_programCounter += 2;
            break;
        case Code::Instruction::Jmp:
            UNDERFLOW_CHECK(1);
            state->m_programCounter = topOfStack;
            stackPointer++; // Pop
            topOfStack = *stackPointer;
            break;
        case Code::Instruction::Cjmp:
            UNDERFLOW_CHECK(2);
            if (stackPointer[1]) {
                state->m_programCounter = topOfStack;
            }
            stackPointer += 2; // Pop 2
            topOfStack = *stackPointer;
            break;
        case Code::Instruction::Fetch: {
            UNDERFLOW_CHECK(1);
            topOfStack = state->m_code.decodeSigned16BitValue(topOfStack);
            break;
        }
        case Code::Instruction::Call: {
            UNDERFLOW_CHECK(1);
            int pc = state->m_programCounter;
            state->m_programCounter = topOfStack;
            stackPointer += 1;
            topOfStack = *stackPointer;

            state = execute(state, stackPointer, topOfStack);

            stackPointer = state->m_stack.m_stackPointer;
            if (stackPointer != state->m_stack.m_end) {
                topOfStack = *stackPointer;
            }
            if (state->m_status == Environment::VMStatus::Success) {
                state->m_programCounter = pc;
            }
            break;
        }
        case Code::Instruction::Ret:
            if (stackPointer != state->m_stack.m_end) {
                *stackPointer = topOfStack;
            }
            state->m_stack.m_stackPointer = stackPointer;
            return state;
            break;
        case Code::Instruction::Wait:
            doFunc(&state, &stackPointer, &topOfStack, s_interpreterDevice->resolveVirtualMachineFunction(Code::Instruction::Wait));
            break;
        default: {
            if (isOptional(current)) {
                auto f = s_interpreterDevice->resolveVirtualMachineFunction(current);
                if (f) {
                    state = f(state, stackPointer, topOfStack);
                } else {
                    state = executeEffect(state, stackPointer, topOfStack, state->m_code[state->m_programCounter]);
                }
                // Same as doFunc
                stackPointer = state->m_stack.m_stackPointer;
                topOfStack = *stackPointer;
                if (state->m_status == Environment::VMStatus::Success) {
                    state->m_programCounter = state->m_programCounter + 1;
                }
            } else {
                // If an instruction is unrecognised or unimplemented then we
                // skip to the end as if there was an error
                state->m_programCounter = state->m_code.length();
            }
        } break;
        }
    }

    // Ensure that the state is reflected correctly on exit
    if (stackPointer != state->m_stack.m_end) {
        *stackPointer = topOfStack;
    }
    state->m_stack.m_stackPointer = stackPointer;
    return state;
}

Environment::VM* executeDiv(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    UNDERFLOW_CHECK(2);
    state->m_stack.m_stackPointer = stackPointer + 1; // Pop
    *(state->m_stack.m_stackPointer) = *(state->m_stack.m_stackPointer) / topOfStack;
    return state;
}

Environment::VM* executeMod(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    UNDERFLOW_CHECK(2);
    state->m_stack.m_stackPointer = stackPointer + 1; // Pop
    *(state->m_stack.m_stackPointer) = *(state->m_stack.m_stackPointer) % topOfStack;
    return state;
}

Environment::VM* executeMax(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    UNDERFLOW_CHECK(2);
    state->m_stack.m_stackPointer = stackPointer + 1; // Pop
    if (*(state->m_stack.m_stackPointer) < topOfStack) {
        *(state->m_stack.m_stackPointer) = topOfStack;
    }
    return state;
}

Environment::VM* executeMin(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    UNDERFLOW_CHECK(2);
    state->m_stack.m_stackPointer = stackPointer + 1; // Pop
    if (*(state->m_stack.m_stackPointer) > topOfStack) {
        *(state->m_stack.m_stackPointer) = topOfStack;
    }
    return state;
}

Environment::VM* executeRot(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    UNDERFLOW_CHECK(3);
    // Always need to write the stack;
    if (stackPointer != state->m_stack.m_end) {
        *stackPointer = topOfStack;
    }
    state->m_stack.m_stackPointer = stackPointer;
    state->m_stack.rotate(3);
    return state;
}

Environment::VM* executeNrot(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    UNDERFLOW_CHECK((uint32_t)topOfStack + 1);
    state->m_stack.m_stackPointer = stackPointer + 1; // Pop
    state->m_stack.rotate(topOfStack);
    return state;
}

Environment::VM* executeTuck(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    UNDERFLOW_CHECK(3);
    // Always need to write the stack;
    if (stackPointer != state->m_stack.m_end) {
        *stackPointer = topOfStack;
    }
    state->m_stack.m_stackPointer = stackPointer;
    state->m_stack.tuck(3);
    return state;
}

Environment::VM* executeNtuck(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    UNDERFLOW_CHECK(topOfStack + 1);
    state->m_stack.m_stackPointer = stackPointer + 1; // Pop
    state->m_stack.tuck(topOfStack);
    return state;
}

Environment::VM* executeSize(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    OVERFLOW_CHECK(1);
    // Always need to write the stack;
    if (stackPointer != state->m_stack.m_end) {
        *stackPointer = topOfStack;
    }
    state->m_stack.m_stackPointer = stackPointer;
    state->m_stack.push(state->m_stack.size());
    return state;
}
}