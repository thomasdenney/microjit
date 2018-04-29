#include "MicroBitDevice.h"

#include "OptionalInstructions.h"

namespace Device {

MicroBitDevice::MicroBitDevice()
{
    resetState(false);
}

void MicroBitDevice::programHalted() const
{
    resetState(speakerHasBeenOn());
}

Environment::VMFunction MicroBitDevice::resolveVirtualMachineFunction(Code::Instruction instr) const
{
    switch (instr) {
    case Code::Instruction::Nrnd:
        return &executeRandom;
    case Code::Instruction::Wait:
        return &executeWait;
    case Code::Instruction::Sleep:
        return &executeSleep;
    case Code::Instruction::Tone:
        return &executeTone;
    case Code::Instruction::Beep:
        return &executeBeep;
    case Code::Instruction::Rgb:
        return &executeRgb;
    case Code::Instruction::Colour:
        return &executeColour;
    case Code::Instruction::Flash:
        return &executeFlash;
    case Code::Instruction::Temp:
        return &executeTemperature;
    case Code::Instruction::Accel:
        return &executeAccelerometer;
    case Code::Instruction::Pixel:
        return &executePixel;
    // Thomas' custom instructions
    case Code::Instruction::ShowNumber:
        return &executeShowNumber;
    default:
        return nullptr;
    }
}
}