#pragma once

#include "Config.h"

#include "Environment/Device.h"

namespace Device {

class MicroBitDevice : public Environment::Device {
public:
    Environment::VMFunction resolveVirtualMachineFunction(Code::Instruction instruction) const final;
    void programHalted() const final;

    static MicroBitDevice& singleton()
    {
        static MicroBitDevice device;
        return device;
    }

    MicroBitDevice(MicroBitDevice const&) = delete;
    void operator=(MicroBitDevice const&) = delete;

private:
    MicroBitDevice();
};
}