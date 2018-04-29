#include "Tests.h"

#include "Device/MicroBitDevice.h"
#include "JIT/Compiler.h"
#include "Tests/Utilities.h"

namespace JIT {

bool testObservers()
{
    ARM::Functor func;
    Code::Array code(nullptr, 0);
    JIT::Compiler compiler(code, &Device::MicroBitDevice::singleton());

    using Status = JIT::Compiler::Status;

    int observerFireCount = 0;
    auto observerId = compiler.addObserver([&](ARM::Functor& func, Status status) {
        if (status == Status::Success) {
            observerFireCount++;
        }
    });

    compiler.compile(func);

    compiler.removeObserver(observerId);

    bool success = observerFireCount == 1;
    func = ARM::Functor();

    compiler.compile(func);
    success &= observerFireCount == 1;

    return success;
}

bool testCompiler()
{
    printTestHeader("COMPILER INFRASTRUCTURE TESTS");
    bool success = true;

    success &= TEST(testObservers);

    return success;
}
}