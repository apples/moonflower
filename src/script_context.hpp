#pragma once

#include "asm_context.hpp"
#include "types.hpp"
#include "compile_message.hpp"
#include "location.hpp"
#include "utility.hpp"
#include "state.hpp"

#include <cassert>
#include <charconv>
#include <unordered_map>
#include <variant>
#include <vector>

namespace moonflower {

struct script_context;

using namespace moonflower;

struct stack_object {
    addresses::local addr;
    type_ptr t;
};

struct object {
    address addr;
    type_ptr t;

    object() = default;
    object(address a, type_ptr t) : addr(a), t(std::move(t)) {}
    object(const stack_object& sobj) : addr(sobj.addr), t(sobj.t) {}
};

struct constant {
    int loc;
    type type;
    union {
        int i;
    } val;
};

struct expression {
    struct nothing {};

    struct stack_id {
        std::int16_t addr;
    };

    struct function {
        std::int16_t addr;
    };

    struct imported_function {
        std::int16_t data_addr;
    };

    struct constant {
        std::variant<int, float, bool> val;
    };

    struct binary {
        std::shared_ptr<const binop_def> def;
    };

    struct call {
        int nargs;
    };

    struct dataload {
        std::int16_t addr;
    };

    std::variant<nothing, stack_id, function, imported_function, constant, binary, call, dataload> expr;
    type_ptr type;
    category category;
};

struct variable {
    std::string name;
    stack_object obj;

    variable() = default;
    variable(std::string name, stack_object obj) : name(std::move(name)), obj(std::move(obj)) {}
};

struct function_context {
    std::string name;
    std::vector<instruction> text;
    std::vector<expression> active_exprs;
    std::vector<variable> local_stack;
    std::vector<stack_object> expr_stack;
    std::int16_t stack_top = 0;

    function_context() = default;
    function_context(std::string name) : name(std::move(name)) {}
};

struct script_context {
    static constexpr int stack_max = 16384;
    state* S;
    std::vector<instruction> program;
    std::vector<std::byte> data;
    std::vector<compile_message> messages;
    function_context cur_func;
    std::unordered_map<std::string, object> static_scope;
    int main_entry = -1;
    std::unordered_map<std::string, type_ptr> global_types;
    type_ptr nulltype;
    std::optional<std::uint16_t> current_import_module;

    script_context(state& S);

    type::usertype& new_usertype(const std::string& name);

    type_ptr get_global_type(const std::string& name);

    void begin_import(const std::string& module_name);

    void import(const std::string& func_name);

    void begin_func(const std::string& name);

    void end_func();

    void add_param(const std::string& name, const location& loc);

    auto add_local(const std::string& name, const type_ptr& t, const location& loc) -> const stack_object&;

    void promote_local(const std::string& name, const location& loc);

    auto push_object(const type_ptr& t, const location& loc) -> const stack_object&;

    void pop_object(const stack_object& obj);

    void pop_objects_until(int pos, bool skip_destroy = false);

    void emit_destroy(const object& obj);

    void push_expr(expression expr);

    stack_object get_return_object();

    std::optional<stack_object> stack_lookup(const std::string& name);

    std::optional<object> static_lookup(const std::string& name);

    int expr_id(const std::string& name, const location& loc);

    int expr_const_int(int val);

    int expr_const_bool(bool val);

    int expr_binop(binop op, int lhs_size, int rhs_size, const location& loc);

    int get_expr_size(int loc) const;

    int expr_call(int nargs, const location& loc);

    std::int16_t emit(const instruction& instr);

    void emit_return(const location& loc);

    int begin_block(const location& loc);

    void end_block(int unwind_to, bool cleanup, const location& loc);

    void clear_expr();

    void emit_vardecl(const std::string& name, const location& loc);

    void emit_discard(const location& loc);

    void emit_copy(const object& dest, const object& src);

    std::int16_t emit_if(const location& loc);

    std::int16_t emit_jmp(const location& loc);

    void set_jmp(std::int16_t addr, const location& loc);

    auto push_func_args(int expr_loc, int nargs, const location& loc) -> int;

    object eval_expr(int expr_loc, const location& loc);
};

}
