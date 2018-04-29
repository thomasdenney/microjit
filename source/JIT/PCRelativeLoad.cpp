#include "Config.h"

#include "PCRelativeLoad.h"

namespace JIT {

PCRelativeLoad::PCRelativeLoad(ARM::Functor& func, int value, ARM::Register destination)
{
    m_instructionOffset = func.length();
    func.add(ARM::nop());
    m_value = value;
    m_destination = destination;
}

void PCRelativeLoad::insertData(ARM::Functor& func)
{
    int length = func.length();
    func.addData(m_value);
    uint8_t offset = (length - ((m_instructionOffset + 2) & 0xFFFFFFFC)) / 2;
    func.buffer()[m_instructionOffset] = ARM::loadWordWithPCOffset(m_destination, offset);
}
}