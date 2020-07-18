#include "interp.hpp"

#include <algorithm>
#include <iostream>

namespace moonflower {

constexpr int OFF_RET_ADDR = 0;
constexpr int OFF_RET_STACK = 4;
constexpr int OFF_RET_INC = 8;

template <typename T>
auto stack_cast(std::byte* stack, std::int16_t addr) -> T& {
    return *reinterpret_cast<T*>(stack + addr);
}

interp_result interp(state& S, std::uint16_t mod_idx, std::uint16_t func_addr, int retc) {
    const instruction* text = S.modules[mod_idx].text.data();
    std::byte* data = S.modules[mod_idx].data.data();
    const instruction* PC = text + func_addr;
    std::byte* stack = S.stack.get() + retc;
    instruction I;

    stack_cast<program_addr>(stack, OFF_RET_ADDR) = {0, 0};
    stack_cast<stack_rep>(stack, OFF_RET_STACK) = {0};
    stack += OFF_RET_INC;

    const auto fetch = [&I, &PC]{ I = *PC; ++PC; };

    const auto mf_func_call = [&S, &mod_idx, &text, &data, &PC, &stack](std::int16_t stack_top, program_addr addr) {
        stack_cast<program_addr>(stack, stack_top + OFF_RET_ADDR) = {mod_idx, std::uint16_t(PC - text)};
        stack_cast<stack_rep>(stack, stack_top + OFF_RET_STACK).soff = stack_top;
        mod_idx = addr.mod;
        text = S.modules[mod_idx].text.data();
        data = S.modules[mod_idx].data.data();
        PC = text + addr.off;
        stack += stack_top + OFF_RET_INC;
    };

    while (true) {
        fetch();

        switch (I.OP) {
            case TERMINATE:
                return {I.A, "terminate"};

            // constant loads
            case ISETC:
                stack_cast<int>(stack, I.A) = I.DI;
                break;
            case FSETC:
                stack_cast<float>(stack, I.A) = I.DF;
                break;

            // address load
            case SETADR:
                stack_cast<program_addr>(stack, I.A) = {mod_idx, std::uint16_t(I.DI)};
                break;

            // data load
            case SETDAT:
                std::copy_n(data + I.BC.B, I.BC.C, stack + I.A);
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
                const auto& addr = stack_cast<program_addr>(stack, I.BC.B);
                mf_func_call(I.A, addr);
                break;
            }
            case RET: {
                const auto& addr = stack_cast<program_addr>(stack, OFF_RET_ADDR - OFF_RET_INC);
                mod_idx = addr.mod;
                text = S.modules[mod_idx].text.data();
                PC = text + addr.off;
                stack -= stack_cast<stack_rep>(stack, OFF_RET_STACK - OFF_RET_INC).soff + OFF_RET_INC;
                break;
            }

            // C function calls
            case CFLOAD: {
                auto& dest = stack_cast<cfunc*>(stack, I.A);
                if constexpr (sizeof(cfunc*) == 4) {
                    std::memcpy(&dest, &I.DI, 4);
                } else if constexpr (sizeof(cfunc*) == 8) {
                    std::memcpy(&dest, PC, 8);
                    ++PC;
                } else {
                    static_assert("Invalid cfunc size");
                }
                break;
            }
            case CFCALL: {
                const auto& func = stack_cast<cfunc*>(stack, I.A);
                func(&S, stack + I.A);
                break;
            }

            // polymorphic function call
            case PFCALL: {
                const auto& pfunc = stack_cast<polyfunc_rep>(stack, I.BC.B);
                switch (pfunc.type) {
                    case polyfunc_type::MOONFLOWER: {
                        mf_func_call(I.A, pfunc.moonflower_func);
                        break;
                    }
                    case polyfunc_type::C: {
                        pfunc.c_func(&S, stack + I.A);
                        break;
                    }
                }
            }

            // invalid ops
            default:
                return {-1, "invalid operation"};
        }
    }
}

}
