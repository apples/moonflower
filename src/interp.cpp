#include "interp.hpp"

#include <algorithm>
#include <iostream>

namespace moonflower {

constexpr int OFF_RET_ADDR = 0;
constexpr int OFF_RET_STACK = 4;
constexpr int OFF_RET_INC = 8;

template <typename T>
auto byte_cast(std::byte* stack, std::int16_t addr) -> T& {
    return *reinterpret_cast<T*>(stack + addr);
}

interp_result interp(state& S, std::uint16_t mod_idx, std::uint16_t func_addr, int retc) {
    const instruction* text = S.modules[mod_idx].text.data();
    const instruction* text_end = S.modules[mod_idx].text.data() + S.modules[mod_idx].text.size();
    const char* terminate_reason = "terminate";
    std::byte* data = S.modules[mod_idx].data.data();
    const instruction* PC = text + func_addr;
    std::byte* stack = S.stack.get() + retc;
    instruction I;

    byte_cast<program_addr>(stack, OFF_RET_ADDR) = {0, 0};
    byte_cast<stack_rep>(stack, OFF_RET_STACK) = {0};
    stack += OFF_RET_INC;

    const auto fetch = [&I, &PC, &text_end, &terminate_reason]{
        if (PC >= text_end) {
            I = instruction{opcode::TERMINATE, -2};
            terminate_reason = "runoff";
        } else {
            I = *PC;
            ++PC;
        }
    };

    const auto mf_func_call = [&S, &mod_idx, &text, &text_end, &data, &PC, &stack](std::int16_t stack_top, program_addr addr) {
        byte_cast<program_addr>(stack, stack_top + OFF_RET_ADDR) = {mod_idx, std::uint16_t(PC - text)};
        byte_cast<stack_rep>(stack, stack_top + OFF_RET_STACK).soff = stack_top;
        mod_idx = addr.mod;
        text = S.modules[mod_idx].text.data();
        text_end = S.modules[mod_idx].text.data() + S.modules[mod_idx].text.size();
        data = S.modules[mod_idx].data.data();
        PC = text + addr.off;
        stack += stack_top + OFF_RET_INC;
    };

    while (true) {
        fetch();

        switch (I.OP) {
            case TERMINATE:
                return {I.A, terminate_reason};

            // constant loads
            case ISETC:
                byte_cast<int>(stack, I.A) = I.DI;
                break;
            case FSETC:
                byte_cast<float>(stack, I.A) = I.DF;
                break;

            // address load
            case SETADR:
                byte_cast<program_addr>(stack, I.A) = {mod_idx, std::uint16_t(I.DI)};
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
                byte_cast<int>(stack, I.A) = byte_cast<int>(stack, I.BC.B) + byte_cast<int>(stack, I.BC.C);
                break;
            case ISUB:
                byte_cast<int>(stack, I.A) = byte_cast<int>(stack, I.BC.B) - byte_cast<int>(stack, I.BC.C);
                break;
            case IMUL:
                byte_cast<int>(stack, I.A) = byte_cast<int>(stack, I.BC.B) * byte_cast<int>(stack, I.BC.C);
                break;
            case IDIV:
                byte_cast<int>(stack, I.A) = byte_cast<int>(stack, I.BC.B) / byte_cast<int>(stack, I.BC.C);
                break;

            // float ops
            case FADD:
                byte_cast<float>(stack, I.A) = byte_cast<float>(stack, I.BC.B) + byte_cast<float>(stack, I.BC.C);
                break;
            case FSUB:
                byte_cast<float>(stack, I.A) = byte_cast<float>(stack, I.BC.B) - byte_cast<float>(stack, I.BC.C);
                break;
            case FMUL:
                byte_cast<float>(stack, I.A) = byte_cast<float>(stack, I.BC.B) * byte_cast<float>(stack, I.BC.C);
                break;
            case FDIV:
                byte_cast<float>(stack, I.A) = byte_cast<float>(stack, I.BC.B) / byte_cast<float>(stack, I.BC.C);
                break;

            // control ops
            case JMP:
                PC = text + I.DI;
                break;
            case CALL: {
                const auto& addr = byte_cast<program_addr>(stack, I.BC.B);
                mf_func_call(I.A, addr);
                break;
            }
            case RET: {
                const auto& addr = byte_cast<program_addr>(stack, OFF_RET_ADDR - OFF_RET_INC);
                mod_idx = addr.mod;
                text = S.modules[mod_idx].text.data();
                PC = text + addr.off;
                stack -= byte_cast<stack_rep>(stack, OFF_RET_STACK - OFF_RET_INC).soff + OFF_RET_INC;
                break;
            }

            // C function calls
            case CFLOAD: {
                auto& dest = byte_cast<cfunc*>(data, I.A);
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
                const auto& func = byte_cast<cfunc*>(data, I.A);
                func(&S, stack);
                break;
            }

            // polymorphic function call
            case PFCALL: {
                const auto& pfunc = byte_cast<polyfunc_rep>(stack, I.BC.B);
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
