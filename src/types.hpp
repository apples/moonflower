#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace moonflower {

enum opcode : uint8_t {
    TERMINATE,

    ISETC,
    FSETC,

    IADD,
    ISUB,
    IMUL,
    IDIV,

    FADD,
    FSUB,
    FMUL,
    FDIV,

    JMP,
    CALL,
    RET,
};

struct alignas(std::int32_t) instruction {
    opcode OP;
    std::int8_t A;
    union {
        struct { std::int8_t B, C; } BC;
        std::int16_t D;
    };

    instruction() = default;
    explicit instruction(opcode o) : OP(o), A(0), BC{0, 0} {}
    instruction(opcode o, std::int8_t a) : OP(o), A(a), BC{0, 0} {}
    instruction(opcode o, std::int8_t a, std::int8_t b, std::int8_t c) : OP(o), A(a), BC{b, c} {}
    instruction(opcode o, std::int8_t a, std::int16_t d) : OP(o), A(a), D(d) {}
};

struct function_def {
    std::int8_t coff;

    function_def() = default;
    explicit function_def(std::int8_t coff) : coff(coff) {}
};

struct function_addr {
    std::uint16_t mod;
    std::uint16_t off;
};

struct stack_rep {
    std::uint16_t coff;
    std::uint16_t soff;
};

union alignas(std::int32_t) value {
    std::int32_t i;
    float f;
    function_def funcdef;
    function_addr func;
    stack_rep stk;

    value() = default;
    explicit value(std::int32_t i) : i(i) {}
    explicit value(float f) : f(f) {}
    explicit value(const function_def& f) : funcdef(f) {}
    explicit value(const function_addr& f) : func(f) {}
    explicit value(const stack_rep& s) : stk(s) {}
};

union bc_entity {
    instruction instr;
    value val;

    bc_entity() = default;
    bc_entity(const instruction& i) : instr(i) {}
    bc_entity(const value& v) : val(v) {}
};

static_assert(sizeof(value) == sizeof(std::int32_t));
static_assert(sizeof(value) == sizeof(instruction));
static_assert(alignof(value) == alignof(instruction));
static_assert(sizeof(bc_entity) == sizeof(instruction));
static_assert(alignof(bc_entity) == alignof(instruction));

struct symbol {
    std::string name;
    int addr;
};

struct module {
    std::vector<bc_entity> text;
    std::vector<symbol> exports;
};

struct state {
    std::vector<module> modules;
    std::unique_ptr<value[]> stack;
    std::size_t stacksize;
};

}
