#include "MarksExecutionTests.h"

namespace JIT {

Code::Instruction EnumTestCode[] = {
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x04,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0xFF,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x18, (Code::Instruction)0x05,
    (Code::Instruction)0x10,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x29,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x09,
    (Code::Instruction)0x19, (Code::Instruction)0xA3, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x75,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x04,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x3F,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x18, (Code::Instruction)0x09,
    (Code::Instruction)0x19, (Code::Instruction)0xA3, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x75,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x55,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x09,
    (Code::Instruction)0x19, (Code::Instruction)0xA3, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x75,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x6B,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x18, (Code::Instruction)0x09,
    (Code::Instruction)0x19, (Code::Instruction)0xA3, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x75,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x18, (Code::Instruction)0x05,
    (Code::Instruction)0x10,
    (Code::Instruction)0x18, (Code::Instruction)0x05,
    (Code::Instruction)0x19, (Code::Instruction)0xA3, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x04,
    (Code::Instruction)0x10,
    (Code::Instruction)0x18, (Code::Instruction)0x06,
    (Code::Instruction)0x10,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x19, (Code::Instruction)0x8C, (Code::Instruction)0x00,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x19, (Code::Instruction)0xE8, (Code::Instruction)0x03,
    (Code::Instruction)0x18, (Code::Instruction)0x08,
    (Code::Instruction)0x19, (Code::Instruction)0xA3, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x20,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x19, (Code::Instruction)0x94, (Code::Instruction)0x00,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x11,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x06,
    (Code::Instruction)0x19, (Code::Instruction)0x8D, (Code::Instruction)0x00,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x00,
    (Code::Instruction)0x10,
    (Code::Instruction)0x00,
    (Code::Instruction)0x05,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x05,
    (Code::Instruction)0x13,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x06,
    (Code::Instruction)0x15,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x00,
    (Code::Instruction)0x10,
    (Code::Instruction)0x00,
    (Code::Instruction)0x05,
    (Code::Instruction)0x1C
};

size_t EnumTestCodeLength = sizeof(EnumTestCode) / sizeof(Code::Instruction);

Code::Instruction LinkTest2Code[] = {
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x04,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0xFF,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x18, (Code::Instruction)0x08,
    (Code::Instruction)0x18, (Code::Instruction)0x06,
    (Code::Instruction)0x10,
    (Code::Instruction)0x19, (Code::Instruction)0xBF, (Code::Instruction)0x01,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x2B,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x05,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x19, (Code::Instruction)0x87, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x20,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x18, (Code::Instruction)0x09,
    (Code::Instruction)0x19, (Code::Instruction)0x9D, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x04,
    (Code::Instruction)0x10,
    (Code::Instruction)0x18, (Code::Instruction)0x06,
    (Code::Instruction)0x10,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x48,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x19, (Code::Instruction)0xE8, (Code::Instruction)0x03,
    (Code::Instruction)0x18, (Code::Instruction)0x08,
    (Code::Instruction)0x19, (Code::Instruction)0x9D, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x08,
    (Code::Instruction)0x19, (Code::Instruction)0xAC, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x60,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x05,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x19, (Code::Instruction)0x87, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x20,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x19, (Code::Instruction)0xD0, (Code::Instruction)0x07,
    (Code::Instruction)0x18, (Code::Instruction)0x08,
    (Code::Instruction)0x19, (Code::Instruction)0x9D, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x08,
    (Code::Instruction)0x18, (Code::Instruction)0x08,
    (Code::Instruction)0x19, (Code::Instruction)0xD6, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x19, (Code::Instruction)0x85, (Code::Instruction)0x00,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x05,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x19, (Code::Instruction)0x87, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x20,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x20,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x19, (Code::Instruction)0x8E, (Code::Instruction)0x00,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x11,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x06,
    (Code::Instruction)0x19, (Code::Instruction)0x87, (Code::Instruction)0x00,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x00,
    (Code::Instruction)0x10,
    (Code::Instruction)0x00,
    (Code::Instruction)0x05,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x05,
    (Code::Instruction)0x13,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x06,
    (Code::Instruction)0x15,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x00,
    (Code::Instruction)0x10,
    (Code::Instruction)0x00,
    (Code::Instruction)0x05,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x18, (Code::Instruction)0x07,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x19, (Code::Instruction)0x95, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x19, (Code::Instruction)0x95, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x01,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x01,
    (Code::Instruction)0x11,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x19, (Code::Instruction)0x9D, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x00,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x19, (Code::Instruction)0x87, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x05,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x18, (Code::Instruction)0x04,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x05,
    (Code::Instruction)0x10,
    (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x01,
    (Code::Instruction)0x19, (Code::Instruction)0x33, (Code::Instruction)0x01,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x19, (Code::Instruction)0x09, (Code::Instruction)0x01,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x05,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x19, (Code::Instruction)0x87, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x00,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x19, (Code::Instruction)0x87, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x05,
    (Code::Instruction)0x10,
    (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x01,
    (Code::Instruction)0x10,
    (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x05,
    (Code::Instruction)0x10,
    (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x01,
    (Code::Instruction)0x19, (Code::Instruction)0x9D, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x00,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x19, (Code::Instruction)0x87, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x05,
    (Code::Instruction)0x10,
    (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x01,
    (Code::Instruction)0x10,
    (Code::Instruction)0x19, (Code::Instruction)0x90, (Code::Instruction)0x01,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x05,
    (Code::Instruction)0x10,
    (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x01,
    (Code::Instruction)0x19, (Code::Instruction)0x9D, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x19, (Code::Instruction)0x95, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x19, (Code::Instruction)0x95, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x19, (Code::Instruction)0x95, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x01,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x01,
    (Code::Instruction)0x11,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x10,
    (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x10,
    (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x00,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x00,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x19, (Code::Instruction)0x87, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x18, (Code::Instruction)0x06,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x00,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x19, (Code::Instruction)0x87, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x10,
    (Code::Instruction)0x18, (Code::Instruction)0x04,
    (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x00,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x00,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x19, (Code::Instruction)0x87, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x18, (Code::Instruction)0x06,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x00,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x19, (Code::Instruction)0x87, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x18, (Code::Instruction)0x0A,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x10,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x19, (Code::Instruction)0xEC, (Code::Instruction)0x01,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x00,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x00,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x19, (Code::Instruction)0x87, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x19, (Code::Instruction)0x66, (Code::Instruction)0x02,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x04,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x19, (Code::Instruction)0x14, (Code::Instruction)0x02,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x00,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x00,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x19, (Code::Instruction)0x87, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x19, (Code::Instruction)0x66, (Code::Instruction)0x02,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x19, (Code::Instruction)0x3C, (Code::Instruction)0x02,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x00,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x00,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x19, (Code::Instruction)0x87, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x19, (Code::Instruction)0x66, (Code::Instruction)0x02,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x19, (Code::Instruction)0x65, (Code::Instruction)0x02,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x10,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x00,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x00,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x19, (Code::Instruction)0x87, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x19, (Code::Instruction)0x66, (Code::Instruction)0x02,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x18, (Code::Instruction)0x06,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x00,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x19, (Code::Instruction)0x87, (Code::Instruction)0x00,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x1C
};

size_t LinkTest2Length = sizeof(LinkTest2Code) / sizeof(Code::Instruction);

Code::Instruction LoopExitTestCode[] = {
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x05,
    (Code::Instruction)0x19, (Code::Instruction)0x09, (Code::Instruction)0x01,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x06,
    (Code::Instruction)0x18, (Code::Instruction)0x04,
    (Code::Instruction)0x19, (Code::Instruction)0x09, (Code::Instruction)0x01,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x06,
    (Code::Instruction)0x19, (Code::Instruction)0x09, (Code::Instruction)0x01,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x19, (Code::Instruction)0xEC, (Code::Instruction)0x00,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x05,
    (Code::Instruction)0x19, (Code::Instruction)0x09, (Code::Instruction)0x01,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x69,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x10,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x42,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x18, (Code::Instruction)0x5E,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x50,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x18, (Code::Instruction)0x6F,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x10,
    (Code::Instruction)0x18, (Code::Instruction)0x04,
    (Code::Instruction)0x10,
    (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x19, (Code::Instruction)0x09, (Code::Instruction)0x01,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x10,
    (Code::Instruction)0x05,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x05,
    (Code::Instruction)0x19, (Code::Instruction)0x09, (Code::Instruction)0x01,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x0A,
    (Code::Instruction)0x18, (Code::Instruction)0x31,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x10,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x0D,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x19, (Code::Instruction)0x80, (Code::Instruction)0x00,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x19, (Code::Instruction)0xF2, (Code::Instruction)0x00,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x10,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x19, (Code::Instruction)0x93, (Code::Instruction)0x00,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x19, (Code::Instruction)0xD4, (Code::Instruction)0x00,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x10,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x19, (Code::Instruction)0xD3, (Code::Instruction)0x00,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x19, (Code::Instruction)0x09, (Code::Instruction)0x01,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x19, (Code::Instruction)0xC8, (Code::Instruction)0x00,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x10,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x10,
    (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x19, (Code::Instruction)0x09, (Code::Instruction)0x01,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x10,
    (Code::Instruction)0x05,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x19, (Code::Instruction)0x09, (Code::Instruction)0x01,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x0A,
    (Code::Instruction)0x19, (Code::Instruction)0xB0, (Code::Instruction)0x00,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x19, (Code::Instruction)0xE1, (Code::Instruction)0x00,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x10,
    (Code::Instruction)0x18, (Code::Instruction)0x05,
    (Code::Instruction)0x10,
    (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x19, (Code::Instruction)0x09, (Code::Instruction)0x01,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x04,
    (Code::Instruction)0x10,
    (Code::Instruction)0x05,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x06,
    (Code::Instruction)0x19, (Code::Instruction)0x09, (Code::Instruction)0x01,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x0A,
    (Code::Instruction)0x18, (Code::Instruction)0x25,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x20,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x19, (Code::Instruction)0xFA, (Code::Instruction)0x00,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x11,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x06,
    (Code::Instruction)0x19, (Code::Instruction)0xF3, (Code::Instruction)0x00,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x00,
    (Code::Instruction)0x10,
    (Code::Instruction)0x00,
    (Code::Instruction)0x05,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x05,
    (Code::Instruction)0x13,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x06,
    (Code::Instruction)0x15,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x00,
    (Code::Instruction)0x10,
    (Code::Instruction)0x00,
    (Code::Instruction)0x05,
    (Code::Instruction)0x1C
};

size_t LoopExitTestLength = sizeof(LoopExitTestCode) / sizeof(Code::Instruction);

Code::Instruction UsingTestCode[] = {
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x05,
    (Code::Instruction)0x18, (Code::Instruction)0x06,
    (Code::Instruction)0x18, (Code::Instruction)0x64,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x05,
    (Code::Instruction)0x10,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x02,
    (Code::Instruction)0x18, (Code::Instruction)0x05,
    (Code::Instruction)0x18, (Code::Instruction)0x64,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x05,
    (Code::Instruction)0x10,
    (Code::Instruction)0x18, (Code::Instruction)0x73,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x33,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x05,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x50,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x20,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x18, (Code::Instruction)0x78,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x0B,
    (Code::Instruction)0x18, (Code::Instruction)0x4E,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x05,
    (Code::Instruction)0x15,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x50,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x20,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x20,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x56,
    (Code::Instruction)0x1E,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x11,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x06,
    (Code::Instruction)0x18, (Code::Instruction)0x50,
    (Code::Instruction)0x1D,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x00,
    (Code::Instruction)0x10,
    (Code::Instruction)0x00,
    (Code::Instruction)0x05,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x05,
    (Code::Instruction)0x13,
    (Code::Instruction)0x0E,
    (Code::Instruction)0x06,
    (Code::Instruction)0x15,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x0F,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x00,
    (Code::Instruction)0x10,
    (Code::Instruction)0x00,
    (Code::Instruction)0x05,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x84, (Code::Instruction)0x01,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x1C,
    (Code::Instruction)0x87, (Code::Instruction)0x30,
    (Code::Instruction)0x18, (Code::Instruction)0x04,
    (Code::Instruction)0x13,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x64,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x03,
    (Code::Instruction)0x13,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x64,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x02,
    (Code::Instruction)0x13,
    (Code::Instruction)0x18, (Code::Instruction)0x01,
    (Code::Instruction)0x00,
    (Code::Instruction)0x18, (Code::Instruction)0x64,
    (Code::Instruction)0x1B,
    (Code::Instruction)0x18, (Code::Instruction)0x00,
    (Code::Instruction)0x1C
};

size_t UsingTestLength = sizeof(UsingTestCode) / sizeof(Code::Instruction);
}