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

inline int value_size(const type& t) {
    return std::visit(overload {
        [](const type::nothing&) { return 0; },
        [](const type::integer&) { return 1; },
        [](const type::function&) { return 0; },
        [](const type::closure&) { return 1; },
    }, t.t);
}

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

struct global_addr {
    std::uint16_t mod;
    std::uint16_t off;
};

struct stack_rep {
    std::uint16_t soff;
};

union alignas(std::int32_t) value {
    std::int32_t i;
    float f;
    global_addr gaddr;
    stack_rep stk;

    value() = default;
    explicit value(std::int32_t i) : i(i) {}
    explicit value(float f) : f(f) {}
    explicit value(const global_addr& a) : gaddr(a) {}
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

struct import {
    std::string modname;
    std::vector<symbol> symbols;
};

struct module {
    std::vector<bc_entity> text;
    std::vector<symbol> exports;
    std::vector<import> imports;
};

struct state {
    std::vector<module> modules;
    std::unique_ptr<value[]> stack;
    std::size_t stacksize;
};

}
