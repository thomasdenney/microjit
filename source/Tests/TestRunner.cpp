#include "TestRunner.h"

#include "Device/MicroBitDevice.h"
#include "MicroBit.h"
#include "Tests/ARM/Tests.h"
#include "Tests/Bit/Tests.h"
#include "Tests/Device/Tests.h"
#include "Tests/Environment/Tests.h"
#include "Tests/JIT/Tests.h"
#include "Tests/Utilities.h"

extern MicroBit uBit;

void unitTests()
{
    printTestHeader("MICRO JIT TEST SUITE");

    printConfiguration();

    bool success = true;
    if (!ProfilingEnabled) {
        success &= ARM::testDecoder();
        success &= ARM::testEncoder();
        success &= Bit::bitTests();
        success &= Environment::testStack();
        success &= JIT::testCompiler();
        success &= JIT::testStaticAnalysis();
    }
    success &= JIT::testCodeExecution();

    printTestStats();

    if (success) {
        MicroBitImage smiley("0,255,0,255,0\n"
                             "0  ,255,0  ,255,0  \n"
                             "0  ,0  ,0  ,0  ,0  \n"
                             "255,0  ,0  ,0  ,255\n"
                             "0  ,255,255,255,0  \n");
        uBit.display.print(smiley);
    } else {
        MicroBitImage frown("0,255,0,255,0\n"
                            "0  ,255,0  ,255,0  \n"
                            "0  ,0  ,0  ,0  ,0  \n"
                            "0  ,255,255,255,0  \n"
                            "255,0  ,0  ,0  ,255\n");
        uBit.display.print(frown);
    }

    Device::MicroBitDevice::singleton().programHalted();
}

void optionalInstructionTests()
{
    Device::testBeep();
    Device::testTone();
    Device::testRgb();
    Device::testColour();
    Device::testFlash();
    Device::testPixel();
    Device::testThermometer();
    Device::testAccelerometer();
    Device::testMusic();

    Device::MicroBitDevice::singleton().programHalted();
}
