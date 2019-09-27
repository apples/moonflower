#pragma once

#include "utility.hpp"

#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

namespace moonflower {

enum opcode : std::uint8_t {
    TERMINATE,

    ISETC,
    FSETC,
    SETADR,

    CPY,

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

enum class terminate_reason : std::int8_t {
    NONE,
    BAD_LITERAL_TYPE,
    BAD_ARITHMETIC_TYPE,
};

struct type {
    struct nothing {};

    struct integer {};

    struct function {
        recursive_wrapper<type> ret_type;
        std::vector<type> params;
    };

    struct closure {
        function base;
    };

    std::variant<nothing, integer, function, closure> t;
};

inline std::int16_t value_size(const type& t) {
    return std::visit(overload {
        [](const type::nothing&) { return 0; },
        [](const type::integer&) { return 1; },
        [](const type::function&) { return 0; },
        [](const type::closure&) { return 1; },
    }, t.t);
}

struct alignas(std::int32_t) instruction {
    struct BC_t { std::int16_t B, C; };

    opcode OP;
    std::uint8_t R;
    std::int16_t A;
    union {
        BC_t BC;
        std::int32_t DI;
        float DF;
    };

    instruction() = default;
    explicit instruction(opcode o) : OP(o), R(0), A(0), DI(0) {}
    instruction(opcode o, std::int16_t a) : OP(o), R(0), A(a), DI(0) {}
    instruction(opcode o, std::int16_t a, BC_t bc) : OP(o), R(0), A(a), BC(bc) {}
    instruction(opcode o, std::int16_t a, std::int32_t di) : OP(o), R(0), A(a), DI(di) {}
    instruction(opcode o, std::int16_t a, float df) : OP(o), R(0), A(a), DF(df) {}
};

struct global_addr {
    std::uint16_t mod;
    std::uint16_t off;
};

struct stack_rep {
    std::uint16_t soff;
};

union alignas(std::int32_t) value {
    std::uint32_t i;
    float f;
    global_addr gaddr;
    stack_rep stk;

    value() = default;
    explicit value(std::int32_t i) : i(i) {}
    explicit value(float f) : f(f) {}
    value(const global_addr& a) : gaddr(a) {}
    value(const stack_rep& s) : stk(s) {}
};

static_assert(sizeof(value) == sizeof(std::int32_t));
static_assert(sizeof(instruction) == sizeof(std::int64_t));

struct symbol {
    std::string name;
    int addr;
};

struct import {
    std::string modname;
    std::vector<symbol> symbols;
};

struct module {
    std::vector<instruction> text;
    std::vector<symbol> exports;
    std::vector<import> imports;
};

struct state {
    std::vector<module> modules;
    std::unique_ptr<value[]> stack;
    std::size_t stacksize;
};

}
