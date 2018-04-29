#pragma once

#include <cstdint>
#include <cstdlib>

namespace Transfer {

class Deserialiser {
public:
    Deserialiser(const char* filename);
    ~Deserialiser();

    bool exists() const { return m_fileHandle >= 0; }
    size_t length() const { return m_length; };

    void readData(uint8_t* data, size_t length);
    int readInt();
    unsigned int readUnsignedInt();

private:
    int m_fileHandle = -1;
    size_t m_length = 0;
};
}