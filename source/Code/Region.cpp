#include "Region.h"

#include <algorithm>

namespace Code {

bool Region::contains(size_t value) const
{
    return value >= start() && value < end();
}

Region Region::add(const Region& that) const
{
    size_t start = std::min(m_start, that.m_start);
    size_t end = std::max(m_start + m_length, that.m_start + that.m_length);
    return Region(start, end - start);
}
}