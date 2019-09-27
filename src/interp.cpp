#include "interp.hpp"

#include <iostream>

namespace moonflower {

constexpr int OFF_RET_ADDR = 0;
constexpr int OFF_RET_STACK = 4;
constexpr int OFF_RET_INC = 8;

template <typename T>
auto stack_cast(std::byte* stack, std::int16_t addr) -> T& {
    return *reinterpret_cast<T*>(stack + addr);
}

int interp(state& S, std::uint16_t mod_idx, std::uint16_t func_addr, int retc) {
    const instruction* text = S.modules[mod_idx].text.data();
    const instruction* PC = text + func_addr;
    std::byte* stack = S.stack.get() + retc;
    instruction I;

    stack_cast<global_addr>(stack, OFF_RET_ADDR) = {0, 0};
    stack_cast<stack_rep>(stack, OFF_RET_STACK) = {0};
    stack += OFF_RET_INC;

    const auto fetch = [&I, &PC]{ I = *PC; ++PC; };

    while (true) {
        fetch();

        switch (I.OP) {
            case TERMINATE:
                return I.A;
            
            // constant loads
            case ISETC:
                stack_cast<int>(stack, I.A) = I.DI;
                break;
            case FSETC:
                stack_cast<float>(stack, I.A) = I.DF;
                break;
            
            // address load
            case SETADR:
                stack_cast<global_addr>(stack, I.A) = {mod_idx, std::uint16_t(I.DI)};
                break;
            
            // copy
            case CPY:
                for (int i = 0; i < I.BC.C; ++i) {
                    stack[I.A + i] = stack[I.BC.B + i];
                }
                break;

            // integer ops
            case IADD:
                stack_cast<int>(stack, I.A) = stack_cast<int>(stack, I.BC.B) + stack_cast<int>(stack, I.BC.C);
                break;
            case ISUB:
                stack_cast<int>(stack, I.A) = stack_cast<int>(stack, I.BC.B) - stack_cast<int>(stack, I.BC.C);
                break;
            case IMUL:
                stack_cast<int>(stack, I.A) = stack_cast<int>(stack, I.BC.B) * stack_cast<int>(stack, I.BC.C);
                break;
            case IDIV:
                stack_cast<int>(stack, I.A) = stack_cast<int>(stack, I.BC.B) / stack_cast<int>(stack, I.BC.C);
                break;

            // float ops
            case FADD:
                stack_cast<float>(stack, I.A) = stack_cast<float>(stack, I.BC.B) + stack_cast<float>(stack, I.BC.C);
                break;
            case FSUB:
                stack_cast<float>(stack, I.A) = stack_cast<float>(stack, I.BC.B) - stack_cast<float>(stack, I.BC.C);
                break;
            case FMUL:
                stack_cast<float>(stack, I.A) = stack_cast<float>(stack, I.BC.B) * stack_cast<float>(stack, I.BC.C);
                break;
            case FDIV:
                stack_cast<float>(stack, I.A) = stack_cast<float>(stack, I.BC.B) / stack_cast<float>(stack, I.BC.C);
                break;
            
            // control ops
            case JMP:
                PC = text + I.DI;
                break;
            case CALL: {
                stack_cast<global_addr>(stack, I.A + OFF_RET_ADDR) = {mod_idx, std::uint16_t(PC - text)};
                stack_cast<stack_rep>(stack, I.A + OFF_RET_STACK).soff = I.A;
                const auto& addr = stack_cast<global_addr>(stack, I.BC.B);
                mod_idx = addr.mod;
                text = S.modules[mod_idx].text.data();
                PC = text + addr.off;
                stack += I.A + OFF_RET_INC;
                break;
            }
            case RET: {
                const auto& addr = stack_cast<global_addr>(stack, OFF_RET_ADDR - OFF_RET_INC);
                mod_idx = addr.mod;
                text = S.modules[mod_idx].text.data();
                PC = text + addr.off;
                stack -= stack_cast<stack_rep>(stack, OFF_RET_STACK - OFF_RET_INC).soff + OFF_RET_INC;
                break;
            }

            // invalid ops
            default:
                return -1;
        }
    }
}

}
