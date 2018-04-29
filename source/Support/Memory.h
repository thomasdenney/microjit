#pragma once

#include "Config.h"

#include <memory>

namespace Support {

/**
 * C++11 support for make_unique, which wasn't introduced until C++14.
 * Based on http://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique#Example
 */
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
}