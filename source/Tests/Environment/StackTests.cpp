#include "Tests.h"

#include "Environment/Stack.h"
#include "Tests/Utilities.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>

// Each test in this file returns the stack to its original state for other
// tests

namespace Environment {

static int32_t stackStorage[4];
static Stack stack(stackStorage, 4);

bool testSize()
{
    bool success = stack.size() == 0;
    stack.push(1);
    success &= stack.size() == 1;
    stack.pop();
    success &= stack.size() == 0;
    return success;
}

bool testClear()
{
    stack.push(1);
    stack.push(2);
    stack.push(3);
    stack.clear();
    return stack.size() == 0;
}

bool testPush()
{
    stack.push(1);
    stack.push(2);
    bool success = stack.pop() == 2;
    stack.clear();
    return success;
}

bool testRotate()
{
    stack.push(1);
    stack.push(2);
    stack.push(3);
    stack.rotate(3);

    bool success = stack.size() == 3 && stack.peek(0) == 1 && stack.peek(1) == 3 && stack.peek(2) == 2;

    stack.clear();

    return success;
}

bool testTuck()
{
    stack.push(1);
    stack.push(2);
    stack.push(3);
    stack.tuck(3);

    bool success = stack.size() == 3 && stack.peek(0) == 2 && stack.peek(1) == 1 && stack.peek(2) == 3;

    if (!success) {
        stack.print();
    }

    stack.clear();

    return success;
}

bool testDuplicate()
{
    stack.push(1);
    stack.duplicate(0);
    bool success = stack.size() == 2 && stack.peek() == 1;
    stack.clear();
    return success;
}

bool testStack()
{
    printTestHeader("STACK (STRUCTURE) TESTS");
    bool success = true;
    success &= TEST(testSize);
    success &= TEST(testClear);
    success &= TEST(testPush);
    success &= TEST(testRotate);
    success &= TEST(testTuck);
    success &= TEST(testDuplicate);
    return success;
}
}