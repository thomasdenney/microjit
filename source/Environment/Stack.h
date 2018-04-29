#pragma once

#include "Config.h"

#include <cstdint>
#include <cstdlib>

namespace Environment {

/**
 * Unsafe implementation of a word-sized stack
 * This implementation does not currently do any bounds checking.
 */
struct Stack {
    /**
     * These members are public so that their offsets can be computed by the JIT compiler
     */
    int32_t* m_stackPointer;
    int32_t* m_end;
    int32_t* m_base;

    Stack(int32_t* base, size_t capacity);

    void clear();

    size_t size();
    size_t capacity();

    int32_t pop();
    int32_t peek();
    int32_t peek(int32_t offset);
    void push(int32_t x);
    void rotate(int32_t n);
    void tuck(int32_t n);
    void duplicate(int32_t n);

    void print();

    bool empty();
    bool full();
};
}