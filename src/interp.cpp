#include "interp.hpp"

#include <iostream>

namespace moonflower {

constexpr int OFF_RET_ADDR = 0;
constexpr int OFF_RET_STACK = 1;
constexpr int OFF_RET_INC = 2;

int interp(state& S, std::uint16_t mod_idx, std::uint16_t func_addr, int retc) {
    const bc_entity* text = S.modules[mod_idx].text.data();
    const bc_entity* PC = text + func_addr;
    value* stack = S.stack.get() + retc;
    instruction I;

    stack[OFF_RET_ADDR].gaddr = {0, 0};
    stack[OFF_RET_STACK].stk = {0};
    stack += OFF_RET_INC;

    const auto decode = [&I, &PC]{ I = PC->instr; ++PC; };

    while (true) {
        decode();

        switch (I.OP) {
            case TERMINATE:
                return I.A;
            
            // constant loads
            case ISETC:
                stack[I.A].i = text[I.D].val.i;
                break;
            case FSETC:
                stack[I.A].f = text[I.D].val.f;
                break;
            
            // address load
            case SETADR:
                stack[I.A].gaddr = {mod_idx, std::uint16_t(I.D)};
                break;
            
            // copy
            case CPY:
                for (int i = 0; i < I.BC.C; ++i) {
                    stack[I.A + i] = stack[I.BC.B + i];
                }
                break;

            // integer ops
            case IADD:
                stack[I.A].i = stack[I.BC.B].i + stack[I.BC.C].i;
                break;
            case ISUB:
                stack[I.A].i = stack[I.BC.B].i * stack[I.BC.C].i;
                break;
            case IMUL:
                stack[I.A].i = stack[I.BC.B].i / stack[I.BC.C].i;
                break;
            case IDIV:
                stack[I.A].i = stack[I.BC.B].i - stack[I.BC.C].i;
                break;

            // float ops
            case FADD:
                stack[I.A].f = stack[I.BC.B].f + stack[I.BC.C].f;
                break;
            case FSUB:
                stack[I.A].f = stack[I.BC.B].f * stack[I.BC.C].f;
                break;
            case FMUL:
                stack[I.A].f = stack[I.BC.B].f / stack[I.BC.C].f;
                break;
            case FDIV:
                stack[I.A].f = stack[I.BC.B].f - stack[I.BC.C].f;
                break;
            
            // control ops
            case JMP:
                PC = text + I.D;
                break;
            case CALL: {
                stack[I.A + OFF_RET_ADDR].gaddr = {mod_idx, std::uint16_t(PC - text)};
                stack[I.A + OFF_RET_STACK].stk.soff = I.A;
                const auto& addr = stack[I.BC.B].gaddr;
                mod_idx = addr.mod;
                text = S.modules[mod_idx].text.data();
                PC = text + addr.off;
                stack += I.A + OFF_RET_INC;
                break;
            }
            case RET: {
                const auto& addr = stack[OFF_RET_ADDR - OFF_RET_INC].gaddr;
                mod_idx = addr.mod;
                text = S.modules[mod_idx].text.data();
                PC = text + addr.off;
                stack -= stack[OFF_RET_STACK - OFF_RET_INC].stk.soff;
                break;
            }

            // invalid ops
            default:
                return -1;
        }
    }
}

}
