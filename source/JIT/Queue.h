#pragma once

#include "Config.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace JIT {

/**
 * A queue designed to be conservative with memory usage with the expectation that queues are shortlived.
 * 
 * It is not safe and doesn't do bounds checking
 * 
 * This class was literally just introduced because std::queue seemed to be eating RAM
 * 
 * The micro:bit allocator for some reason doesn't support delete[], which lead to memory leaks :(
 * so this class uses malloc and free
 */
class Queue {
private:
    uint16_t* m_buffer;
    uint16_t m_start;
    uint16_t m_len;
    uint16_t m_capacity;

public:
    Queue()
        : m_buffer(nullptr)
        , m_start(0)
        , m_len(0)
        , m_capacity(0)
    {
    }

    ~Queue();

    void reset();
    bool empty() const;
    uint16_t pop();
    bool push(uint16_t value);
};
}