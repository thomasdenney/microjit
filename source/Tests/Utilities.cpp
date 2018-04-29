#include "Utilities.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static int successCount = 0;
static int failureCount = 0;
// 74 is the width of a window that takes up a third of my laptop screen
const static size_t terminalWidth = 74;

bool printResult(bool result, const char* name)
{
    if (result) {
        incSuccess();
    } else {
        incFailure();
    }
    if (ProfilingEnabled) {
        // When profiling is enabled we don't want the emoji in the name
        printf("%s\n", name);
    } else {
        printf("%s %s\n", result ? "\u2705" : "\u274C", name);
    }
    return result;
}

void printTestStats()
{
    printf("Success rate: %d success%s, %d failure%s\n",
        successCount,
        successCount == 1 ? "" : "es",
        failureCount,
        failureCount == 1 ? "" : "s");
}

void printConfiguration()
{
#define BOOL_PRINT(name) printf("%s = %s\n", #name, name ? "true" : "false")
#define INT_PRINT(name) printf("%s = %d\n", #name, (int)name)
#define ENUM_PRINT(name, table) printf("%s = %s\n", #name, table[(int)name])

    BOOL_PRINT(AllowPCRelativeLoads);
    BOOL_PRINT(AlwaysPrintCompilation);
    BOOL_PRINT(AlwaysPrintStaticAnalysis);
    INT_PRINT(BrightnessFactor);
    BOOL_PRINT(BoundsCheckElimination);
    BOOL_PRINT(CompileOptionalInstructionTests);
    ENUM_PRINT(ConditionalBranchingMode, ConditionalBranchType_Strings);
    BOOL_PRINT(EnsureZeroesAfterStack);
    ENUM_PRINT(Mode, ProjectMode_Strings);
    BOOL_PRINT(ProfilingEnabled);
    ENUM_PRINT(RegisterAllocationMode, RegisterAllocation_Strings);
    BOOL_PRINT(RegisterWriteElimination);
    ENUM_PRINT(StackCheckMode, StackCheck_Strings);
    BOOL_PRINT(TailCallsOptimised);
    INT_PRINT(TestExecutionCount);
}

void incSuccess()
{
    successCount++;
}

void incFailure()
{
    failureCount++;
}

void printLine()
{
    for (size_t i = 0; i != terminalWidth; ++i) {
        putchar('-');
    }
    putchar('\n');
}

void printTestHeader(const char* str)
{
    printLine();
    size_t padding = terminalWidth / 2 - strlen(str) / 2;
    for (size_t i = 0; i != padding; ++i) {
        putchar(' ');
    }
    printf("%s\n", str);
    printLine();
}

size_t estimateOfFreeMemory()
{
    size_t higher = 1;
    void* attempt = malloc(higher);
    while (attempt != nullptr) {
        free(attempt);
        higher *= 2;
        attempt = malloc(higher);
    }

    if (attempt) {
        free(attempt);
        attempt = nullptr;
    }

    size_t lower = higher / 2;
    while (lower != higher - 1) {
        if (attempt) {
            free(attempt);
        }

        size_t mid = (lower + higher) / 2;
        attempt = malloc(mid);
        if (attempt) {
            lower = mid;
        } else {
            higher = mid;
        }
    }

    if (attempt) {
        free(attempt);
    }

    return lower;
}

void printEstimateOfFreeMemory()
{
    printf("Free memory = %d\n", (int)estimateOfFreeMemory());
}