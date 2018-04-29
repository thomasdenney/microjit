#include "Deserialiser.h"

#include "Config.h"
#include "Flash.h"

namespace Transfer {

Deserialiser::Deserialiser(const char* name)
{
    auto fs = microBitFileSystem();
    if (fs) {
        m_fileHandle = fs->open(name, MB_READ);
        if (m_fileHandle >= 0) {
            m_length = fs->seek(m_fileHandle, 0, MB_SEEK_END);
            fs->seek(m_fileHandle, 0, MB_SEEK_SET);
        }
    }
}

Deserialiser::~Deserialiser()
{
    if (m_fileHandle >= 0) {
        auto fs = microBitFileSystem();
        if (fs) {
            fs->close(m_fileHandle);
            m_fileHandle = -1;
        }
    }
}

void Deserialiser::readData(uint8_t* buffer, size_t length)
{
    auto fs = microBitFileSystem();
    if (m_fileHandle >= 0 && fs) {
        fs->read(m_fileHandle, buffer, length);
    }
}

int Deserialiser::readInt()
{
    int data = 0;
    readData((uint8_t*)&data, sizeof(int));
    return data;
}

unsigned int Deserialiser::readUnsignedInt()
{
    unsigned int data = 0;
    readData((uint8_t*)&data, sizeof(unsigned int));
    return data;
}
}