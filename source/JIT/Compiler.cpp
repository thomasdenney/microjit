#include "Compiler.h"

#include "ARM/Decoder.h"
#include "ARM/Encoder.h"
#include "Bit/Bit.h"
#include "BoundsCheckCodeGenerator.h"
#include "Code/Iterator.h"
#include "DynamicCompilation.h"
#include "Interpreter.h"
#include "RegisterFileStateCOWAllocator.h"
#include "RegisterFileStateDefaultAllocator.h"
#include "Support/Memory.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>

namespace JIT {

/**
 * This is currently a poor man's estimate for the worst case
 * Currently the optional push 16/non-zero pop will yield 20 instructions, but this could be
 * prefaced by a stack push if at the start of the function (although this shouldn't be too
 * big a deal).
 */
const int MaxArmInstructionsPerStackInstruction = 21;

DynamicFunctionResult compileFunctionDynamically(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    state->m_stack.m_stackPointer = ++stackPointer;
    if (state->m_compiler) {
        ARM::Functor& func = *(state->m_entryFunctor);

        int32_t originalCodeLocation = (int32_t)func.buffer();

        if (!state->m_compiler->functionPointerForStackFunction(func, topOfStack)) {
            auto compilerSuccess = state->m_compiler->compileNewFunction(func, topOfStack);
            if (compilerSuccess != Compiler::Status::Success) {
                printf("Compiler failure, will return nullptr\n");
                return { state, nullptr, 0 };
            } else {
                if (AlwaysPrintCompilation) {
                    state->m_compiler->prettyPrintCode(func);
                }
            }
        }

        auto function = state->m_compiler->functionPointerForStackFunction(func, topOfStack);

        int32_t newCodeLocation = (int32_t)func.buffer();

        return { state, function, newCodeLocation - originalCodeLocation };
    } else {
        // Need to jump to the halt with error
        printf("WARNING: No compiler attached to handle dealing with function\n");
        return { state, nullptr, 0 };
    }
}

void popNextToTemp(ARM::Functor& func)
{
    func.add(ARM::addSmallImm(StackPointerRegister, StackPointerRegister, 4));
    func.add(ARM::loadWordWithOffset(TempRegister, StackPointerRegister, 0));
}

void compileDynamicCall(ARM::Functor& func)
{
    compileCFunctionCall(func, (Environment::VMFunction)((int)&compileFunctionDynamicallyASM | 0x1), false);
}

void compileAdd(ARM::Functor& func)
{
    popNextToTemp(func);
    func.add(ARM::addReg(StackTopRegister, StackTopRegister, TempRegister));
}

void compileSub(ARM::Functor& func)
{
    popNextToTemp(func);
    func.add(ARM::subReg(StackTopRegister, TempRegister, StackTopRegister));
}

void compileMul(ARM::Functor& func)
{
    popNextToTemp(func);
    func.add(ARM::mul(StackTopRegister, TempRegister));
}

void compileDiv(ARM::Functor& func)
{
    compileCFunctionCall(func, &executeDiv);
}

void compileMod(ARM::Functor& func)
{
    compileCFunctionCall(func, &executeMod);
}

void compileInc(ARM::Functor& func)
{
    func.add(ARM::addSmallImm(StackTopRegister, StackTopRegister, 1));
}

void compileDec(ARM::Functor& func)
{
    func.add(ARM::subSmallImm(StackTopRegister, StackTopRegister, 1));
}

void compileMax(ARM::Functor& func)
{
    // TODO Assembly version (already implemented in non-naive version)
    compileCFunctionCall(func, &executeMax);
}

void compileMin(ARM::Functor& func)
{
    // TODO Assembly version (already implemented in non-naive version)
    compileCFunctionCall(func, &executeMin);
}

void Compiler::compileRandom(ARM::Functor& func) const
{
    compileCFunctionCall(func, m_device->resolveVirtualMachineFunction(Code::Instruction::Nrnd));
}

void compileConditional(ARM::Functor& func, ARM::Condition c)
{
    popNextToTemp(func);
    func.add(ARM::compareLowRegisters(TempRegister, StackTopRegister));
    func.add(ARM::conditionalBranch(c, 1));
    func.add(ARM::moveImmediate(StackTopRegister, 0));
    func.add(ARM::unconditionalBranch(0));
    func.add(ARM::moveImmediate(StackTopRegister, 1));
}

void compileDrop(ARM::Functor& func)
{
    func.add(ARM::addSmallImm(StackPointerRegister, StackPointerRegister, 4));
    func.add(ARM::loadWordWithOffset(StackTopRegister, StackPointerRegister, 0));
}

void compileDup(ARM::Functor& func)
{
    func.add(ARM::storeWordWithOffset(StackTopRegister, StackPointerRegister, 0));
    func.add(ARM::subSmallImm(StackPointerRegister, StackPointerRegister, 4));
}

void compileNdup(ARM::Functor& func)
{
    func.add(ARM::logicalShiftLeftImmediate(StackTopRegister, StackTopRegister, 2)); // Multiply offset by 2
    func.add(ARM::loadWordWithRegisterOffset(StackTopRegister, StackPointerRegister, StackTopRegister));
}

void compileSwap(ARM::Functor& func)
{
    func.add(ARM::loadWordWithOffset(TempRegister, StackPointerRegister, 1));
    func.add(ARM::storeWordWithOffset(StackTopRegister, StackPointerRegister, 1));
    func.add(ARM::moveLowToLow(StackTopRegister, TempRegister));
}

void compileRot(ARM::Functor& func)
{
    compileCFunctionCall(func, &executeRot);
}

void compileNrot(ARM::Functor& func)
{
    compileCFunctionCall(func, &executeNrot);
}

void compileTuck(ARM::Functor& func)
{
    compileCFunctionCall(func, &executeTuck);
}

void compileNtuck(ARM::Functor& func)
{
    compileCFunctionCall(func, &executeNtuck);
}

void compileSize(ARM::Functor& func)
{
    compileCFunctionCall(func, &executeSize);
}

void compileLoadConstant(ARM::Functor& func, int value, ARM::Register destination, bool allowPcRelativeLoad, std::vector<PCRelativeLoad>& relativeLoads)
{
    if (allowPcRelativeLoad && (value < 0 || value > 0xFF)) {
        relativeLoads.push_back(PCRelativeLoad(func, value, destination));
        return;
    }
    compileLoadConstant(func, value, destination);
}

void compilePush(ARM::Functor& func, int value, bool allowPcRelativeLoad, std::vector<PCRelativeLoad>& relativeLoads)
{
    func.add(ARM::storeWordWithOffset(StackTopRegister, StackPointerRegister, 0));
    func.add(ARM::subSmallImm(StackPointerRegister, StackPointerRegister, 4));
    compileLoadConstant(func, value, StackTopRegister, allowPcRelativeLoad, relativeLoads);
}

void Compiler::compileWait(ARM::Functor& func) const
{
    compileCFunctionCall(func, m_device->resolveVirtualMachineFunction(Code::Instruction::Wait));
}

void compileFetch(ARM::Functor& func, ARM::Register fromRegister, ARM::Register toRegister)
{
    // SECTION ONE: LOAD 16-bit value form memory
    // TempRegister := address of code base
    func.add(ARM::loadWordWithOffset(TempRegister, StatePointerRegister, offsetof(Environment::VM, m_code.m_code) / sizeof(const Code::Instruction*)));
    // TempRegister := address of first (low) byte to load
    func.add(ARM::addReg(TempRegister, TempRegister, fromRegister));
    // StackTopRegister := first byte (low)
    func.add(ARM::loadByteWithOffset(toRegister, TempRegister, 0));
    // TempRegister := address of second (high) byte to load
    func.add(ARM::addSmallImm(TempRegister, TempRegister, 1));
    // TempRegister := second byte (high)
    func.add(ARM::loadByteWithOffset(TempRegister, TempRegister, 0));
    // Shift high byte
    func.add(ARM::logicalShiftLeftImmediate(TempRegister, TempRegister, 8));
    // Add high byte to low byte
    // StackTopRegister := full 16-bit value
    func.add(ARM::addReg(toRegister, toRegister, TempRegister));

    // SECTION TWO: Resolve twos complement
    func.add(ARM::signExtendHalfWord(toRegister, toRegister));
}

void compilePseudoCall(ARM::Functor& func)
{
    func.add(ARM::nop());
    func.add(ARM::nop());
}

void compileCall(ARM::Functor& func, int i, int destination)
{
    auto pair = ARM::branchAndLinkNatural(destination - i);
    func.buffer()[i] = pair.instruction1;
    func.buffer()[i + 1] = pair.instruction2;
}

void compileReturn(ARM::Functor& func)
{
    func.add(ARM::ret());
}

void compileReturnWithPop(ARM::Functor& func)
{
    func.add(ARM::popMultiple(true, ARM::RegisterList::empty));
}

void compileEntryCode(ARM::Functor& func)
{
    func.add(ARM::pushMultiple(true, ARM::RegisterList::r4 | ARM::RegisterList::r5 | ARM::RegisterList::r6 | ARM::RegisterList::r7));
    // Allows us to use the upper four variable registers
    func.add(ARM::moveGeneral(ARM::Register::r4, ARM::Register::r8));
    func.add(ARM::moveGeneral(ARM::Register::r5, ARM::Register::r9));
    func.add(ARM::moveGeneral(ARM::Register::r6, ARM::Register::r10));
    func.add(ARM::moveGeneral(ARM::Register::r7, ARM::Register::r11));
    func.add(ARM::pushMultiple(false, ARM::RegisterList::r4 | ARM::RegisterList::r5 | ARM::RegisterList::r6 | ARM::RegisterList::r7));

    func.add(ARM::moveGeneral(TempRegister, ARM::Register::sp));
    func.add(ARM::storeWordWithOffset(TempRegister, StatePointerRegister, offsetof(Environment::VM, m_escapeStackAddress) / 4));

    // Loads the stack base and end into registers
    func.add(ARM::loadWordWithOffset(TempRegister, StatePointerRegister, (uint8_t)offsetof(Environment::VM, m_stack.m_end) / sizeof(int32_t*)));
    func.add(ARM::moveGeneral(StackEndRegister, TempRegister));
    func.add(ARM::loadWordWithOffset(TempRegister, StatePointerRegister, (uint8_t)offsetof(Environment::VM, m_stack.m_base) / sizeof(int32_t*)));
    func.add(ARM::moveGeneral(StackBaseRegister, TempRegister));
}

void Compiler::compileHalt(ARM::Functor& func)
{
    m_linker.addHalt(func);
}

void Compiler::compileHaltCode(ARM::Functor& func)
{
    m_linker.setHaltOffset(func.length());
    compileWriteStateToMemory(func);
    func.add(ARM::loadWordWithOffset(TempRegister, StatePointerRegister, offsetof(Environment::VM, m_escapeStackAddress) / 4));
    func.add(ARM::moveGeneral(ARM::Register::sp, TempRegister));
    // Recover the values for the higher registers
    func.add(ARM::popMultiple(false, ARM::RegisterList::r4 | ARM::RegisterList::r5 | ARM::RegisterList::r6 | ARM::RegisterList::r7));
    func.add(ARM::moveGeneral(ARM::Register::r8, ARM::Register::r4));
    func.add(ARM::moveGeneral(ARM::Register::r9, ARM::Register::r5));
    func.add(ARM::moveGeneral(ARM::Register::r10, ARM::Register::r6));
    func.add(ARM::moveGeneral(ARM::Register::r11, ARM::Register::r7));
    // Recover the values for the lower registers
    func.add(ARM::popMultiple(true, ARM::RegisterList::r4 | ARM::RegisterList::r5 | ARM::RegisterList::r6 | ARM::RegisterList::r7));
}

void Compiler::compileStackOverflowCode(ARM::Functor& func)
{
    m_linker.setStackOverflowOffset(func.length());
    // The stack overflow check leaves the check PC in temp register 3
    func.add(ARM::storeWordWithOffset(TempRegister3, StatePointerRegister, offsetof(Environment::VM, m_errorPC) / sizeof(uint32_t)));
    func.add(ARM::moveImmediate(TempRegister, (uint8_t)Environment::VMStatus::StackOverflow));
    func.add(ARM::storeWordWithOffset(TempRegister, StatePointerRegister, offsetof(Environment::VM, m_status) / sizeof(int32_t*)));
    m_linker.addHalt(func);
}

void Compiler::compileStackUnderflowCode(ARM::Functor& func)
{
    m_linker.setStackUnderflowOffset(func.length());
    // The stack underflow check leaves the check PC in temp register 3
    func.add(ARM::storeWordWithOffset(TempRegister3, StatePointerRegister, offsetof(Environment::VM, m_errorPC) / sizeof(uint32_t)));
    func.add(ARM::moveImmediate(TempRegister, (uint8_t)Environment::VMStatus::StackUnderflow));
    func.add(ARM::storeWordWithOffset(TempRegister, StatePointerRegister, offsetof(Environment::VM, m_status) / sizeof(int32_t*)));
    m_linker.addHalt(func);
}

void Compiler::compileOptional(ARM::Functor& func, Code::Instruction optional, unsigned pushPop) const
{
    auto f = m_device->resolveVirtualMachineFunction(optional);
    if (f) {
        compileCFunctionCall(func, f);
    } else {
        int pushCount = Bit::uintRegion(pushPop, 4, 4);
        int popCount = Bit::uintRegion(pushPop, 0, 4);
        // If the function has side effects
        if (pushCount - popCount != 0) {
            if (popCount == 0) {
                func.add(ARM::storeWordWithOffset(StackTopRegister, StackPointerRegister, 0));
            } else {
                func.add(ARM::addLargeImm(StackPointerRegister, popCount * 4));
            }

            if (pushCount > 0) {
                func.add(ARM::moveImmediate(StackTopRegister, 0));
                func.add(ARM::subLargeImm(StackPointerRegister, pushCount * 4));
                for (int i = 0; i < pushCount; i++) {
                    func.add(ARM::storeWordWithOffset(StackTopRegister, StackPointerRegister, i));
                }
            } else {
                func.add(ARM::loadWordWithOffset(StackTopRegister, StackPointerRegister, 0));
            }
        }
    }
}

Compiler::Status Compiler::compileBasicBlockNaive(ARM::Functor& func, Code::Region basicBlock, Code::Region functionBlock, std::vector<PCRelativeLoad>& relativeLoads)
{
    int numberOfPushInstructions = 0;
    for (Code::Iterator iter(m_source, basicBlock); !iter.finished(); ++iter) {
        const int maximumRemainingInstructionEstimate = (functionBlock.end() - iter.index()) * MaxArmInstructionsPerStackInstruction;

        if (iter.index() == basicBlock.start()) {
            m_linker.setLinkOffset(iter.index(), func.length());
        }

        if (m_analysis.isCallDestination(iter.index())) {
            if (m_analysis.functionNeedsToPushRegisters(iter.index())) {
                func.add(ARM::pushMultiple(true, ARM::RegisterList::empty));
            }
        }

        if (StackCheckMode != StackCheck::None && iter.index() == basicBlock.start()) {
            BoundsCheckCodeGenerator::compile(m_analysis.stackEffectForBasicBlock(basicBlock), func, m_linker);
        }

        switch (iter.instruction()) {
        case Code::Instruction::Add:
            compileAdd(func);
            break;
        case Code::Instruction::Sub:
            compileSub(func);
            break;
        case Code::Instruction::Mul:
            compileMul(func);
            break;
        case Code::Instruction::Div:
            compileDiv(func);
            break;
        case Code::Instruction::Mod:
            compileMod(func);
            break;
        case Code::Instruction::Inc:
            compileInc(func);
            break;
        case Code::Instruction::Dec:
            compileDec(func);
            break;
        case Code::Instruction::Max:
            compileMax(func);
            break;
        case Code::Instruction::Min:
            compileMin(func);
            break;
        case Code::Instruction::Lt:
            compileConditional(func, ARM::Condition::lt);
            break;
        case Code::Instruction::Le:
            compileConditional(func, ARM::Condition::le);
            break;
        case Code::Instruction::Eq:
            compileConditional(func, ARM::Condition::eq);
            break;
        case Code::Instruction::Ge:
            compileConditional(func, ARM::Condition::ge);
            break;
        case Code::Instruction::Gt:
            compileConditional(func, ARM::Condition::gt);
            break;
        case Code::Instruction::Drop:
            compileDrop(func);
            break;
        case Code::Instruction::Dup:
            compileDup(func);
            break;
        case Code::Instruction::Ndup:
            compileNdup(func);
            break;
        case Code::Instruction::Swap:
            compileSwap(func);
            break;
        case Code::Instruction::Rot:
            compileRot(func);
            break;
        case Code::Instruction::Nrot:
            compileNrot(func);
            break;
        case Code::Instruction::Tuck:
            compileTuck(func);
            break;
        case Code::Instruction::Ntuck:
            compileNtuck(func);
            break;
        case Code::Instruction::Size:
            compileSize(func);
            break;
        case Code::Instruction::Nrnd:
            compileRandom(func);
            break;
        case Code::Instruction::Push8:
        case Code::Instruction::Push16:
            if (iter.currentIsSafePush() && !(iter.hasMoreInstructions() && isJumpOrCall(iter.nextInstruction()))) {
                // Using an *extremely* conservative estimate for whether or not we'll have space for push
                // instructions to use PC relative loads
                compilePush(func, iter.pushValue(), ((numberOfPushInstructions + maximumRemainingInstructionEstimate) < 0xFF) && AllowPCRelativeLoads, relativeLoads);
                numberOfPushInstructions++;
            }
            break;
        case Code::Instruction::Fetch:
            compileFetch(func, StackTopRegister, StackTopRegister);
            break;
        case Code::Instruction::Jmp: {
            if (iter.lastWasPush()) {
                auto destination = iter.pushValue();
                m_linker.addUnconditionalJump(func, destination, skipDistanceForBranch(basicBlock, destination));
            } else {
                printf("Unsupported non-constant jump at %d\n", (int)iter.index());
                return Status::UnsupportedVariableJump;
            }
            break;
        }
        case Code::Instruction::Cjmp: {
            if (iter.lastWasPush()) {
                auto destination = iter.pushValue();
                m_linker.addConditionalJump(func, destination, skipDistanceForBranch(basicBlock, destination));
            } else {
                printf("Unsupported non-constant conditional jump at %d\n", (int)iter.index());
                return Status::UnsupportedVariableJump;
            }
            break;
        }
        case Code::Instruction::Call: {
            compileCall(func, iter, basicBlock, functionBlock);
            break;
        }
        case Code::Instruction::Ret:
            if (m_analysis.functionNeedsToPushRegisters(functionBlock.start())) {
                compileReturnWithPop(func);
            } else {
                compileReturn(func);
            }
            break;
        case Code::Instruction::Halt:
            compileHalt(func);
            break;
        case Code::Instruction::Wait:
            compileWait(func);
            break;
        default: {
            if (isOptional(iter.instruction())) {
                compileOptional(func, m_source[iter.index()], (unsigned)m_source[iter.index() + 1]);
            }
            break;
        }
        }
    }
    return Status::Success;
}

int Compiler::skipDistanceForBranch(Code::Region basicBlock, int destination)
{
    auto destinationBlock = m_analysis.basicBlockAtIndex(destination);
    auto selfEffect = m_analysis.stackEffectForBasicBlock(basicBlock);
    auto destinationEffect = m_analysis.stackEffectForBasicBlock(destinationBlock);
    auto canSkipBoundsCheck = BoundsCheckElimination && selfEffect.supersedes(destinationEffect);
    auto skipCount = canSkipBoundsCheck ? BoundsCheckCodeGenerator::numberOfInstructions(destinationEffect) : 0;
    return skipCount;
}

Compiler::Status Compiler::compileBasicBlockStack(ARM::Functor& func, Code::Region basicBlock, Code::Region functionBlock, std::vector<PCRelativeLoad>& relativeLoads)
{
    auto stackEffect = m_analysis.stackEffectForBasicBlock(basicBlock);
    const bool functionReturnsViaPop = m_analysis.functionNeedsToPushRegisters(functionBlock.start());

    m_linker.setLinkOffset(basicBlock.start(), func.length());

    if (m_analysis.isCallDestination(basicBlock.start()) && functionReturnsViaPop) {
        func.add(ARM::pushMultiple(true, ARM::RegisterList::empty));
    }

    if (StackCheckMode != StackCheck::None) {
        BoundsCheckCodeGenerator::compile(m_analysis.stackEffectForBasicBlock(basicBlock), func, m_linker);
    }

    std::unique_ptr<RegisterFileState> registerState = RegisterAllocationMode == RegisterAllocation::Stack ? (std::unique_ptr<RegisterFileState>)Support::make_unique<RegisterFileStateDefaultAllocator>() : (std::unique_ptr<RegisterFileState>)Support::make_unique<RegisterFileStateCOWAllocator>();

    for (Code::Iterator iter(m_source, basicBlock); !iter.finished(); ++iter) {
        Compiler::Status status = Status::Success;

        switch (iter.instruction()) {
        case Code::Instruction::Add:
        case Code::Instruction::Sub:
        case Code::Instruction::Mul:
        case Code::Instruction::Max:
        case Code::Instruction::Min:
            status = compileTwoOperandNativeOp(func, *registerState, iter.instruction());
            break;
        case Code::Instruction::Lt:
        case Code::Instruction::Le:
        case Code::Instruction::Eq:
        case Code::Instruction::Ge:
        case Code::Instruction::Gt:
            if (ConditionalBranchingMode == ConditionalBranchType::FewerBranches && RegisterAllocationMode == RegisterAllocation::StackWithCopyOnWrite && iter.nextArePushAndConditionalJump()) {
                if (!registerState->returnToComparisonState(func)) {
                    return Status::RegisterAllocationError;
                }
            } else {
                status = compileTwoOperandNativeOp(func, *registerState, iter.instruction());
            }
            break;
        case Code::Instruction::Div:
        case Code::Instruction::Mod:
        case Code::Instruction::Nrnd:
        case Code::Instruction::Nrot:
        case Code::Instruction::Size:
        case Code::Instruction::Wait:
            status = compileNonNativeOp(func, *registerState, iter.instruction());
            break;
        case Code::Instruction::Ntuck: {
            auto topReg = registerState->readRegister(0);
            auto compiledNtuck = false;
            if (registerState->registerValueIsKnown(topReg)) {
                auto value = registerState->knownRegisterValue(topReg);
                if (0 < value && value <= 5) {
                    registerState->pop();
                    compiledNtuck = registerState->ntuck(func, value);
                }
            }

            if (!compiledNtuck) {
                status = compileNonNativeOp(func, *registerState, iter.instruction());
            }
            break;
        }
        case Code::Instruction::Ndup:
            // Original code I had for this is the best I think that can be done
            registerState->returnToNaiveState(func);
            compileNdup(func);
            break;
        case Code::Instruction::Inc:
        case Code::Instruction::Dec:
            status = compileOneOperandNativeOp(func, *registerState, iter.instruction());
            break;
        case Code::Instruction::Push8:
        case Code::Instruction::Push16:
            if (iter.currentIsSafePush() && !(iter.hasMoreInstructions() && isJumpOrCall(iter.nextInstruction()))) {
                status = compileRegisterAllocatedPush(func, *registerState, iter.pushValue(), false, relativeLoads);
            }
            break;
            break;
        case Code::Instruction::Drop:
            if (!registerState->dropTopOfStack(func)) {
                return Status::RegisterAllocationError;
            }
            break;
        case Code::Instruction::Dup:
            if (!registerState->dupTopOfStack(func)) {
                return Status::RegisterAllocationError;
            }
            break;
        case Code::Instruction::Rot:
            if (!registerState->rot(func)) {
                return Status::RegisterAllocationError;
            }
            break;
        case Code::Instruction::Swap:
            if (!registerState->swap(func)) {
                return Status::RegisterAllocationError;
            }
            break;
        case Code::Instruction::Tuck:
            if (!registerState->tuck(func)) {
                return Status::RegisterAllocationError;
            }
            break;
        case Code::Instruction::Fetch: {
            registerState->commitRegisterValue(func, 0);
            compileFetch(func, registerState->readRegister(0), registerState->topOfStackWriteBackRegister());
            break;
        }
        case Code::Instruction::Jmp: {
            if (!registerState->returnToNaiveState(func)) {
                return Status::RegisterAllocationError;
            }
            if (iter.lastWasPush()) {
                auto destination = iter.pushValue();
                m_linker.addUnconditionalJump(func, destination, skipDistanceForBranch(basicBlock, destination));
            } else {
                printf("Unsupported non-constant jump at %d\n", (int)iter.index());
                return Status::UnsupportedVariableJump;
            }
            break;
        }
        case Code::Instruction::Cjmp: {
            if (iter.lastWasPush()) {
                // Annoyingly it is only possible to eliminate the bounds check of the destination when the branch is taken
                auto destination = iter.pushValue();

                if (ConditionalBranchingMode == ConditionalBranchType::FewerBranches && RegisterAllocationMode == RegisterAllocation::StackWithCopyOnWrite && iter.twoPrevWasConditionalCheck()) {
                    auto cmpRegs = registerState->comparisonRegisters();
                    auto cmp = m_source[iter.nPreviousIndex(2)];
                    auto cond = ARM::Condition::eq;
                    switch (cmp) {
                    case Code::Instruction::Lt:
                        cond = ARM::Condition::lt;
                        break;
                    case Code::Instruction::Le:
                        cond = ARM::Condition::le;
                        break;
                    case Code::Instruction::Eq:
                        cond = ARM::Condition::eq;
                        break;
                    case Code::Instruction::Ge:
                        cond = ARM::Condition::ge;
                        break;
                    case Code::Instruction::Gt:
                        cond = ARM::Condition::gt;
                        break;
                    default: // Never occurs
                        break;
                    }
                    m_linker.addMinimalBranchConditionalJump(func, destination, skipDistanceForBranch(basicBlock, destination), cond, std::get<0>(cmpRegs), std::get<1>(cmpRegs));
                } else {
                    if (!registerState->returnToNaiveState(func)) {
                        return Status::RegisterAllocationError;
                    }
                    m_linker.addConditionalJump(func, destination, skipDistanceForBranch(basicBlock, destination));
                }
            } else {
                printf("Unsupported non-constant conditional jump at %d\n", (int)iter.index());
                return Status::UnsupportedVariableJump;
            }
            break;
        }
        case Code::Instruction::Call: {
            if (!registerState->returnToNaiveState(func)) {
                return Status::RegisterAllocationError;
            }
            compileCall(func, iter, basicBlock, functionBlock);
            break;
        }
        case Code::Instruction::Ret:
            if (!registerState->returnToNaiveState(func)) {
                return Status::RegisterAllocationError;
            }
            if (functionReturnsViaPop) {
                compileReturnWithPop(func);
            } else {
                compileReturn(func);
            }
            break;
        case Code::Instruction::Halt:
            if (!registerState->returnToNaiveState(func)) {
                return Status::RegisterAllocationError;
            }
            compileHalt(func);
            break;
        default: {
            if (isOptional(iter.instruction())) {
                if (!registerState->returnToNaiveState(func)) {
                    return Status::RegisterAllocationError;
                }
                compileOptional(func, m_source[iter.index()], (unsigned)m_source[iter.index() + 1]);
            }
            break;
        }
        }

        if (status != Status::Success) {
            return status;
        }
    }

    if (!registerState->inNaiveState()) {
        if (!registerState->returnToNaiveState(func)) {
            return Status::RegisterAllocationError;
        }
    }
    return Status::Success;
}

void Compiler::compileCall(ARM::Functor& func, Code::Iterator& iter, Code::Region basicBlock, Code::Region functionBlock)
{
    if (iter.lastWasPush()) {
        auto destination = iter.pushValue();
        if (!m_analysis.functionNeedsToPushRegisters(functionBlock.start()) && !m_analysis.functionNeedsToPushRegisters(iter.pushValue()) && iter.hasMoreInstructions() && iter.nextInstruction() == Code::Instruction::Ret) {
            m_linker.addUnconditionalJump(func, destination, skipDistanceForBranch(basicBlock, destination));
            // Important to skip the return instruction
            ++iter;
        } else {
            m_linker.addCall(func, destination);
        }
    } else {
        compileDynamicCall(func);
    }
}

Compiler::Status Compiler::compileOneOperandNativeOp(ARM::Functor& func, RegisterFileState& registerState, Code::Instruction instr)
{
    if (!registerState.ensureRegistersHoldValues(1, func)) {
        return Status::RegisterAllocationError;
    }

    auto top = registerState.pop();
    auto topIsKnown = registerState.registerValueIsKnown(top);
    auto topValue = registerState.knownRegisterValue(top);

    auto dest = registerState.push(func);

    if (topIsKnown) {
        switch (instr) {
        case Code::Instruction::Inc:
            registerState.setKnownRegisterValue(func, dest, topValue + 1);
            break;
        case Code::Instruction::Dec:
            registerState.setKnownRegisterValue(func, dest, topValue - 1);
            break;
        default:
            break;
        }
    } else {
        registerState.commitRegisterValue(func, top);
        switch (instr) {
        case Code::Instruction::Inc:
            func.add(ARM::addSmallImm(dest, top, 1));
            break;
        case Code::Instruction::Dec:
            func.add(ARM::subSmallImm(dest, top, 1));
            break;
        default:
            break;
        }
    }

    return Status::Success;
}

Compiler::Status Compiler::compileTwoOperandNativeOp(ARM::Functor& func, RegisterFileState& registerState, Code::Instruction instr)
{
    if (!registerState.ensureRegistersHoldValues(2, func)) {
        return Status::RegisterAllocationError;
    }

    auto top1 = registerState.pop();
    auto top2 = registerState.pop();

    auto top1IsKnown = registerState.registerValueIsKnown(top1);
    auto top2IsKnown = registerState.registerValueIsKnown(top2);

    if (!(top1IsKnown && top2IsKnown)) {
        registerState.commitRegisterValue(func, top1);
        registerState.commitRegisterValue(func, top2);
    }

    auto top1Value = registerState.knownRegisterValue(top1);
    auto top2Value = registerState.knownRegisterValue(top2);

    auto dest = registerState.push(func);

    if (top1IsKnown && top2IsKnown) {
        switch (instr) {
        case Code::Instruction::Add:
            registerState.setKnownRegisterValue(func, dest, top2Value + top1Value);
            break;
        case Code::Instruction::Sub:
            registerState.setKnownRegisterValue(func, dest, top2Value - top1Value);
            break;
        case Code::Instruction::Mul:
            registerState.setKnownRegisterValue(func, dest, top2Value * top1Value);
            break;
        case Code::Instruction::Div:
            registerState.setKnownRegisterValue(func, dest, top2Value / top1Value);
            break;
        case Code::Instruction::Mod:
            registerState.setKnownRegisterValue(func, dest, top2Value % top1Value);
            break;
        case Code::Instruction::Max:
            if (top1Value > top2Value) {
                registerState.setKnownRegisterValue(func, dest, top1Value);
            } else {
                registerState.setKnownRegisterValue(func, dest, top2Value);
            }
            break;
        case Code::Instruction::Min:
            if (top1Value < top2Value) {
                registerState.setKnownRegisterValue(func, dest, top1Value);
            } else {
                registerState.setKnownRegisterValue(func, dest, top2Value);
            }
            break;
        case Code::Instruction::Lt:
            if (top2Value < top1Value) {
                registerState.setKnownRegisterValue(func, dest, 1);
            } else {
                registerState.setKnownRegisterValue(func, dest, 0);
            }
            break;
        case Code::Instruction::Le:
            if (top2Value <= top1Value) {
                registerState.setKnownRegisterValue(func, dest, 1);
            } else {
                registerState.setKnownRegisterValue(func, dest, 0);
            }
            break;
        case Code::Instruction::Eq:
            if (top2Value == top1Value) {
                registerState.setKnownRegisterValue(func, dest, 1);
            } else {
                registerState.setKnownRegisterValue(func, dest, 0);
            }
            break;
        case Code::Instruction::Ge:
            if (top2Value >= top1Value) {
                registerState.setKnownRegisterValue(func, dest, 1);
            } else {
                registerState.setKnownRegisterValue(func, dest, 0);
            }
            break;
        case Code::Instruction::Gt:
            if (top2Value > top1Value) {
                registerState.setKnownRegisterValue(func, dest, 1);
            } else {
                registerState.setKnownRegisterValue(func, dest, 0);
            }
            break;
        default:
            break;
        }
    } else {
        switch (instr) {
        case Code::Instruction::Add:
            func.add(ARM::addReg(dest, top1, top2));
            break;
        case Code::Instruction::Sub:
            func.add(ARM::subReg(dest, top2, top1));
            break;
        case Code::Instruction::Mul:
            if (dest == top1) {
                func.add(ARM::mul(dest, top2));
            } else if (dest == top2) {
                func.add(ARM::mul(dest, top1));
            } else {
                // Given that we are playing with the top of stack I don't think this is ever executed
                func.add(ARM::moveLowToLow(TempRegister, top1));
                func.add(ARM::mul(TempRegister, top2));
                func.add(ARM::moveLowToLow(dest, TempRegister));
            }
            break;
        case Code::Instruction::Max:
            func.add(ARM::moveLowToLow(TempRegister, top2));
            func.add(ARM::compareLowRegisters(top1, top2));
            func.add(ARM::conditionalBranchNatural(ARM::Condition::le, 2));
            func.add(ARM::moveLowToLow(TempRegister, top1));
            func.add(ARM::moveLowToLow(dest, TempRegister));
            break;
        case Code::Instruction::Min:
            func.add(ARM::moveLowToLow(TempRegister, top2));
            func.add(ARM::compareLowRegisters(top1, top2));
            func.add(ARM::conditionalBranchNatural(ARM::Condition::ge, 2));
            func.add(ARM::moveLowToLow(TempRegister, top1));
            func.add(ARM::moveLowToLow(dest, TempRegister));
            break;
        case Code::Instruction::Lt:
        case Code::Instruction::Le:
        case Code::Instruction::Eq:
        case Code::Instruction::Ge:
        case Code::Instruction::Gt: {
            func.add(ARM::moveImmediate(TempRegister, 1));
            func.add(ARM::compareLowRegisters(top2, top1));
            switch (instr) {
            case Code::Instruction::Lt:
                func.add(ARM::conditionalBranch(ARM::Condition::lt, 0));
                break;
            case Code::Instruction::Le:
                func.add(ARM::conditionalBranch(ARM::Condition::le, 0));
                break;
            case Code::Instruction::Eq:
                func.add(ARM::conditionalBranch(ARM::Condition::eq, 0));
                break;
            case Code::Instruction::Ge:
                func.add(ARM::conditionalBranch(ARM::Condition::ge, 0));
                break;
            case Code::Instruction::Gt:
                func.add(ARM::conditionalBranch(ARM::Condition::gt, 0));
                break;
            default:
                break;
            }
            func.add(ARM::moveImmediate(TempRegister, 0));
            func.add(ARM::moveLowToLow(dest, TempRegister));
            break;
        }
        default:
            break;
        }
    }
    return Status::Success;
}

Compiler::Status Compiler::compileNonNativeOp(ARM::Functor& func, RegisterFileState& registerState, Code::Instruction instr)
{
    if (!registerState.returnToNaiveState(func)) {
        return Status::RegisterAllocationError;
    }
    switch (instr) {
    case Code::Instruction::Div:
        compileCFunctionCall(func, &executeDiv);
        break;
    case Code::Instruction::Mod:
        compileCFunctionCall(func, &executeMod);
        break;
    case Code::Instruction::Min:
        compileCFunctionCall(func, &executeMin);
        break;
    case Code::Instruction::Max:
        compileCFunctionCall(func, &executeMax);
        break;
    case Code::Instruction::Nrnd:
        compileCFunctionCall(func, m_device->resolveVirtualMachineFunction(Code::Instruction::Nrnd));
        break;
    case Code::Instruction::Nrot:
        compileCFunctionCall(func, &executeNrot);
        break;
    case Code::Instruction::Ntuck:
        compileCFunctionCall(func, &executeNtuck);
        break;
    case Code::Instruction::Size:
        compileCFunctionCall(func, &executeSize);
        break;
    case Code::Instruction::Wait:
        compileWait(func);
        break;
    default:
        break;
    }
    return Status::Success;
}

Compiler::Status Compiler::compileRegisterAllocatedPush(ARM::Functor& func, RegisterFileState& registerState, int value, bool allowPcRelativeLoad, std::vector<PCRelativeLoad>& relativeLoads)
{
    auto dest = registerState.push(func);
    registerState.setKnownRegisterValue(func, dest, value);
    return Status::Success;
}

Compiler::Status Compiler::compileFunction(ARM::Functor& func, Code::Region function)
{
    std::vector<PCRelativeLoad> relativeLoads;
    for (auto basicBlock : m_analysis.basicBlocksForFunction(function)) {
        Compiler::Status status;
        switch (RegisterAllocationMode) {
        case RegisterAllocation::Naive:
            status = compileBasicBlockNaive(func, basicBlock, function, relativeLoads);
            break;
        case RegisterAllocation::Stack:
        case RegisterAllocation::StackWithCopyOnWrite:
            status = compileBasicBlockStack(func, basicBlock, function, relativeLoads);
            break;
        }

        if (status != Status::Success) {
            return status;
        }
    }

    // The function should have terminated in all code paths by this point, so if not halt because
    // otherwise we will run into the next section
    compileHalt(func);

    // Compile the data section for this function
    if (relativeLoads.size() > 0) {
        if (func.length() % 2 == 1) {
            // Data needs to be aligned properly
            func.add(ARM::nop());
        }
        for (PCRelativeLoad& relativeLoad : relativeLoads) {
            relativeLoad.insertData(func);
        }
    }

    return Status::Success;
}

Compiler::Status Compiler::compile(ARM::Functor& functor)
{
    auto status = compileGeneral(functor, 0, true);
    notifyObservers(functor, status);
    return status;
}

Compiler::Status Compiler::compileNewFunction(ARM::Functor& functor, int start)
{
    auto status = compileGeneral(functor, start, false);
    notifyObservers(functor, status);
    return status;
}

Compiler::Status Compiler::compileGeneral(ARM::Functor& functor, int start, bool compileGlobal)
{
    auto analysisResult = m_analysis.analyse((size_t)start);
    if (analysisResult != StaticAnalysis::Status::Success) {
        printf("Static analysis failed: %s\n", StaticAnalysis::statusString(analysisResult));
        m_analysis.printStaticAnalyis();
        return Status::StaticAnalysisFailed;
    }
    if (AlwaysPrintStaticAnalysis) {
        m_analysis.printStaticAnalyis();
    }

    auto status = Status::Success;

    auto functions = m_analysis.newFunctionRegions();

    ARM::resetEncodingStatusFlags();

    if (compileGlobal) {
        // The halt support stores the LR for the function calling the functor
        // I now always compile halt support in case of error conditions
        compileEntryCode(functor);

        // Call the start of Stack code if it exists
        if (functions.size() > 0) {
            m_linker.addCall(functor, 0);
        }

        compileHaltCode(functor);
        if (StackCheckMode != StackCheck::None) {
            compileStackOverflowCode(functor);
            compileStackUnderflowCode(functor);
        }
    }

    for (auto f : functions) {
        status = compileFunction(functor, f);
        if (status != Status::Success) {
            return status;
        }
    }

    if (!m_linker.link(functor, m_analysis)) {
        return Status::LinkerFailed;
    }

    if (!ARM::checkEncodingStatusFlags()) {
        ARM::printEncodingStatusFlags();
        return Status::InstructionEncodingError;
    }

    functor.commit();

    return status;
}

Environment::VMFunction Compiler::functionPointerForStackFunction(const ARM::Functor& func, int offset)
{
    if (m_linker.hasOffsetForBasicBlock(offset)) {
        return (Environment::VMFunction)((uint32_t)(&func.buffer()[m_linker.offsetForBasicBlock(offset)]) | 0x1);
    } else {
        return nullptr;
    }
}

void Compiler::prettyPrintCode(ARM::Functor& func)
{
    func.print();
}

static const char* statusStrings[] = {
    "UnknownFailure",
    "Success",
    "StaticAnalysisFailed",
    "UnsupportedVariableJump",
    "RegisterAllocationError",
    "InstructionEncodingError",
    "LinkerFailed"
};

const char* Compiler::statusString(Status status)
{
    return statusStrings[(int)status];
}

Compiler::ObserverId Compiler::addObserver(Compiler::ObserverFunc&& func)
{
    m_observers.emplace_back(++m_nextObserverId, std::move(func));
    return m_nextObserverId;
}

bool Compiler::removeObserver(Compiler::ObserverId id)
{
    auto toRemove = std::remove_if(m_observers.begin(), m_observers.end(), [=](Compiler::Observer& observer) {
        return observer.first == id;
    });
    auto didRemove = toRemove != m_observers.end();
    m_observers.erase(toRemove);
    return didRemove;
}

void Compiler::notifyObservers(ARM::Functor& func, Status status)
{
    for (auto& observer : m_observers) {
        observer.second(func, status);
    }
}

void Compiler::serialise()
{
    m_linker.serialise();
    m_analysis.serialise();
}

void Compiler::deserialise()
{
    m_linker.deserialise();
    m_analysis.deserialise();
}
}