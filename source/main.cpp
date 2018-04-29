#include "ARM/Decoder.h"
#include "Config.h"
#include "Device/MicroBitDevice.h"
#include "Device/OptionalInstructions.h"
#include "JIT/Compiler.h"
#include "JIT/Interpreter.h"
#include "MicroBit.h"
#include "MicroBitFileSystem.h"
#include "Tests/TestRunner.h"
#include "Transfer/Flash.h"
#include "Transfer/Serial.h"
#include <cstdint>

MicroBit uBit;

static int32_t stackStorage[128];

void readAndExecuteProgram()
{
    using Status = JIT::Compiler::Status;

    Transfer::Serial::readProgram();

    Transfer::microBitFileSystem()->remove("stack.hex");
    Transfer::microBitFileSystem()->remove("bytecode");
    Transfer::microBitFileSystem()->remove("linker");
    Transfer::microBitFileSystem()->remove("sa");

    Transfer::writeStackCodeToFlash((Code::Instruction*)Transfer::Serial::programBuffer(), Transfer::Serial::programLength());

    Environment::Stack stack(stackStorage, 128);

    Code::Array code((const Code::Instruction*)Transfer::Serial::programBuffer(), Transfer::Serial::programLength());
    Environment::VM state(stack, code);
    ARM::Functor func;
    JIT::Compiler compiler(state.m_code, &Device::MicroBitDevice::singleton());
    state.m_compiler = &compiler;

    compiler.addObserver([&](ARM::Functor& func, Status status) {
        if (status == Status::Success && WriteCompiledCodeToFlash) {
            func.serialise();
            compiler.serialise();
        }
    });

    auto result = compiler.compile(func);

    if (result == JIT::Compiler::Status::Success) {
        state.call(func);
        // Ensure sounder and LEDs are off after execution
        state.m_stack.print();
    } else {
        printf("Compilation error: %s\n", JIT::Compiler::statusString(result));
    }

    Device::MicroBitDevice::singleton().programHalted();
}

void tryRunFromFlash()
{
    using Status = JIT::Compiler::Status;

    size_t length;
    Code::Instruction* buffer = Transfer::readCodeFromFlash((Code::Instruction*)Transfer::Serial::programBuffer(), MAX_SERIAL_BUFFER_SIZE, &length);
    if (!buffer) {
        return;
    }

    Environment::Stack stack(stackStorage, 128);

    Code::Array code((const Code::Instruction*)buffer, length);
    Environment::VM state(stack, code);
    JIT::Compiler compiler(state.m_code, &Device::MicroBitDevice::singleton());
    state.m_compiler = &compiler;

    ARM::Functor func;

    compiler.addObserver([&](ARM::Functor& func, Status status) {
        if (status == Status::Success && WriteCompiledCodeToFlash) {
            Transfer::microBitFileSystem()->remove("");
            func.serialise();
            compiler.serialise();
        }
    });

    if (WriteCompiledCodeToFlash) {
        func.deserialise();
        compiler.deserialise();
        state.call(func);
    } else {
        auto res = compiler.compile(func);
        if (res == Status::Success) {
            state.call(func);
        }
    }
    Device::MicroBitDevice::singleton().programHalted();
}

int main()
{
    uBit.init();
    JIT::setGlobalInterpreterDevice(&Device::MicroBitDevice::singleton());

    switch (Mode) {
    case ProjectMode::UnitTests:
        unitTests();
        break;
    case ProjectMode::OptionalInstructionTests:
        optionalInstructionTests();
        break;
    case ProjectMode::SerialDeploy:
        tryRunFromFlash();
        while (true) {
            readAndExecuteProgram();
        }
        break;
    }
    release_fiber();
    return 0;
}
