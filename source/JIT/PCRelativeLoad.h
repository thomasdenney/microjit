#pragma once

#include "ARM/Functor.h"

namespace JIT {

/**
 * Behaves a lot like the regular linker operations, but because the linker doesn't
 * use this class I've avoided implementing it
 */
class PCRelativeLoad {
private:
    size_t m_instructionOffset;
    int m_value;
    ARM::Register m_destination;

public:
    /**
     * Will immediately insert a nop
     */
    PCRelativeLoad(ARM::Functor& func, int value, ARM::Register destination);

    void insertData(ARM::Functor& func);
};
}