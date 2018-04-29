#pragma once

#include "Config.h"

#define PRINT_FUNCTION_NAME() printf("%s:%s:%d\n", __FILE__ + strlen("/Users/thomas/Oxford/Year 3/Project/jit/source/"), __FUNCTION__, __LINE__)

void printConfiguration();

typedef bool (*TestFunction)();

void printLine();
void printTestHeader(const char* name);

void incFailure();
void incSuccess();

void printEstimateOfFreeMemory();

bool printResult(bool result, const char* name);

#define TEST(test) printResult(test(), STR_NAME(test))

void printTestStats();
