#include "Config.h"

#include "Code/Instruction.h"
#include "MicroBitFileSystem.h"

namespace Transfer {

MicroBitFileSystem* microBitFileSystem();

bool writeStackCodeToFlash(Code::Instruction* code, size_t length);

/**
 * The buffer provided *might* be  used for the program code --- another buffer
 * may be used if the provided buffer is too small. If the preferred buffer is not used then you are
 * response for freeing the memory allocated by this function
 *
 * If reading succeeded a pointer to the data will be returned, if not it will be the null pointer
 *
 * In the event that the source program exists but is of 0 length the null pointer will be returned
 */
Code::Instruction* readCodeFromFlash(Code::Instruction* preferredBuffer, size_t preferredBufferSize, size_t* outSize);
}