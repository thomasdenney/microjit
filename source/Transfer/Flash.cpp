#include "Flash.h"

#include "Deserialiser.h"
#include "MicroBit.h"
#include "Serialiser.h"

namespace Transfer {

static const char DefaultFilename[] = "stack.hex";

MicroBitFileSystem* microBitFileSystem()
{
    // Adapted from MicroBitFilSystem::init
    // By default it will compute the start of the file system at + PAGE_SIZE * 1
    // However, this didn't work reliably for me so I instead offset it by another page
    // I suspect that the reason for this problem is that at some point it tries to overwrite
    // the page immediately before flashStart, which in the original case would include code
    if (MicroBitFileSystem::defaultFileSystem == nullptr) {
        uint32_t flashStart = FLASH_PROGRAM_END;
        if (flashStart % PAGE_SIZE != 0) {
            flashStart = ((uint32_t)flashStart & ~(PAGE_SIZE - 1)) + PAGE_SIZE * 3;
        }
        const int flashPages = 0;

        return new MicroBitFileSystem(flashStart, flashPages);
    } else {
        return MicroBitFileSystem::defaultFileSystem;
    }
}

bool writeStackCodeToFlash(Code::Instruction* code, size_t length)
{
    Serialiser serialiser(DefaultFilename);
    serialiser.appendData((uint8_t*)code, length);
    return true;
}

Code::Instruction* readCodeFromFlash(Code::Instruction* preferredBuffer, size_t preferredBufferSize, size_t* outSize)
{
    Deserialiser deserialiser(DefaultFilename);
    if (deserialiser.exists() && deserialiser.length() > 0 && deserialiser.length() <= preferredBufferSize) {
        *outSize = deserialiser.length();
        deserialiser.readData((uint8_t*)preferredBuffer, deserialiser.length());
        return preferredBuffer;
    }
    return nullptr;
}
}