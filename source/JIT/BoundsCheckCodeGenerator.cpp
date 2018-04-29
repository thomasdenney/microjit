#include "BoundsCheckCodeGenerator.h"

#include "Compiler.h"

namespace JIT {

void BoundsCheckCodeGenerator::compile(Code::BlockStackEffect effect)
{
    if (StackCheckMode == StackCheck::None) {
        return;
    }

    auto popCount = effect.popCount();
    auto pushCount = effect.pushCount();
    if (popCount == 0 && pushCount == 0) {
        return;
    }

    m_func->add(ARM::moveGeneral(TempRegister3, ARM::Register::pc));

    auto canAdjustStackPointerThroughArithmetic = popCount * 4 + pushCount * 4 < 256;
    int offset = 0;

    if (popCount != 0) {
        auto reg = TempRegister;
        if (canAdjustStackPointerThroughArithmetic) {
            reg = StackPointerRegister;
            m_func->add(ARM::addLargeImm(StackPointerRegister, popCount * 4));
            offset += popCount * 4;
        } else {
            if (popCount * 4 < 8) {
                m_func->add(ARM::addSmallImm(TempRegister, StackPointerRegister, popCount * 4));
            } else {
                compileLoadConstant(*m_func, popCount * 4, TempRegister);
                m_func->add(ARM::addReg(TempRegister, StackPointerRegister, TempRegister));
            }
        }
        m_func->add(ARM::compareRegistersGeneral(reg, StackEndRegister));
        m_linker->addStackUnderflowCheck(*m_func);
    }

    if (pushCount != 0) {
        auto reg = TempRegister;
        if (canAdjustStackPointerThroughArithmetic) {
            reg = StackPointerRegister;
            m_func->add(ARM::subLargeImm(StackPointerRegister, (popCount + pushCount) * 4));
            offset -= (popCount + pushCount) * 4;
        } else {
            if (pushCount * 4 < 8) {
                m_func->add(ARM::subSmallImm(TempRegister, StackPointerRegister, pushCount * 4));
            } else {
                compileLoadConstant(*m_func, popCount * 4, TempRegister);
                m_func->add(ARM::subReg(TempRegister, StackPointerRegister, TempRegister));
            }
        }
        m_func->add(ARM::compareRegistersGeneral(reg, StackBaseRegister));
        m_linker->addStackOverflowCheck(*m_func);
    }

    if (offset > 0) {
        m_func->add(ARM::subLargeImm(StackPointerRegister, offset));
    } else if (offset < 0) {
        m_func->add(ARM::addLargeImm(StackPointerRegister, -offset));
    }
}

void BoundsCheckCodeGenerator::compile(Code::BlockStackEffect effect, ARM::Functor& func, Linker& linker)
{
    BoundsCheckCodeGenerator generator(&func, &linker);
    generator.compile(effect);
}

size_t BoundsCheckCodeGenerator::numberOfInstructions(Code::BlockStackEffect effect)
{
    ARM::Functor temporaryFunctor;
    Linker temporaryLinker;
    BoundsCheckCodeGenerator generator(&temporaryFunctor, &temporaryLinker);
    generator.compile(effect);
    return temporaryFunctor.length();
}
}