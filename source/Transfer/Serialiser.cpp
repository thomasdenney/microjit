#include "Serialiser.h"

#include "Config.h"
#include "Flash.h"

namespace Transfer {

Serialiser::Serialiser(const char* name)
{
    auto fs = microBitFileSystem();
    if (fs) {
        m_fileHandle = fs->open(name, MB_WRITE | MB_CREAT);

        if (m_fileHandle < 0) {
            fs->remove(name);
            m_fileHandle = fs->open(name, MB_WRITE | MB_CREAT);
        }
    }
}

Serialiser::~Serialiser()
{
    if (m_fileHandle >= 0) {
        auto fs = microBitFileSystem();
        if (fs) {
            fs->close(m_fileHandle);
            m_fileHandle = -1;
        }
    }
}

void Serialiser::appendData(const uint8_t* data, size_t length)
{
    auto fs = microBitFileSystem();
    if (m_fileHandle >= 0 && fs) {
        fs->write(m_fileHandle, (uint8_t*)data, (int)length);
    }
}

void Serialiser::appendInt(const int i)
{
    appendData((const uint8_t*)&i, sizeof(const int));
}

void Serialiser::appendUnsignedInt(const unsigned int u)
{
    appendData((const uint8_t*)&u, sizeof(const unsigned int));
}
}