#include "Tests.h"

#include "Bit/Bit.h"
#include "Tests/Utilities.h"

namespace Bit {

bool testPositiveConversions()
{
    return twosComplement(0, 32) == 0 && twosComplement(0, 16) == 0 && twosComplement(0, 16) == 0 && twosComplement(0, 8) == 0 && twosComplement(1, 32) == 1 && twosComplement(1, 16) == 1 && twosComplement(1, 11) == 1 && twosComplement(1, 8) == 1;
}

bool testNoBitNumberConversion()
{
    uint32_t res = twosComplement(-1, 32);
    return res == 0xFFFFFFFF;
}

bool testHalfBitsNumberConversion()
{
    uint32_t res = twosComplement(-1, 16);
    return res == (uint32_t)0x0000FFFF;
}

bool test11BitNumberConversion()
{
    // This is fairly common in branching instructions
    uint32_t res = twosComplement(-1, 11);
    return res == 0b11111111111;
}

bool testQuarterBitsNumberConversion()
{
    uint32_t res = twosComplement(-1, 8);
    return res == (uint32_t)0x000000FF;
}

bool unTwosComplementTest(int value, int bits)
{
    return unTwosComplement(twosComplement(value, bits), bits) == value;
}

bool unTwosComplementTestZero()
{
    return unTwosComplementTest(0, 22);
}

bool unTwosComplementTestNegativeOne8Bits()
{
    return unTwosComplementTest(-1, 8);
}

bool unTwosComplementTestNegativeOne11Bits()
{
    return unTwosComplementTest(-1, 11);
}

bool unTwosComplementTestNegative42()
{
    return unTwosComplementTest(-42, 11);
}

bool unTwosComplementTestPositive()
{
    return unTwosComplementTest(42, 11);
}

bool bitTests()
{
    bool success = true;

    printTestHeader("BIT FIDDLING TESTS");
    success &= TEST(testNoBitNumberConversion);
    success &= TEST(testHalfBitsNumberConversion);
    success &= TEST(test11BitNumberConversion);
    success &= TEST(testQuarterBitsNumberConversion);
    success &= TEST(testPositiveConversions);
    success &= TEST(unTwosComplementTestZero);
    success &= TEST(unTwosComplementTestNegativeOne8Bits);
    success &= TEST(unTwosComplementTestNegativeOne11Bits);
    success &= TEST(unTwosComplementTestNegative42);
    success &= TEST(unTwosComplementTestNegative42);

    return success;
}
}