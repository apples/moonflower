#include "interp.hpp"

#include <algorithm>

#ifdef NDEBUG
#define MOONFLOWER_DEBUG 0
#else
#define MOONFLOWER_DEBUG 1
#include <iostream>
#endif

#define MOONFLOWER_PROFILE 0

#if MOONFLOWER_PROFILE
#include <chrono>
#include <iostream>
#include <iomanip>
#endif

namespace moonflower {

constexpr int OFF_RET_ADDR = 0;
constexpr int OFF_RET_STACK = 4;

template <typename T>
auto byte_cast(std::byte* stack, std::int16_t addr) -> T& {
    return *reinterpret_cast<T*>(stack + addr);
}

#if MOONFLOWER_PROFILE
using clock = std::chrono::system_clock;
struct profile_context {
    int counts[PFCALL + 1] = {};
    clock::duration results[PFCALL + 1] = {};
    clock::time_point start;
    ~profile_context() {
        std::cout << "[PROFILE]        " <<
            std::setw(12) << "count" << " " <<
            std::setw(12) << "total dur" << " " <<
            std::setw(12) << "avg dur" << "\n";
        for (int i = 0; i < PFCALL + 1; ++i) {
            std::cout << "[PROFILE] (" << std::setw(3) << i << "): " <<
                std::setw(12) << counts[i] << " " <<
                std::setw(12) << results[i].count() << " " <<
                std::setw(12) << (counts[i] != 0 ? results[i].count()/counts[i] : 0) << "\n";
        }
    }
};
static profile_context profile_ctx;
#endif
interp_result interp(state& S, std::uint16_t mod_idx, std::uint16_t func_addr, int retc) {
    const instruction* text = S.modules[mod_idx].text.data();
#if MOONFLOWER_DEBUG
    const instruction* text_end = S.modules[mod_idx].text.data() + S.modules[mod_idx].text.size();
    volatile int icount = 0;
#endif
    const char* terminate_reason = "terminate";
    std::byte* data = S.modules[mod_idx].data.data();
    const instruction* PC = text + func_addr;
    std::byte* stack = S.stack.get() + retc;
    instruction I;

    byte_cast<program_addr>(stack, OFF_RET_ADDR) = {0, 0};
    byte_cast<stack_rep>(stack, OFF_RET_STACK) = {0};

#if MOONFLOWER_DEBUG
    const auto fetch = [&]{
        ++icount;
        if (PC >= text_end) {
            I = instruction{opcode::TERMINATE, -2};
            terminate_reason = "runoff";
            std::cerr << "runoff at " << (PC - text) << " module " << S.modules[mod_idx].name << " text " << (void*)text << " text_end " << (void*)text_end << "\n";
        } else {
            I = *PC;
            ++PC;
        }
    };
#else
    const auto fetch = [&I, &PC]{
        I = *PC;
        ++PC;
    };
#endif

    const auto mf_func_call = [&](std::int16_t stack_top, program_addr addr) {
        stack += stack_top;
        byte_cast<program_addr>(stack, OFF_RET_ADDR) = {mod_idx, std::uint16_t(PC - text)};
        byte_cast<stack_rep>(stack, OFF_RET_STACK).soff = stack_top;
        mod_idx = addr.mod;
        text = S.modules[mod_idx].text.data();
        data = S.modules[mod_idx].data.data();
        PC = text + addr.off;
#if MOONFLOWER_DEBUG
        text_end = text + S.modules[mod_idx].text.size();
#endif
    };

    while (true) {
        fetch();

#if MOONFLOWER_PROFILE
        profile_ctx.start = clock::now();
#endif

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
            case BSETC:
                byte_cast<bool>(stack, I.A) = I.DB[0];
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
                if (I.BC.C == sizeof(int)) { // optimization
                    std::copy_n(stack + I.BC.B, sizeof(int), stack + I.A);
                } else if (I.BC.C == sizeof(void*)) { // optimization
                    std::copy_n(stack + I.BC.B, sizeof(void*), stack + I.A);
                } else {
                    std::copy_n(stack + I.BC.B, I.BC.C, stack + I.A);
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
            case ICLT:
                byte_cast<bool>(stack, I.A) = byte_cast<int>(stack, I.BC.B) < byte_cast<int>(stack, I.BC.C);
                break;
            
            // integer constant ops
            case IADDC:
                byte_cast<int>(stack, I.A) = byte_cast<int>(stack, I.BC.B) + I.BC.C;
                break;
            case ICLTC:
                byte_cast<bool>(stack, I.A) = byte_cast<int>(stack, I.BC.B) < I.BC.C;
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
                PC += I.DI;
                break;
            case JMPIFN:
                if (!byte_cast<bool>(stack, I.A)) {
                    PC += I.DI;
                }
                break;
            case CALL: {
                const auto& addr = byte_cast<program_addr>(stack, I.BC.B);
                mf_func_call(I.A, addr);
                break;
            }
            case RET: {
                const auto& addr = byte_cast<program_addr>(stack, OFF_RET_ADDR);
                mod_idx = addr.mod;
                text = S.modules[mod_idx].text.data();
                data = S.modules[mod_idx].data.data();
                PC = text + addr.off;
#if MOONFLOWER_DEBUG
                text_end = text + S.modules[mod_idx].text.size();
#endif
                stack -= byte_cast<stack_rep>(stack, OFF_RET_STACK).soff;
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

#if MOONFLOWER_PROFILE
        profile_ctx.results[I.OP] += clock::now() - profile_ctx.start;
        ++profile_ctx.counts[I.OP];
#endif
    }
}

}
