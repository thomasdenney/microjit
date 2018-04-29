#pragma once

#include "Config.h"

#include <cstddef>

namespace JIT {

/// OFFSETS of special linker locations for error handling and the like
class SpecialLinkerLocations {
public:
    size_t m_locations[3];
};
}