#pragma once

#include "Config.h"

#include "ARM/Functor.h"
#include "Code/Array.h"
#include "Stack.h"
#include "VMStatus.h"

// Forward declaration
namespace JIT {
class Compiler;
}

namespace Environment {

struct VM;
/**
 * The type for the virtual machine state changes
 */
typedef VM* (*VMFunction)(VM*, int32_t*, int32_t);

struct VM {
    VM(Environment::Stack stack, Code::Array code)
        : m_stack(stack)
        , m_code(code)
        , m_programCounter(0)
        , m_escapeStackAddress(0)
        , m_escapePC(0)
        , m_errorPC(0)
        , m_jumpTable(nullptr)
        , m_entryFunctor(nullptr)
        , m_compiler(nullptr)
        , m_compileOrInterpretFunction(nullptr)
        , m_status(VMStatus::Success)
    {
    }

    Environment::Stack m_stack;
    Code::Array m_code;
    int32_t m_programCounter;
    uint32_t m_escapeStackAddress;
    uint32_t m_escapePC;
    uint32_t m_errorPC;
    uint16_t** m_jumpTable;
    /**
     * The most recent functor to be passed to the call method
     */
    ARM::Functor* m_entryFunctor;

    /// Ideally this would be a shared/unique pointer but I don't think this struct should necessarily have ownership
    JIT::Compiler* m_compiler;

    /// Function pointer that will be used to jump out for a period
    VMFunction m_compileOrInterpretFunction;
    VMStatus m_status;

    inline void reset()
    {
        m_programCounter = 0;
        m_status = VMStatus::Success;
    }

    inline VM* call(ARM::Functor& functor)
    {
        m_entryFunctor = &functor;
        m_jumpTable = functor.jumpTable();
        return ARM::typedCall<VM*, VM*, int32_t*, int32_t>(functor.fp(), this, m_stack.m_stackPointer, m_stack.peek());
    }
};
}