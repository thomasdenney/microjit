#include "Functor.h"

#include "Decoder.h"
#include "Transfer/Deserialiser.h"
#include "Transfer/Serialiser.h"

namespace ARM {

void Functor::reserve(size_t capacity)
{
    m_buffer.reserve(capacity);
}

void Functor::add(Instruction instruction)
{
    m_buffer.push_back(instruction);
    m_hasChanges = true;
}

void Functor::add(InstructionPair pair)
{
    add(pair.instruction1);
    add(pair.instruction2);
}

void Functor::addData(int data)
{
    add(data & 0xFFFF);
    add((data >> 16) & 0xFFFF);
}

void Functor::commit()
{
    asm("DSB\t\n"
        "ISB");
}

Instruction* Functor::buffer() const
{
    // Cast necessary because there are two overloads of data(), and the const one gets called here
    return (Instruction*)m_buffer.data();
}

Instruction** Functor::jumpTable() const
{
    return (Instruction**)m_jumpTable.data();
}

size_t Functor::length() const
{
    return m_buffer.size();
}

f_void Functor::fp()
{
    if (m_hasChanges) {
        commit();
        m_hasChanges = false;
    }
    return (f_void)((unsigned int)buffer() | 0x1);
}

void Functor::print()
{
    printFunction(fp(), length());
}

void Functor::attachJumpTable(std::vector<Instruction*>&& jumpTable)
{
    if (m_hasChanges) {
        commit();
    }
    m_jumpTable = jumpTable;
}

void Functor::serialise()
{
    Transfer::Serialiser serialiser("bytecode");
    serialiser.appendData((const uint8_t*)m_buffer.data(), m_buffer.size() * sizeof(Instruction));
}

void Functor::deserialise()
{
    Transfer::Deserialiser deserialiser("bytecode");
    if (deserialiser.exists() && deserialiser.length() > 0) {
        m_buffer = std::vector<Instruction>(deserialiser.length() / sizeof(Instruction), 0);
        deserialiser.readData((uint8_t*)m_buffer.data(), deserialiser.length());
        commit();
    }
}
}