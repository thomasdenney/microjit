#include "OptionalInstructions.h"

#include "JIT/Interpreter.h" // For execute effect
#include "MicroBit.h"
#include <climits>
#include <cstdint>

extern "C" {
#include "nrf_delay.h"
#include "nrf_gpio.h"
}

// Has to be outside namespace to match the one declared in main.cpp
extern MicroBit uBit;

namespace Device {

static bool s_speakerHasBeenOn = false;

void jitWait(int32_t duration);

/**
 * The implementations of the optional instructions in this file are largely based
 * on Alex Rogers' existing work for his Stack interpreter, however I have
 * worked to optimise it
 */

#define SOUNDER_PIN P0

#define NUMBER_OF_NEOPIXELS 9

void setPinOn(int pin)
{
    NRF_GPIO->OUTSET = 1UL << pin;
}

void setPinOff(int pin)
{
    NRF_GPIO->OUTSET = 1UL << pin;
}

static uint8_t pixels[NUMBER_OF_NEOPIXELS][3] = {
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
};

static uint32_t s_irqState;

void sendStartTimingInterpreterSignal()
{
    nrf_gpio_cfg_output(NEOPIXEL_PIN);
    NRF_GPIO->OUTCLR = (1UL << NEOPIXEL_PIN);

    nrf_delay_us(50);

    s_irqState = __get_PRIMASK();
    __disable_irq();

    neoPixelSendZero();
    neoPixelSendZero();
}

void sendEndTimingInterpreterSignal()
{
    neoPixelSendZero();
    neoPixelSendOne();

    __set_PRIMASK(s_irqState);
}

void sendStartTimingCompilerSignal()
{
    nrf_gpio_cfg_output(NEOPIXEL_PIN);
    NRF_GPIO->OUTCLR = (1UL << NEOPIXEL_PIN);

    nrf_delay_us(50);

    s_irqState = __get_PRIMASK();
    __disable_irq();

    neoPixelSendOne();
    neoPixelSendZero();
}

void sendEndTimingCompilerSignal()
{
    neoPixelSendOne();
    neoPixelSendOne();

    __set_PRIMASK(s_irqState);
}

void updateLeds()
{
    if (ProfilingEnabled) {
        return;
    }
    uint8_t colours[3];
    nrf_gpio_cfg_output(NEOPIXEL_PIN);
    NRF_GPIO->OUTCLR = (1UL << NEOPIXEL_PIN);

    nrf_delay_us(50);

    uint32_t irq_state = __get_PRIMASK();
    __disable_irq();

    for (int i = 0; i < NUMBER_OF_NEOPIXELS; i++) {
        colours[0] = pixels[i][1] >> BrightnessFactor;
        colours[1] = pixels[i][0] >> BrightnessFactor;
        colours[2] = pixels[i][2] >> BrightnessFactor;

        for (int j = 0; j < 3; j++) {
            // I really feel like the compiler could generate this code for me
            if ((colours[j] & 128) > 0) {
                neoPixelSendOne();
            } else {
                neoPixelSendZero();
            }
            if ((colours[j] & 64) > 0) {
                neoPixelSendOne();
            } else {
                neoPixelSendZero();
            }
            if ((colours[j] & 32) > 0) {
                neoPixelSendOne();
            } else {
                neoPixelSendZero();
            }
            if ((colours[j] & 16) > 0) {
                neoPixelSendOne();
            } else {
                neoPixelSendZero();
            }
            if ((colours[j] & 8) > 0) {
                neoPixelSendOne();
            } else {
                neoPixelSendZero();
            }
            if ((colours[j] & 4) > 0) {
                neoPixelSendOne();
            } else {
                neoPixelSendZero();
            }
            if ((colours[j] & 2) > 0) {
                neoPixelSendOne();
            } else {
                neoPixelSendZero();
            }
            if ((colours[j] & 1) > 0) {
                neoPixelSendOne();
            } else {
                neoPixelSendZero();
            }
        }
    }

    __set_PRIMASK(irq_state);
}

void setAllNeoPixels(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < NUMBER_OF_NEOPIXELS; i++) {
        pixels[i][0] = r;
        pixels[i][1] = g;
        pixels[i][2] = b;
    }
    updateLeds();
}

void ledColour(uint8_t colour)
{
    // This could all be done with bit shifts
    uint8_t r = (colour & 0x04) ? 255 : 0;
    uint8_t g = (colour & 0x02) ? 255 : 0;
    uint8_t b = (colour & 0x01) ? 255 : 0;
    setAllNeoPixels(r, g, b);
}

/**
 * Sets a specific pixel (one-indexed)
 */
void setLedToColour(uint8_t pixel, uint8_t colour)
{
    pixels[pixel - 1][0] = (colour & 0x04) ? 255 : 0;
    pixels[pixel - 1][1] = (colour & 0x02) ? 255 : 0;
    pixels[pixel - 1][2] = (colour & 0x01) ? 255 : 0;
    updateLeds();
}

