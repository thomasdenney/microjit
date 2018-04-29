#pragma once

#include "Config.h"

#include "Environment/VM.h"

namespace Device {

#define NEOPIXEL_PIN 2

inline void neoPixelSendOne()
{
    NRF_GPIO->OUTSET = (1UL << NEOPIXEL_PIN);
    // 9 NOPs
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    NRF_GPIO->OUTCLR = (1UL << NEOPIXEL_PIN);
}

inline void neoPixelSendZero()
{
    NRF_GPIO->OUTSET = (1UL << NEOPIXEL_PIN);
    asm("nop");
    NRF_GPIO->OUTCLR = (1UL << NEOPIXEL_PIN);
    // 11 NOPs
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
}

bool speakerHasBeenOn();

/**
 * Important that you use this with |sendEndTimingInterpreterSignal| because otherwise interrupt flags won't be
 * correctly restored
 */
void sendStartTimingInterpreterSignal();
void sendEndTimingInterpreterSignal();

/**
 * Important that you use this with |sendEndTimingCompilerSignal|
 */
void sendStartTimingCompilerSignal();
void sendEndTimingCompilerSignal();

/**
 * Ensures that no tone is being played over the speaker and no LEDs are on
 * If this method is not called on start-up it cannot be guaranteed that the
 * LEDs or speaker will work correctly
 * 
 * Resetting the speaker is optional (but recommended)
 */
void resetState(bool resetSpeaker);

Environment::VM* executeRandom(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeWait(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeSleep(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeTone(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeBeep(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeRgb(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeColour(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeFlash(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeAccelerometer(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeTemperature(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executePixel(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);

Environment::VM* executeShowNumber(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack);
}