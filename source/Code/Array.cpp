#include "Array.h"

#include "Iterator.h"
#include <cstdio>

namespace Code {

int Array::decodeSigned8BitValue(size_t offset) const
{
    return signed8BitValueAtOffset(m_code, offset);
}

int Array::decodeSigned16BitValue(size_t offset) const
{
    return signed16BitValueAtOffset(m_code, offset);
}

int Array::decodeUnsigned16BitValue(size_t offset) const
{
    return (unsigned)m_code[offset] | ((unsigned)m_code[offset + 1] << 8);
}

void Array::print() const
{
    printf("------------------------------------------------\n");
    printf("Position\tOffset\tHex\t\tAssembly\n");
    for (Iterator iter(*this); !iter.finished(); ++iter) {
        const Instruction instruction = iter.instruction();
        const size_t i = iter.index();
        // Position, offset, hex
        printf("%08x\t%d\t%02x", (unsigned int)&m_code[i], (int)i, (unsigned int)instruction);
        if (instruction == Instruction::Push8) {
            printf("  %02x\t\t", (unsigned int)m_code[i + 1]);
        } else if (instruction == Instruction::Push16) {
            printf(" %02x %02x\t", (unsigned int)m_code[i + 1], (unsigned int)m_code[i + 2]);
        } else if (isOptional(instruction)) {
            printf(" %02x\t\t", (unsigned int)m_code[i + 1]);
        } else {
            printf("\t\t");
        }
        // Assembly
        printStackInstruction(&m_code[i]);
        printf("\n");
    }
}

Region Array::region() const
{
    return Region(0, m_length);
}

size_t Array::length() const
{
    return m_length;
}

Instruction* Array::buffer() const
{
    // Have to discard const modifier
    return (Instruction*)m_code;
}
}