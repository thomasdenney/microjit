#include "Tests.h"

#include "JIT/StaticAnalysis.h"
#include "Tests/Utilities.h"

namespace JIT {

bool testEmptyStaticAnalysis()
{
    Code::Array code(nullptr, 0);
    StaticAnalysis analysis(code);

    bool result = analysis.analyse() == StaticAnalysis::Status::Success;

    if (!result) {
        analysis.printStaticAnalyis();
    }
    return result;
}

static const Code::Instruction singleOptionalInstruction[] = { Code::Instruction::Temp, Code::Instruction::TempEffect };
bool testSingleOptionalInstruction()
{
    Code::Array code(singleOptionalInstruction, sizeof(singleOptionalInstruction) / sizeof(Code::Instruction));
    StaticAnalysis analysis(code);

    bool result = analysis.analyse() == StaticAnalysis::Status::Success;

    if (!result) {
        analysis.printStaticAnalyis();
    }
    return result;
}

bool testStaticAnalysis()
{
    printTestHeader("STATIC ANALYSIS TESTS");

    bool success = true;

    success &= TEST(testEmptyStaticAnalysis);
    success &= TEST(testSingleOptionalInstruction);

    return success;
}
}