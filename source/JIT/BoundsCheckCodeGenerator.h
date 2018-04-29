#pragma once

#include "Code/BlockStackEffect.h"
#include "Linker.h"

namespace JIT {

class BoundsCheckCodeGenerator {
private:
    ARM::Functor* m_func;
    Linker* m_linker;

    BoundsCheckCodeGenerator(ARM::Functor* func, Linker* linker)
        : m_func(func)
        , m_linker(linker)
    {
    }

    void compile(Code::BlockStackEffect effect);

public:
    static void compile(Code::BlockStackEffect effect, ARM::Functor& func, Linker& linker);

    static size_t numberOfInstructions(Code::BlockStackEffect effect);
};
}