#pragma once

#include "utility.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
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

enum class category : std::uint8_t {
    OBJECT,
    EXPIRING,
};

enum class binop : std::uint8_t {
    ADD,
    SUB,
    MUL,
    DIV,
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

    using address = std::variant<local, global>;
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

    struct closure {
        function base;
    };

    struct usertype {
        int size = 0;
        std::unordered_map<std::string, field_def> fields;
        std::unordered_map<binop, std::vector<binop_def>> binops;
        std::function<void(std::int16_t from, std::int16_t to)> emit_copy;
    };

    using variant = std::variant<nothing, function, closure, usertype>;

    variant t;

    type() = default;
    type(const type&) = delete;
    type(type&&) = default;
    type& operator=(const type&) = delete;
    type& operator=(type&&) = default;

    type(variant v): t(std::move(v)) {}
};

inline std::string to_string(const type& t) {
    using namespace std::literals;
    return std::visit(overload {
        [](const type::nothing&) { return "[nothing]"s; },
        [](const type::function&) { return "[function]"s; },
        [](const type::closure&) { return "[closure]"s; },
        [](const type::usertype&) { return "[usertype]"s; },
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
        [](const type::closure& a_c, const type::closure& b_c) { return a_c.base == b_c.base; },
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
        [](const type::nothing&) { return 0; },
        [](const type::function&) { return 0; },
        [](const type::closure&) { return 4; },
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

static_assert(sizeof(instruction) == sizeof(std::int64_t));

struct global_addr {
    std::uint16_t mod;
    std::uint16_t off;
};

struct stack_rep {
    std::uint16_t soff;
};

struct symbol {
    std::string name;
    std::int16_t addr;
};

struct import {
    std::string modname;
    std::vector<symbol> symbols;
};

struct module {
    std::vector<instruction> text;
    std::vector<symbol> exports;
    std::vector<import> imports;
    std::uint16_t entry_point;
};

}
