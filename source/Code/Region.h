#pragma once

#include "Config.h"

#include <cstddef>

namespace Code {

/**
 * Used for representing a window into an array
 */
class Region {
public:
    Region()
        : Region(0, 0)
    {
    }
    Region(size_t start, size_t length)
        : m_start(start)
        , m_length(length)
    {
    }

    size_t start() const { return m_start; }
    size_t length() const { return m_length; }
    size_t end() const { return m_start + m_length; }

    bool operator==(const Region& region) const
    {
        return m_start == region.m_start && m_length == region.m_length;
    }

    bool operator<(const Region& region) const
    {
        return m_start < region.m_start || (m_start == region.m_start && m_length < region.m_length);
    }

    bool contains(size_t value) const;

    Region add(const Region& that) const;

private:
    size_t m_start, m_length;
};
}
