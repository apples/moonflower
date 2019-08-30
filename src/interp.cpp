#include "interp.hpp"

namespace moonflower {

constexpr int OFF_RET_ADDR = 0;
constexpr int OFF_RET_CONST = 1;
constexpr int OFF_RET_STACK = 2;

int interp(state& S) {
    const bc_entity* PC = S.text.get() + 1;
    const bc_entity* constants = S.text.get() + S.text.get()->val.funcdef.coff;
    value* stack = S.stack.get();
    instruction I;

    const auto decode = [&I, &PC]{ I = PC->instr; ++PC; };

    while (true) {
        decode();

        switch (I.OP) {
            case TERMINATE:
                return I.A;
            
            // constant loads
            case ISETC:
                stack[I.A].i = constants[I.BC.B].val.i;
                break;
            case FSETC:
                stack[I.A].f = constants[I.BC.B].val.f;
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
                PC = S.text.get() + stack[I.BC.B].i;
                break;
            case CALL:
                stack[I.A + OFF_RET_ADDR].i = PC - S.text.get();
                stack[I.A + OFF_RET_CONST].i = constants - S.text.get();
                stack[I.A + OFF_RET_STACK].i = I.A;
                PC = S.text.get() + stack[I.BC.B].i;
                constants = PC - PC->val.funcdef.coff;
                stack += I.A + 3;
                break;
            case RET:
                PC = S.text.get() + stack[OFF_RET_ADDR - 3].i;
                constants = S.text.get() + stack[OFF_RET_CONST - 3].i;
                stack -= stack[OFF_RET_STACK - 3].i;
                break;

            // invalid ops
            default:
                return -1;
        }
    }
}

}
