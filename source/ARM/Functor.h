#pragma once

#include "Config.h"

#include "Encoder.h"
#include <cstdlib>
#include <vector>

namespace ARM {

typedef void (*f_void)(void);

template <typename ReturnType, typename... ArgumentTypes>
constexpr inline ReturnType typedCall(f_void fp, ArgumentTypes... args)
{
    typedef ReturnType (*auto_f)(ArgumentTypes...);
    return ((auto_f)fp)(args...);
}

/**
 * Wrapper for constructing functions at runtime and calling them
 *
 * Due to the Halting Problem it is not possible to guarantee that the function
 * will return, so please ensure that you add the return instruction.
 */
class Functor {
private:
    std::vector<Instruction> m_buffer;
    std::vector<Instruction*> m_jumpTable;
    bool m_hasChanges;

public:
    Functor()
        : m_buffer(0)
        , m_jumpTable(0)
        , m_hasChanges(false)
    {
    }

    void serialise();
    void deserialise();

    Instruction** jumpTable() const;
    Instruction* buffer() const;
    size_t length() const;
    f_void fp();

    /**
     * Issues the ISB, DSB instruction sequence to invalidate processor cache
     */
    void commit();

    /**
     * WARNING: This should be removed once the relocating mechanism has been written
     */
    void reserve(size_t minimumInstructions);

    void add(Instruction x);
    void add(InstructionPair pair);
    void addData(int data);

    /**
     * This function will call commit
     */
    void attachJumpTable(std::vector<Instruction*>&& jumpTable);

    template <typename ReturnType, typename... ArgumentTypes>
    inline ReturnType call(ArgumentTypes... args)
    {
        return typedCall<ReturnType, ArgumentTypes...>(fp(), args...);
    }

    void print();
};
}