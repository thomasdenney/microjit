#pragma once

#include "Config.h"

#include "Instruction.h"
#include "Region.h"
#include <cstdlib>

namespace Code {

/**
 * This class does not manage the memory associated with it!
 */
class Array {
public:
    Array(const Instruction* code, size_t length)
        : m_code((Instruction*)code)
        , m_length(length)
    {
    }

    const Instruction& operator[](int offset) const
    {
        return m_code[offset];
    }

    Instruction* buffer() const;

    size_t length() const;

    Region region() const;

    int decodeSigned8BitValue(size_t offset) const;
    int decodeSigned16BitValue(size_t offset) const;
    int decodeUnsigned16BitValue(size_t offset) const;

    void print() const;

    /**
     * These are publicly accessible so that their locations can be accessed by the compiler and the generated code.
     * In normal C++ code they should not be accessed.
     */
    Instruction* m_code;
    size_t m_length;
};
}