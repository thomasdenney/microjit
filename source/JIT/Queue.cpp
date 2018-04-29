#include "Queue.h"

namespace JIT {

Queue::~Queue()
{
    if (m_buffer) {
        free(m_buffer);
    }
}

void Queue::reset()
{
    m_len = 0;
    m_start = 0;
}

bool Queue::empty() const
{
    return m_len == 0;
}

uint16_t Queue::pop()
{
    uint16_t value = m_buffer[m_start];
    m_start = (m_start + 1) % m_capacity;
    m_len -= 1;
    return value;
}

bool Queue::push(uint16_t value)
{
    if (m_len == m_capacity) {
        uint16_t oldCapacity = m_capacity;
        if (oldCapacity > 0) {
            m_capacity *= 2;
        } else {
            m_capacity = 1;
        }
        uint16_t* newBuffer = (uint16_t*)malloc(m_capacity * sizeof(uint16_t));
        if (newBuffer) {
            for (uint16_t i = 0; i < m_len; ++i) {
                newBuffer[i] = m_buffer[(m_start + i) % oldCapacity];
            }
            if (m_buffer) {
                free(m_buffer);
            }
            m_buffer = newBuffer;
            m_start = 0;
        } else {
            printf("WARNING: Queue failed to allocate\n");
            return false;
        }
    }
    m_buffer[(m_start + m_len) % m_capacity] = value;
    m_len += 1;
    return true;
}
}