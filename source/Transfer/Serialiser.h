#pragma once

#include <cstdint>
#include <cstdlib>

namespace Transfer {

class Serialiser {
public:
    Serialiser(const char* filename);
    ~Serialiser();

    void appendData(const uint8_t* data, size_t length);
    void appendInt(const int i);
    void appendUnsignedInt(const unsigned int u);

private:
    int m_fileHandle = -1;
};
}