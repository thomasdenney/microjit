#include "Iterator.h"

#include "Bit/Bit.h"

namespace Code {

bool Iterator::finished() const
{
    return index() >= m_region.end();
}

size_t Iterator::index() const
{
    return m_lastIndicies[0];
}

Region Iterator::region() const
{
    return m_region;
}

size_t Iterator::lastIndex() const
{
    return m_lastIndicies[1];
}

size_t Iterator::nPreviousIndex(int n) const
{
    return m_lastIndicies[n];
}

size_t Iterator::nextIndex() const
{
    switch (instruction()) {
    case Instruction::Push8:
        return index() + 2;
    case Instruction::Push16:
        return index() + 3;
    default:
        if (currentIsOptional()) {
            return index() + 2;
        }
        return index() + 1;
    }
}

bool Iterator::hasMoreInstructions() const
{
    return nextIndex() < m_region.end();
}

Iterator& Iterator::operator++()
{
    for (int i = NUMBER_OF_INDICIES_TO_TRACK - 1; i > 0; --i) {
        m_lastIndicies[i] = m_lastIndicies[i - 1];
    }
    m_lastIndicies[0] = nextIndex();
    return *this;
}

Instruction Iterator::instruction() const
{
    return m_code[index()];
}

Instruction Iterator::nextInstruction() const
{
    return m_code[nextIndex()];
}

bool Iterator::currentIsPush() const
{
    return isPush(instruction());
}

bool Iterator::currentIsSafePush() const
{
    if (instruction() == Instruction::Push8) {
        return index() + 1 < m_region.end();
    } else if (instruction() == Instruction::Push16) {
        return index() + 2 < m_region.end();
    }
    return false;
}

bool Iterator::lastWasPush() const
{
    if (lastIndex() != index()) {
        return isPush(m_code[lastIndex()]);
    }
    return false;
}

bool Iterator::twoPrevWasConditionalCheck() const
{
    if (lastIndex() != index() && m_lastIndicies[2] != lastIndex()) {
        return isCondition(m_code[m_lastIndicies[2]]);
    }
    return false;
}

int Iterator::pushValue() const
{
    size_t instrOffset;
    if (currentIsPush()) {
        instrOffset = index();
    } else if (lastWasPush()) {
        instrOffset = lastIndex();
    } else {
        return 0;
    }

    if (m_code[instrOffset] == Instruction::Push8) {
        return signed8BitValueAtOffset(m_code, instrOffset + 1);
    } else {
        return signed16BitValueAtOffset(m_code, instrOffset + 1);
    }
}

bool Iterator::currentIsOptional() const
{
    return isOptional(instruction()) && (index() + 1) < m_region.end();
}

bool Iterator::nextArePushAndConditionalJump() const
{
    Iterator iter = *this;
    if (iter.hasMoreInstructions()) {
        ++iter;
        if (iter.currentIsPush() && iter.hasMoreInstructions()) {
            return iter.nextInstruction() == Instruction::Cjmp;
        }
    }
    return false;
}

size_t Iterator::optionalPushCount() const
{
    return Bit::uintRegion((unsigned)m_code[index() + 1], 4, 4);
}

size_t Iterator::optionalPopCount() const
{
    return Bit::uintRegion((unsigned)m_code[index() + 1], 0, 4);
}
}