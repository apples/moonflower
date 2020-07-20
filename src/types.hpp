#pragma once

#include "utility.hpp"

#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace moonflower {

class state;

enum opcode : std::uint8_t {
    TERMINATE, // A: return code)

    ISETC, // A: dest, DI: value
    FSETC, // A: dest, DF: value
    BSETC, // A: dest, DI: boolean value
    SETADR, // A: dest, DI: text address value
    SETDAT, // A: dest, B: data address, C: size

    CPY, // A: dest, B: source, C: count

    IADD, // A: dest, B: x, C: y
    ISUB, // A: dest, B: x, C: y
    IMUL, // A: dest, B: x, C: y
    IDIV, // A: dest, B: x, C: y
    ICLT, // A: dest, B: x, C: y

    FADD, // A: dest, B: x, C: y
    FSUB, // A: dest, B: x, C: y
    FMUL, // A: dest, B: x, C: y
    FDIV, // A: dest, B: x, C: y

    JMP, // DI: text address to jump to, relative to PC
    JMPIFN, // A: stack addr of boolean value, DI: text address to jump to if false, relative to PC
    CALL, // A: stack top, B: stack addr of program_addr to call
    RET, // no args

    CFLOAD, // A: dest, B: cfunc id
    CFCALL, // A: data addr of cfunc

    PFCALL, // A: stack top, B: stack addr of polyfunc_rep
};

struct alignas(std::int64_t) instruction {
    struct BC_t { std::int16_t B, C; };

    opcode OP;
    std::uint8_t R;
    std::int16_t A;
    union {
        BC_t BC;
        std::int32_t DI;
        float DF;
        bool DB[4];
    };

    instruction() = default;
    explicit instruction(std::array<std::byte, 8> b) { std::memcpy(this, b.data(), 8); }
    explicit instruction(opcode o) : OP(o), R(0), A(0), DI(0) {}
    instruction(opcode o, std::int16_t a) : OP(o), R(0), A(a), DI(0) {}
    instruction(opcode o, std::int16_t a, BC_t bc) : OP(o), R(0), A(a), BC(bc) {}
    instruction(opcode o, std::int16_t a, std::int32_t di) : OP(o), R(0), A(a), DI(di) {}
    instruction(opcode o, std::int16_t a, float df) : OP(o), R(0), A(a), DF(df) {}
    instruction(opcode o, std::int16_t a, bool db0) : OP(o), R(0), A(a), DB{db0} {}
};

static_assert(sizeof(instruction) == sizeof(std::int64_t));

enum class terminate_reason : std::int8_t {
    NONE,
    BAD_LITERAL_TYPE,
    BAD_ARITHMETIC_TYPE,
};

enum class category : std::uint8_t {
    OBJECT,
    EXPIRING,
};

enum class binop : std::uint8_t {
    ADD,
    SUB,
    MUL,
    DIV,
    CLT,
};

struct type;

using type_ptr = std::shared_ptr<type>;

struct field_def {
    int offset;
    type_ptr type;
};

class script_context;

namespace addresses {
    struct local {
        std::int16_t value;
    };

    struct global {
        std::int16_t value;
    };

    struct data {
        std::int16_t value;
    };

    using address = std::variant<local, global, data>;
}

using addresses::address;

struct binop_def {
    type_ptr rhs_type;
    type_ptr return_type;
    std::function<void(script_context& context, const address& dest, const address& lhs, const address& rhs)> emit;
};

struct type {
    struct nothing {};

    struct function {
        type_ptr ret_type;
        std::vector<type_ptr> params;
    };

    struct function_ptr {
        function base;
    };

    struct usertype {
        std::size_t size = 0;
        std::unordered_map<std::string, field_def> fields;
        std::unordered_map<binop, std::vector<binop_def>> binops;
        std::function<void(script_context& context, std::int16_t from, std::int16_t to)> emit_copy;
        std::function<void(script_context& context, const address& from, const address& to)> emit_boolean;
    };

    using variant = std::variant<nothing, function, function_ptr, usertype>;

    variant t;

    type() = default;
    type(const type&) = delete;
    type(type&&) = default;
    type& operator=(const type&) = delete;
    type& operator=(type&&) = default;

    type(variant v): t(std::move(v)) {}
};

struct program_addr {
    std::uint16_t mod;
    std::uint16_t off;
};

struct stack_rep {
    std::uint16_t soff;
};

using cfunc = void(state* s, std::byte* stk);

enum class polyfunc_type {
    MOONFLOWER,
    C,
};

struct polyfunc_rep {
    polyfunc_type type;
    union {
        program_addr moonflower_func;
        cfunc* c_func;
    };
};

struct symbol {
    std::string name;
    std::uint16_t addr;
};

struct import {
    std::string modname;
    std::vector<symbol> symbols;
};

struct module {
    std::string name;
    std::vector<instruction> text;
    std::vector<std::byte> data;
    std::vector<symbol> exports;
    std::vector<import> imports;
    std::uint16_t entry_point;
};

inline std::string to_string(const type& t) {
    return std::visit(overload {
        [](const type::nothing&) { return "[nothing]"; },
        [](const type::function&) { return "[function]"; },
        [](const type::function_ptr&) { return "[function_ptr]"; },
        [](const type::usertype&) { return "[usertype]"; },
    }, t.t);
}

inline std::string to_string(const type_ptr& t) {
    return to_string(*t);
}

inline bool operator==(const type& a, const type& b);
inline bool operator!=(const type& a, const type& b);

inline bool operator==(const type::function& a_f, const type::function& b_f) {
    if (*a_f.ret_type != *b_f.ret_type) {
        return false;
    }

    if (a_f.params.size() != b_f.params.size()) {
        return false;
    }

    for (auto i = 0; i < a_f.params.size(); ++i) {
        if (*a_f.params[i] != *b_f.params[i]) {
            return false;
        }
    }

    return true;
}

inline bool operator==(const type& a, const type& b) {
    return std::visit(overload {
        [](const type::nothing&, const type::nothing&) { return true; },
        [](const type::function& a_f, const type::function& b_f) {return a_f == b_f; },
        [](const type::function_ptr& a_c, const type::function_ptr& b_c) { return a_c.base == b_c.base; },
        [](const type::usertype& a_ut, const type::usertype& b_ut) { return &a_ut == &b_ut; },
        [](const auto&, const auto&) { return false; }
    }, a.t, b.t);
}

inline bool operator!=(const type& a, const type& b) {
    return !(a == b);
}

inline type_ptr make_type_ptr(type::variant v) {
    return std::make_shared<type>(std::move(v));
}

inline std::int16_t value_size(const type& t) {
    return std::visit(overload {
        [](const type::nothing&) { return 0ull; },
        [](const type::function&) { return 0ull; },
        [](const type::function_ptr&) { return sizeof(program_addr); },
        [](const type::usertype& ut) { return ut.size; }
    }, t.t);
}

inline std::int16_t value_size(const type_ptr& t) {
    return value_size(*t);
}

inline auto get_binop(binop op, const type::usertype& ut, const type_ptr& rhs) -> const binop_def* {
    auto iter = ut.binops.find(op);
    if (iter != end(ut.binops)) {
        for (const auto& def : iter->second) {
            if (*def.rhs_type == *rhs) {
                return &def;
            }
        }
    }
    return nullptr;
}

}
