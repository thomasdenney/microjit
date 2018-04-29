#include "Tests.h"

#include "Device/MicroBitDevice.h"
#include "Device/OptionalInstructions.h"
#include "JIT/Compiler.h"
#include "JIT/Interpreter.h"
#include "Tests/Utilities.h"

namespace Device {

void compileAndRun(const Code::Instruction* instructions, size_t length)
{
    int32_t stackStorage[128];
    Environment::Stack stack(stackStorage, 128);
    Code::Array code(instructions, length);
    code.print();
    Environment::VM state(stack, code);

    if (CompileOptionalInstructionTests) {
        ARM::Functor func;
        JIT::Compiler compiler(state.m_code, &Device::MicroBitDevice::singleton());
        auto result = compiler.compile(func);
        if (result == JIT::Compiler::Status::Success) {
            compiler.prettyPrintCode(func);
            state.call(func);
            // Ensure sounder and LEDs are off after execution
            Device::MicroBitDevice::singleton().programHalted();
            printLine();
            state.m_stack.print();
        } else {
            printf("Compilation error: %s\n", JIT::Compiler::statusString(result));
        }
    } else {
        JIT::execute(&state);
    }
}

void testBeep()
{
    static const Code::Instruction instructions[] = {
        Code::Instruction::Push16, (Code::Instruction)0xEE, (Code::Instruction)0x01,
        Code::Instruction::Push16, (Code::Instruction)0xE8, (Code::Instruction)0x03,
        Code::Instruction::Beep, Code::Instruction::BeepEffect,
        Code::Instruction::Halt
    };
    compileAndRun(instructions, sizeof(instructions) / sizeof(Code::Instruction));
}

void testTone()
{
    static const Code::Instruction instructions[] = {
        Code::Instruction::Push16, (Code::Instruction)0xE8, (Code::Instruction)0x03,
        Code::Instruction::Tone, Code::Instruction::ToneEffect,
        Code::Instruction::Push16, (Code::Instruction)0xEE, (Code::Instruction)0x01,
        Code::Instruction::Wait,
        Code::Instruction::Push8, (Code::Instruction)0x00,
        Code::Instruction::Tone, Code::Instruction::ToneEffect,
        Code::Instruction::Halt
    };
    compileAndRun(instructions, sizeof(instructions) / sizeof(Code::Instruction));
}

void testRgb()
{
    // Should be #FFCC00
    static const Code::Instruction instructions[] = {
        Code::Instruction::Push16, (Code::Instruction)0xFF, (Code::Instruction)0x00,
        Code::Instruction::Push16, (Code::Instruction)0xCC, (Code::Instruction)0x00,
        Code::Instruction::Push8, (Code::Instruction)0x00,
        Code::Instruction::Rgb, Code::Instruction::RgbEffect,
        Code::Instruction::Push16, (Code::Instruction)0xD0, (Code::Instruction)0x07,
        Code::Instruction::Wait,
        Code::Instruction::Halt
    };
    compileAndRun(instructions, sizeof(instructions) / sizeof(Code::Instruction));
}

void testColour()
{
    /*
    0x00 Off
    Ox01 Blue
    0x02 Green
    0x03 Cyan
    0x04 Red
    0x05 Magenta
    0x06 Yellow
    0x07 White
    */
    static const Code::Instruction instructions[] = {
        Code::Instruction::Push8, (Code::Instruction)0x07,
        Code::Instruction::Colour, Code::Instruction::ColourEffect,
        Code::Instruction::Push16, (Code::Instruction)0xD0, (Code::Instruction)0x07,
        Code::Instruction::Wait,
        Code::Instruction::Halt
    };
    compileAndRun(instructions, sizeof(instructions) / sizeof(Code::Instruction));
}

void testFlash()
{
    static const Code::Instruction instructions[] = {
        Code::Instruction::Push8, (Code::Instruction)0x04,
        Code::Instruction::Push16, (Code::Instruction)0xEE, (Code::Instruction)0x03,
        Code::Instruction::Flash, Code::Instruction::FlashEffect,
        Code::Instruction::Halt
    };
    compileAndRun(instructions, sizeof(instructions) / sizeof(Code::Instruction));
}

void testPixel()
{
    static const Code::Instruction instructions[] = {
        Code::Instruction::Push8,
        (Code::Instruction)0x00,
        Code::Instruction::Colour,
        Code::Instruction::ColourEffect,
        Code::Instruction::Push8,
        (Code::Instruction)0x7F,
        Code::Instruction::Wait,

        Code::Instruction::Push8,
        (Code::Instruction)0x00,
        Code::Instruction::Dup,
        Code::Instruction::Inc,
        Code::Instruction::Pixel,
        Code::Instruction::PixelEffect,
        Code::Instruction::Push8,
        (Code::Instruction)0x7F,
        Code::Instruction::Wait,

        Code::Instruction::Push8,
        (Code::Instruction)0x01,
        Code::Instruction::Dup,
        Code::Instruction::Inc,
        Code::Instruction::Pixel,
        Code::Instruction::PixelEffect,
        Code::Instruction::Push8,
        (Code::Instruction)0x7F,
        Code::Instruction::Wait,

        Code::Instruction::Push8,
        (Code::Instruction)0x02,
        Code::Instruction::Dup,
        Code::Instruction::Inc,
        Code::Instruction::Pixel,
        Code::Instruction::PixelEffect,
        Code::Instruction::Push8,
        (Code::Instruction)0x7F,
        Code::Instruction::Wait,

        Code::Instruction::Push8,
        (Code::Instruction)0x03,
        Code::Instruction::Dup,
        Code::Instruction::Inc,
        Code::Instruction::Pixel,
        Code::Instruction::PixelEffect,
        Code::Instruction::Push8,
        (Code::Instruction)0x7F,
        Code::Instruction::Wait,

        Code::Instruction::Push8,
        (Code::Instruction)0x04,
        Code::Instruction::Dup,
        Code::Instruction::Inc,
        Code::Instruction::Pixel,
        Code::Instruction::PixelEffect,
        Code::Instruction::Push8,
        (Code::Instruction)0x7F,
        Code::Instruction::Wait,

        Code::Instruction::Push8,
        (Code::Instruction)0x05,
        Code::Instruction::Dup,
        Code::Instruction::Inc,
        Code::Instruction::Pixel,
        Code::Instruction::PixelEffect,
        Code::Instruction::Push8,
        (Code::Instruction)0x7F,
        Code::Instruction::Wait,

        Code::Instruction::Push8,
        (Code::Instruction)0x06,
        Code::Instruction::Dup,
        Code::Instruction::Inc,
        Code::Instruction::Pixel,
        Code::Instruction::PixelEffect,
        Code::Instruction::Push8,
        (Code::Instruction)0x7F,
        Code::Instruction::Wait,

        Code::Instruction::Push8,
        (Code::Instruction)0x07,
        Code::Instruction::Dup,
        Code::Instruction::Inc,
        Code::Instruction::Pixel,
        Code::Instruction::PixelEffect,
        Code::Instruction::Push8,
        (Code::Instruction)0x7F,
        Code::Instruction::Wait,

        Code::Instruction::Push8,
        (Code::Instruction)0x08,
        Code::Instruction::Dup,
        Code::Instruction::Inc,
        Code::Instruction::Pixel,
        Code::Instruction::PixelEffect,

        Code::Instruction::Push16,
        (Code::Instruction)0x10,
        (Code::Instruction)0x27,
        Code::Instruction::Wait,
        Code::Instruction::Halt,
    };
    compileAndRun(instructions, sizeof(instructions) / sizeof(Code::Instruction));
}

void testAccelerometer()
{
    static const Code::Instruction instructions[] = {
        Code::Instruction::Accel,
        Code::Instruction::AccelEffect,
        Code::Instruction::ShowNumber,
        Code::Instruction::ShowNumberEffect,
        Code::Instruction::ShowNumber,
        Code::Instruction::ShowNumberEffect,
        Code::Instruction::ShowNumber,
        Code::Instruction::ShowNumberEffect,
    };
    compileAndRun(instructions, sizeof(instructions) / sizeof(Code::Instruction));
}

void testThermometer()
{
    static const Code::Instruction instructions[] = {
        Code::Instruction::Temp, Code::Instruction::TempEffect,
        Code::Instruction::ShowNumber, Code::Instruction::ShowNumberEffect
    };
    compileAndRun(instructions, sizeof(instructions) / sizeof(Code::Instruction));
}

void testMusic()
{
    static const uint8_t instructions[] = {
        0x18, 0x21,
        0x18, 0x06,
        0x18, 0x01,
        0x0F,
        0x12,
        0x00,
        0x18, 0x07,
        0x04,
        0x0F,
        0x05,
        0x84, 0x01,
        0x0F,
        0x18, 0x21,
        0x1B,
        0x12,
        0x06,
        0x0F,
        0x18, 0x04,
        0x15,
        0x18, 0x00,
        0x0D,
        0x18, 0x06,
        0x1E,
        0x20,
        0x18, 0x02,
        0x02,
        0x18, 0x31,
        0x00,
        0x1A,
        0x19, 0xC8, 0x00,
        0x82, 0x02,
        0x18, 0x32,
        0x1F,
        0x1C,
        0xEE, 0x01,
        0x0B, 0x02,
        0x4B, 0x02,
        0x93, 0x02,
        0xBB, 0x02,
        0xE4, 0x02,
        0x10, 0x03
    };
    compileAndRun((const Code::Instruction*)instructions, sizeof(instructions) / sizeof(Code::Instruction));
}
}