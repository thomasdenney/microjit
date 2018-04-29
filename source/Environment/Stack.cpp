#include "Stack.h"

#include <cstdio>

namespace Environment {

Stack::Stack(int32_t* base, size_t capacity)
{
    this->m_base = base;
    m_end = m_stackPointer = base + capacity;
    if (capacity > 16 && EnsureZeroesAfterStack) {
        for (int i = 0; i < 5; ++i) {
            push(0);
        }
        m_end = m_stackPointer;
    }
}

void Stack::clear()
{
    m_stackPointer = m_end;
}

size_t Stack::size()
{
    return (size_t)(m_end - m_stackPointer);
}

size_t Stack::capacity()
{
    return (size_t)(m_end - m_base);
}

int32_t Stack::pop()
{
    int32_t x = *m_stackPointer;
    m_stackPointer++;
    return x;
}

int32_t Stack::peek()
{
    return peek(0);
}

int32_t Stack::peek(int32_t offset)
{
    return m_stackPointer[offset];
}

void Stack::push(int32_t x)
{
    m_stackPointer--;
    *m_stackPointer = x;
}

void Stack::rotate(int32_t n)
{
    int32_t bottom = m_stackPointer[n - 1];
    for (int32_t offset = n - 1; offset > 0; offset--) {
        m_stackPointer[offset] = m_stackPointer[offset - 1];
    }
    m_stackPointer[0] = bottom;
}

void Stack::tuck(int32_t n)
{
    int32_t top = m_stackPointer[0];
    for (int32_t offset = 0; offset < n - 1; offset++) {
        m_stackPointer[offset] = m_stackPointer[offset + 1];
    }
    m_stackPointer[n - 1] = top;
}

void Stack::duplicate(int32_t n)
{
    push(*(m_stackPointer + n));
}

void Stack::print()
{
    int32_t* stack = m_end - 1;
    while (stack >= m_stackPointer) {
        printf("%d\t", (int)*stack);
        stack--;
    }
    printf("\n");
}

bool Stack::full()
{
    return m_stackPointer <= m_base;
}

bool Stack::empty()
{
    return m_stackPointer >= m_end;
}
}