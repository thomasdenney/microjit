#pragma once

#include "Config.h"

#include "ARM/Functor.h"
#include "Code/Array.h"
#include "CodeGen.h"
#include "Environment/Device.h"
#include "Environment/VM.h"
#include "Linker.h"
#include "PCRelativeLoad.h"
#include "RegisterFileState.h"
#include "StaticAnalysis.h"
#include <functional>
#include <utility>
#include <vector>

namespace JIT {

struct DynamicFunctionResult {
    Environment::VM* m_virtualMachinePointer;
    Environment::VMFunction m_functionLocation;
    int32_t m_newCodeOffset;
};

DynamicFunctionResult compileFunctionDynamically(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

class Compiler {
public:
    Compiler(Code::Array source, Environment::Device* device)
        : m_source(source)
        , m_analysis(source)
        , m_device(device)
        , m_linker()
    {
    }

    enum class Status : uint32_t {
        UnknownFailure,
        Success,
        StaticAnalysisFailed,
        UnsupportedVariableJump,
        RegisterAllocationError,
        InstructionEncodingError,
        LinkerFailed
    };

    static const char* statusString(Status status);

    /**
     * Compiles the source code to a destination JIT function.
     */
    Status compile(ARM::Functor& func);

    /**
     * Used for compiling potentially uncompiled functions for dynamic calls, i.e. don't use for general compilation
     */
    Status compileNewFunction(ARM::Functor& func, int newFunction);

    /**
     * Returns nullptr in the event that the function hasn't been successfully compiled
     */
    Environment::VMFunction functionPointerForStackFunction(const ARM::Functor& func, int offset);

    void prettyPrintCode(ARM::Functor& func);

    /**
     * The compiler needs to be kept around and linked to the virtual machine context if this is true
     */
    bool hasDynamicCalls() const { return m_analysis.hasDynamicCalls(); }

    using ObserverId = size_t;
    using ObserverFunc = std::function<void(ARM::Functor&, Status)>;

    ObserverId addObserver(ObserverFunc&& observerFunc);

    /// Returns true on success
    bool removeObserver(ObserverId id);

    void serialise();
    void deserialise();

private:
    const Code::Array m_source;
    StaticAnalysis m_analysis;

    const Environment::Device* m_device;
    Linker m_linker;

    // Compilation phases
    Status compileGeneral(ARM::Functor& func, int start, bool compileGlobal);

    Status compileFunction(ARM::Functor& func, Code::Region function);

    /**
     * The standard compilation approach that we can always fall back to if a basic block pushes too many values to fit
     * in registers
     */
    Status compileBasicBlockNaive(ARM::Functor& func, Code::Region basicBlock, Code::Region functionBlock, std::vector<PCRelativeLoad>& relativeLoads);

    /**
     * A smarter approach that pushes into registers whilst it can
     * Currently doesn't aim to do anything with getting lower stack elements into registers
     *
     * Note: [deliberately] doesn't yet support PC relative loads
     */
    Status compileBasicBlockStack(ARM::Functor& func, Code::Region basicBlock, Code::Region functionBlock, std::vector<PCRelativeLoad>& relativeLoads);

    int skipDistanceForBranch(Code::Region currentBlock, int destination);

    /**
     * Used for the register allocation compiler only
     */
    Status compileTwoOperandNativeOp(ARM::Functor& func, RegisterFileState& registerState, Code::Instruction instr);

    Status compileOneOperandNativeOp(ARM::Functor& func, RegisterFileState& registerState, Code::Instruction instr);

    Status compileNonNativeOp(ARM::Functor& func, RegisterFileState& registerState, Code::Instruction instr);

    /**
     * Used only for the register allocation compiler
     */
    Status compileRegisterAllocatedPush(ARM::Functor& func, RegisterFileState& registerState, int value, bool allowPcRelativeLoad, std::vector<PCRelativeLoad>& relativeLoads);

    /// Shared between different approaches so that TCO works regardless
    void compileCall(ARM::Functor& func, Code::Iterator& iter, Code::Region thisBasicBlock, Code::Region thisFunctionBlock);

    void compileHalt(ARM::Functor& func);

    void compileHaltCode(ARM::Functor& func);
    void compileStackOverflowCode(ARM::Functor& func);
    void compileStackUnderflowCode(ARM::Functor& func);

    void compileRandom(ARM::Functor& func) const;
    void compileWait(ARM::Functor& func) const;
    void compileOptional(ARM::Functor& func, Code::Instruction optional, unsigned pushPop) const;

    using Observer = std::pair<ObserverId, std::function<void(ARM::Functor&, Status)>>;

    std::vector<Observer> m_observers;
    ObserverId m_nextObserverId = 0;

    void notifyObservers(ARM::Functor& func, Status status);
};
}