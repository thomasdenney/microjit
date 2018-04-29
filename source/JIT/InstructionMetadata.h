#pragma once

#include "Config.h"

#include <cstdint>

namespace JIT {

/**
 * Illegal => everything else is false
 * Data => !Code
 * Code => !Data
 */
enum class InstructionMetadata : uint8_t {
    /// Code properties

    /**
     * Denotes that we haven't examined this instruction during static analysis
     */
    Nothing = 0,
    /**
     * Means that we cannot execute it (but they could be treated as data)
     * Used to indicate the one or two instructions following a push or optional instruction
     */
    Illegal = 1 << 1,

    /**
     * Static analysis should find this to be true for every exectuable instruction
     * 
     * Code => !Illegal
     */
    Code = 1 << 2,

    /**
     * Using the definition from Modern Compiler Implementation in ML.
     * 
     * A basic block is a sequence of statements that is always entered at the beginning
     * and exited at the end, i.e. the first statement is a label (jump destination),
     * and the last statement is a jump or conditional jump. This can jump can be implicit
     * 
     * BasicBlockStart => Code
     */
    BasicBlockStart = 1 << 3,

    /**
     * Iteration properties that allow for reverse iteration. These *must* be ignored if
     * BasicBlockStart holds
     * 
     * LastInstructionDoubleWidth => Code & !LastInstructionTripleWidth
     * 
     * LastInstructionTripleWidth => Code & !LastInstructionDoubleWidth
     */
    LastInstructionDoubleWidth = 1 << 4,
    LastInstructionTripleWidth = 1 << 5,

    /// Function properties

    /**
     * FunctionStart => BasicBlockStart
     */
    FunctionStart = 1 << 6,

    /**
     * The function does not perform recursion (although tail call recursion is allowed) and 
     * therefore doesn't need to push or pop the return address
     * 
     * NoRecursion => FunctionStart
     */
    NoRecursion = 1 << 7,
};

inline constexpr InstructionMetadata operator|(InstructionMetadata x, InstructionMetadata y)
{
    return static_cast<InstructionMetadata>(static_cast<uint32_t>(x) | static_cast<uint32_t>(y));
}

inline constexpr InstructionMetadata operator&(InstructionMetadata x, InstructionMetadata y)
{
    return static_cast<InstructionMetadata>(static_cast<uint32_t>(x) & static_cast<uint32_t>(y));
}

/**
 * Returns the violated property, or Nothing if none are violated
 */
InstructionMetadata violatedProperty(InstructionMetadata meta);

/// This is so that InstructionMetadata can be used in if statements
inline constexpr bool any(InstructionMetadata x)
{
    return static_cast<bool>(x);
}
}