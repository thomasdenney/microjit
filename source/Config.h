#pragma once

#include "Enums.h"

/**
 * I don't really want to use this heap allocator:
 *  - It isn't a particularly good implementation of a heap allocator
 *  - It literally uses macros to redefine malloc and free, and then
 *    everything else relies on it
 *  - In order to use the macros you have to include this everywhere
 *    otherwise the normal allocation will be used, which will break
 *  - The default behaviour (disabled in config.json) is to panic on
 *    a full heap. I firmly believe that the default behaviour should
 *    be to return a null pointer
 */
#include "MicroBitHeapAllocator.h"

/**
 * This file contains general compilation parameters for the JIT as a whole.
 *
 * Please sort alphabetically
 */

const bool AllowPCRelativeLoads = false;

const bool AlwaysPrintCompilation = false;

const bool AlwaysPrintStaticAnalysis = false;

/**
 * Lower is brighter
 * The brightness values are bitwise left shifted by this value
 * 7 is the lowest value that will actually show anything
 * 4 is reasonably comfortable is daylight and artificial light
 * 0 is unreasonably bright
 */
const int BrightnessFactor = 4;

const bool BoundsCheckElimination = true;

/// Opposed to interpreting them
const bool CompileOptionalInstructionTests = true;

enum class ConditionalBranchType {
    Naive,
    FewerBranches
};

const ConditionalBranchType ConditionalBranchingMode = ConditionalBranchType::FewerBranches;

static const char* ConditionalBranchType_Strings[] = {
    STR_NAME(ConditionalBranchType::Naive),
    STR_NAME(ConditionalBranchType::FewerBranches)
};

/**
 * If the stack has a capacity of at least 16 (the default is 128, so this isn't a problem) this
 * will ensure that there are five zero values below the stack. By ensuring this is true we can
 * optimise certain load and store operations by avoiding redundant stack checks. The stack
 * capacity will just appear as slightly reduced.
 *
 * See Stack.cpp
 */
const bool EnsureZeroesAfterStack = true;

enum class ProjectMode {
    UnitTests,
    OptionalInstructionTests,
    SerialDeploy
};

const ProjectMode Mode = ProjectMode::SerialDeploy;

static const char* ProjectMode_Strings[] = {
    STR_NAME(ProjectMode::UnitTests),
    STR_NAME(ProjectMode::OptionalInstructionTests),
    STR_NAME(ProjectMode::SerialDeploy)
};

/**
 * The project mode must be UnitTests mode for this to do anything uesful.
 *
 * Enabling this:
 *  - Disables the use of the NeoPixel and sounder, so optional instruction tests do not work correctly
 *  - Doesn't print emoji in output
 *  - Doesn't perform correctness checks
 *  - Executes the |preTest| |TestExecutionCount| times to get a sample for how long that takes
 *  - Executes |preTest| and the code itself |TestExecutionCount| times for comparison
 */
const bool ProfilingEnabled = false;

enum class RegisterAllocation {
    Naive,
    Stack,
    StackWithCopyOnWrite,
    // TODO: intra-block scheduling algorithm based on Koopman's paper
};

const RegisterAllocation RegisterAllocationMode = RegisterAllocation::StackWithCopyOnWrite;

static const char* RegisterAllocation_Strings[] = {
    STR_NAME(RegisterAllocation::Naive),
    STR_NAME(RegisterAllocation::Stack),
    STR_NAME(RegisterAllocation::StackWithCopyOnWrite)
};

/**
 * Maintains an internal map of register contents so that writing to registers can be optimised
 * away until the last moment. Also allows for more efficient arithmetic operations in some cases
 */
const bool RegisterWriteElimination = true;