void setLedsOff()
{
    setAllNeoPixels(0, 0, 0);
}

void jitWait(int32_t duration)
{
    // I had issues using uBit.sleep here because that suspends the current fibre and seemingly doesn't restore it
    // in such a way that is compatible with the VM, although I'm not presently sure why...
    wait_ms((int)duration);
}

void sounderOff()
{
    uBit.io.SOUNDER_PIN.setDigitalValue(0);
}

void sounderOn(int frequency)
{
    uBit.io.SOUNDER_PIN.setAnalogValue(512);
    uBit.io.SOUNDER_PIN.setAnalogPeriodUs(1000000 / frequency);
    s_speakerHasBeenOn = true;
}

bool speakerHasBeenOn()
{
    return s_speakerHasBeenOn;
}

void resetState(bool resetSpeaker)
{
    if (resetSpeaker) {
        sounderOff();
        s_speakerHasBeenOn = false;
    }
    setLedsOff();
}

Environment::VM* executeRandom(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    UNDERFLOW_CHECK(1);
    *stackPointer = uBit.random(topOfStack);
    state->m_stack.m_stackPointer = stackPointer;
    return state;
}

Environment::VM* executeWait(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    UNDERFLOW_CHECK(1);
    state->m_stack.m_stackPointer = stackPointer + 1;
    jitWait(topOfStack);
    return state;
}

Environment::VM* executeSleep(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    // TODO Implement
    JIT::executeEffect(state, stackPointer, topOfStack, Code::Instruction::SleepEffect);
    return state;
}

Environment::VM* executeTone(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    UNDERFLOW_CHECK(1);
    int frequency = topOfStack;
    state->m_stack.m_stackPointer = stackPointer + 1;
    if (frequency < 0 || frequency > INT16_MAX) {
        // TODO Error
    } else if (frequency == 0) {
        sounderOff();
    } else {
        sounderOn(frequency);
    }
    return state;
}

Environment::VM* executeBeep(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    UNDERFLOW_CHECK(2);
    int duration = topOfStack;
    state->m_stack.m_stackPointer = stackPointer + 1;
    int frequency = state->m_stack.m_stackPointer[0];
    state->m_stack.m_stackPointer = state->m_stack.m_stackPointer + 1;

    if (frequency < 0 || duration < 0 || frequency > INT16_MAX || duration > INT16_MAX) {
        // TODO Error
    } else if (frequency == 0) {
        sounderOff();
    } else {
        sounderOn(frequency);
        jitWait(duration);
        sounderOff();
    }

    return state;
}

Environment::VM* executeRgb(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    UNDERFLOW_CHECK(3);
    uint8_t b = (uint8_t)topOfStack;
    uint8_t g = (uint8_t)(*(stackPointer + 1));
    uint8_t r = (uint8_t)(*(stackPointer + 2));
    state->m_stack.m_stackPointer = stackPointer + 3; // Pop
    setAllNeoPixels(r, g, b);
    return state;
}

Environment::VM* executeColour(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    UNDERFLOW_CHECK(1);
    state->m_stack.m_stackPointer = stackPointer + 1; // Pop
    ledColour((uint8_t)topOfStack);
    return state;
}

Environment::VM* executeFlash(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    UNDERFLOW_CHECK(2);
    int32_t duration = topOfStack;
    uint8_t colour = (uint8_t)(*(stackPointer + 1));
    state->m_stack.m_stackPointer = stackPointer + 2; // Pop

    ledColour(colour);
    jitWait(duration);
    setLedsOff();

    return state;
}

Environment::VM* executeAccelerometer(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    OVERFLOW_CHECK(3);
    state->m_stack.m_stackPointer = stackPointer - 3; // Push 3
    *(state->m_stack.m_stackPointer + 0) = (int32_t)uBit.accelerometer.getZ();
    *(state->m_stack.m_stackPointer + 1) = (int32_t)uBit.accelerometer.getY();
    *(state->m_stack.m_stackPointer + 2) = (int32_t)uBit.accelerometer.getX();
    return state;
}

Environment::VM* executeTemperature(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    OVERFLOW_CHECK(1);
    state->m_stack.m_stackPointer = stackPointer - 1; // Push
    *(state->m_stack.m_stackPointer) = (int32_t)uBit.thermometer.getTemperature();
    return state;
}

Environment::VM* executePixel(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    UNDERFLOW_CHECK(2);
    uint8_t pixel = (uint8_t)topOfStack;
    uint8_t colour = (uint8_t)(*(stackPointer + 1));
    state->m_stack.m_stackPointer = stackPointer + 2;
    setLedToColour(pixel, colour);
    return state;
}

Environment::VM* executeShowNumber(Environment::VM* state, int32_t* stackPointer, int32_t topOfStack)
{
    UNDERFLOW_CHECK(1);
    state->m_stack.m_stackPointer = stackPointer + 1; // Pop
    uBit.display.scroll(ManagedString((int)topOfStack));
    return state;
}
}