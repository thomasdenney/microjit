#include "Serial.h"

#include "MicroBit.h"
#include "Tests/Utilities.h"

extern MicroBit uBit;

namespace Transfer {

static uint8_t s_buffer[MAX_SERIAL_BUFFER_SIZE];
static size_t s_length;

inline int Serial::readUnsignedNumber()
{
    int read = uBit.serial.read(SYNC_SPINWAIT);
    if (read == MICROBIT_NO_DATA || read == MICROBIT_SERIAL_IN_USE || read == MICROBIT_NO_DATA) {
        return -1;
    }
    return read;
}

size_t Serial::readProgramLength()
{
    int lower = readUnsignedNumber();
    int upper = readUnsignedNumber();
    if (lower >= 0 && upper >= 0) {
        return ((unsigned)upper << 8) | (unsigned)lower;
    }
    return 0;
}

void Serial::readProgramSafe()
{
    size_t i = 0;
    while (i != s_length) {
        int read = readUnsignedNumber();
        if (read < 0) {
            s_length = 0;
            break;
        } else {
            s_buffer[i] = (uint8_t)read;
            ++i;
        }
    }
}

void Serial::readProgram()
{
    s_length = readProgramLength();
    readProgramSafe();
}

size_t Serial::programLength()
{
    return s_length;
}

uint8_t* Serial::programBuffer()
{
    return s_buffer;
}
}