/**
 * Before reading the following, note that excluding stack checks can make the code 5-87% faster
 * in some tests.
 *
 * In the current implementation a stack check is placed at the start of each basic block.
 * For a basic block with only pops or only pushes, the stack check requires the following:
 *
 * Instruction          Cycles  Description
 * mov r5, pc           1       So that we know where the bounds check failed
 * mov r3, #popCount*4  1/2     offset from stack pointer. In the (unlikely) worst case this requires a load
 * add r3, r1, r3       1       In some cases this instruction and the previous can be a single instruction
 *                              Uses sub for pushes
 * ldr r4, [r0, #1 * 4] 2       Load address of the end of the stack (or start for pushes)
 * cmp r3, r4           1       Compare final stack address with new address
 * bgt stackUnderflow   1/2     If an overflow will occur branch to the check
 *                              If this is a long distance then this requires two branch instructions
 * nop/b                1/2     Always added, but could be an unconditional branch in case of a long
 *                              distance branch
 *
 * If the basic block pushes and pops then the same code is generated, but the first instruction is only
 * emitted once. Therefore for a basic block that only pushes/pops the cycle count is
 * 1 + 1 + 1/2 (possibly) + 2 + 1 + (2 or 3) = 7 to 10 cycles
 *
 * For a basic block that pushes and pops it is
 * (7 to 10) * 2 - 1 = 13 to 19 cycles
 *
 * Whilst these cycle counts are significant, the bigger problem is the code size. In the best case
 * a total of 6 instructions are emitted, in the worst case 15 are emitted, so 12-30  additional
 * bytes are required per basic block. This is quite a significant cost to incur.
 *
 * As an alternative, code can be generated such that the stack checks are done via function calls
 * For instance, for a function that only pops, the following code calls the stack check:
 *
 * Instrucrion          Cycles  Description
 * mov r3, #popCount*4  1/2     2 cycles if emitted as a load
 * bl stackUnderflow    3       Call the stack undeflow check
 *
 * Then the following code is emitted for the actual underflow check:
 *
 * Instruction          Cycles  Description
 * add r3, r1, r3       1       Sub in the case of pushes
 * ldr r4 [r0, #1 * 4]  2       Address of end of stack
 * cmp r3, r4           1       Comparison
 * bgt fail             1/2     Enter fail state, which is the same as before
 * ret                  2       Return from the function
 * fail: ...
 *
 * With this alternative approach we incur a cost 4 to 7 cycles (in case of pops and pushes) across
 * 6-8 bytes of instructions per basic block. The stack check code then (assuming success) then takes
 * a further 7-12 cycles to do the check, and the failure code is generated as before (only the address
 * is copied from LR rather than R5).
 *
 * Therefore a single stack check for a basic block now costs 11-19 cycles compared with 7-19 cycles
 * for the previous approach (0-57% slower). The main difference however is that 6-22 bytes of instructions are saved
 * per basic block, which for short basic blocks (the common case) is very significant. For larger programs
 * this therefore offers a pretty significant saving in size.
 *
 * The pathological case of 7 cycles to 11 cycles occurs when we had only a single pop/push and could
 * therefore previously emit a single add/sub instruction with a small immediate to immediately compute
 * the new stack pointer value. In my tests of small programs this occurs for about 57% of basic blocks, but in larger
 * programs I've tested it to be as high as 67%.
 *
 * Potential performance improvements that can be done:
 *
 *  -   Currently registers r8 and r9 are completely unused by any part of the compiler. These could be used
 *      to store the values of the stack base and stack end throughout program execution, which would eliminate
 *      the need to ever do loads, reducing the cost of each approach by 2-4 cycles and 2-4 bytes (depending on
 *      whether it pushes xor pops, or does both)
 *  -   Do the addition to the existing stack pointer just before calling the stack check code. This will reduce
 *      the cycle cost by 1 in cases where an add/sub operation can be used as described above, but increase
 *      the size per basic-block by 2 bytes
 *  -   Another test showed that 98% of all basic blocks (and 100% in larger, more representative programs) push
 *      and pop fewer than eight values. If we could keep a copy of the stack pointer, base address, and end address
 *      all divided by four then we could just use the add/sub small immediate instructions to determine the new
 *      stack addresses divided by four. The cost of dividing the stack pointer by four (through an arithmetic shift) is
 *      one cycle, which doesn't really gain us much. Another point: we could instead adjust the register
 *      containing the stack pointer and then correct it before running the code (the existing allocator can handle this, but
 *      the more naive allocators would need some extra code generation).
 *
 *      With or without function calls, but with the stack base/end stored in registers this would reduce the cycle
 *      count by 3 in most cases. The disadvantage of this approach is that you couldn't tell by how much the stack
 *      would overflow, but this can be derived from basic block metadata if absolutely necessary.
 *
 *      This approach will be implemented as BoundsCheckInPlaceOnStackPointerRegister
 */
enum class StackCheck {
    None,
    BoundsCheckInPlace
};

const StackCheck StackCheckMode = StackCheck::BoundsCheckInPlace;

static const char* StackCheck_Strings[] = {
    STR_NAME(StackCheck::None),
    STR_NAME(StackCheck::FewerBranches)
};

const bool TailCallsOptimised = true;

const int TestExecutionCount = 5;

const bool WriteCompiledCodeToFlash = true;
