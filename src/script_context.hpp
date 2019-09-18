#pragma once

#include "asm_context.hpp"
#include "types.hpp"
#include "compile_message.hpp"
#include "location.hpp"
#include "utility.hpp"

#include <cassert>
#include <charconv>
#include <unordered_map>
#include <variant>
#include <vector>

namespace moonflower_script {

using namespace moonflower;

enum class category : uint8_t {
    OBJECT,
    EXPIRING,
};

namespace addresses {
    struct local {
        int addr;
    };

    struct global {
        int addr;
    };

    using address = std::variant<local, global>;
}

using addresses::address;

struct object {
    address address;
    type type;
};

struct constant {
    int loc;
    type type;
    union {
        int i;
    } val;
};

enum class binop : uint8_t {
    ADD,
    SUB,
    MUL,
    DIV,
};

struct expression {
    struct nothing {};

    struct stack_id {
        int addr;
    };

    struct function {
        int addr;
    };

    struct constant {
        int addr;
    };

    struct binary {
        binop op;
    };

    struct call {
        int nargs;
    };

    std::variant<nothing, stack_id, function, constant, binary, call> expr;
    type type;
    category category;
};

struct function_context {
    std::vector<bc_entity> text;
    std::vector<constant> constants;
    int next_constant = 0;
    std::vector<expression> active_exprs;
    std::unordered_map<std::string, object> stack_scope;
    int stack_top = 0;
};

struct script_context {
    std::vector<bc_entity> program;
    std::vector<compile_message> messages;
    function_context cur_func;
    std::unordered_map<std::string, object> static_scope;

    int get_or_make_const_int(int val) {
        for (const auto& c : cur_func.constants) {
            if (std::holds_alternative<type::integer>(c.type.t) && c.val.i == val) {
                return c.loc;
            }
        }

        constant c;
        c.loc = cur_func.next_constant;
        c.type = {type::integer{}};
        c.val.i = val;

        cur_func.constants.push_back(c);
        cur_func.next_constant += 1;
        
        return c.loc;
    }

    void begin_func(const std::string& name) {
        cur_func = {};
    }

    void end_func() {
        auto const_offset = program.size();

        for (const auto& c : cur_func.constants) {
            program.push_back(value{c.val.i});
        }

        for (const auto& bc : cur_func.text) {
            auto result = bc;
            switch (bc.instr.OP) {
                case opcode::ISETC:
                    result.instr.D = bc.instr.D + const_offset;
                    break;
            }
            program.push_back(result);
        }
    }

    void add_param(const std::string& name) {
        cur_func.stack_scope[name] = object{addresses::local{cur_func.stack_top}, {type::integer{}}};
        ++cur_func.stack_top;
    }    

    void push_expr(expression expr) {
        cur_func.active_exprs.push_back(expr);
    }

    std::optional<object> stack_lookup(const std::string& name) {
        auto iter = cur_func.stack_scope.find(name);
        if (iter != end(cur_func.stack_scope)) {
            return iter->second;
        } else {
            return std::nullopt;
        }
    }

    std::optional<object> static_lookup(const std::string& name) {
        auto iter = static_scope.find(name);
        if (iter != end(static_scope)) {
            return iter->second;
        } else {
            return std::nullopt;
        }
    }

    int expr_id(const std::string& name, const location& loc) {
        if (auto idx = stack_lookup(name)) {
            push_expr({expression::stack_id{std::get<addresses::local>(idx->address).addr}, idx->type, category::OBJECT});
        } else if (auto idx = static_lookup(name)) {
            std::visit(overload {
                [&](const type::function& func) {
                    push_expr({expression::function{std::get<addresses::global>(idx->address).addr}, type::closure{func}, category::EXPIRING});
                },
                [&](const auto&) {
                    push_expr({expression::nothing{}, type::nothing{}, category::OBJECT});
                    messages.emplace_back("Static name is not a function: " + name, loc);
                }
            }, idx->type.t);
        } else {
            push_expr({expression::nothing{}, type::nothing{}, category::OBJECT});
            messages.emplace_back("Could not find name: " + name, loc);
        }

        return 1;
    }

    int expr_const_int(int val) {
        push_expr({expression::constant{get_or_make_const_int(val)}, type::integer{}, category::EXPIRING});
        return 1;
    }

    int expr_binop(binop op, int lhs_size, int rhs_size, const location& loc) {
        const auto& lhs = *(rbegin(cur_func.active_exprs) + rhs_size);
        const auto& rhs = *rbegin(cur_func.active_exprs);

        std::visit(overload {
            [&](type::integer) {
                if (std::holds_alternative<type::integer>(rhs.type.t)) {
                    push_expr({expression::binary{op}, type::integer{}, category::EXPIRING});
                }
            },
            [&](auto&&) {
                push_expr({expression::nothing{}, type::nothing{}, category::OBJECT});
                messages.emplace_back("No suitable operation", loc);
            }
        }, lhs.type.t);

        return lhs_size + rhs_size + 1;
    }

    int get_expr_size(int loc) const {
        auto iter = rbegin(cur_func.active_exprs) + loc;
        const auto& expr = *iter;

        return 1 + std::visit(overload {
            [](expression::nothing) { return 0; },
            [](expression::stack_id) { return 0; },
            [](expression::function) { return 0; },
            [](expression::constant) { return 0; },
            [&](const expression::binary& e) {
                const auto rhs_size = get_expr_size(loc + 1);
                const auto lhs_size = get_expr_size(loc + 1 + rhs_size);
                return rhs_size + lhs_size;
            },
            [&](const expression::call& c) {
                auto size = 0;
                for (int i = 0; i < c.nargs; ++i) {
                    size += get_expr_size(loc + 1 + size);
                }
                size += get_expr_size(loc + 1 + size);
                return size;
            }
        }, expr.expr);
    }

    int expr_call(int nargs, const location& loc) {
        auto expr_size = 0;
        for (int i = 0; i < nargs; ++i) {
            expr_size += get_expr_size(expr_size);
        }
        const auto& funcexpr = *(rbegin(cur_func.active_exprs) + expr_size);
        expr_size += get_expr_size(expr_size);
        std::visit(overload {
            [&](const type::function& func) {
                push_expr({expression::call{nargs}, func.ret_type.get(), category::EXPIRING});
            },
            [&](const auto&) {
                cur_func.active_exprs.resize(cur_func.active_exprs.size() - expr_size);
                push_expr({expression::nothing{}, type::nothing{}, category::OBJECT});
                messages.emplace_back("Called object is not a function", loc);
            }
        }, funcexpr.type.t);
        return expr_size + 1;
    }

    int push(const type& t) {
        int addr = cur_func.stack_top;
        cur_func.stack_top += value_size(t);
        return addr;
    }

    void pop(const type& t) {
        cur_func.stack_top -= value_size(t);
    }

    void emit(const bc_entity& bc) {
        cur_func.text.push_back(bc);
    }

    void emit_return(const location& loc) {
        if (!cur_func.active_exprs.empty()) {
            auto type = cur_func.active_exprs.back().type;
            auto result = eval_expr(0, loc);
            clear_expr();
            std::visit(overload {
                [&](const addresses::local& a) {
                    emit_copy(type, -1, a.addr);
                },
                [](const addresses::global& a) { throw std::runtime_error("Not implemented."); }
            }, result.address);
        }
        emit(instruction{opcode::RET});
    }

    void clear_expr() {
        cur_func.active_exprs.clear();
    }

    void emit_vardecl(const std::string& name, const location& loc) {
        auto type = cur_func.active_exprs.back().type;
        auto dest = cur_func.stack_top;
        auto result = eval_expr(0, loc);
        clear_expr();
        auto addr = std::visit(overload {
            [](const addresses::local& a) { return a.addr; },
            [&](const addresses::global& a) -> int { 
                throw std::runtime_error("Not implemented");
            }
        }, result.address);
        cur_func.stack_scope[name] = {addresses::local{dest}, type};
    }

    struct eval_result {
        address address;
        std::function<void()> emit_cleanup;
    };

    void emit_copy(const type& t, int dest, int src) {
        emit(instruction{opcode::CPY, std::int8_t(dest), std::int8_t(src), std::int8_t(value_size(t))});
    }

    std::function<void()> push_func_args(int expr_loc, int nargs, const location& loc) {
        if (nargs > 0) {
            auto expr_size = get_expr_size(expr_loc);
            auto cleanup_rest = push_func_args(expr_loc + expr_size, nargs - 1, loc);
            auto cleanup_self = push_func_args(expr_loc, 0, loc);
            return [cleanup_self = std::move(cleanup_self), cleanup_rest = std::move(cleanup_rest)]{
                cleanup_self();
                cleanup_rest();
            };
        }

        auto dest = cur_func.stack_top;
        const auto& expr = *(rbegin(cur_func.active_exprs) + expr_loc);
        auto type = expr.type;
        auto result = eval_expr(expr_loc, loc);
        auto cleanup = std::function<void()>{};
        std::visit(overload {
            [&](const addresses::local& a) {
                if (a.addr != dest) {
                    dest = push(type);
                    emit_copy(type, dest, a.addr);
                    cleanup = [=]{ pop(type); };
                } else {
                    cleanup = std::move(result.emit_cleanup);
                }
            },
            [&](const addresses::global&) {
                throw std::runtime_error("Not implemented");
            }
        }, result.address);
        return cleanup;
    }

    eval_result eval_expr(int expr_loc, const location& loc) {
        const auto& expr = *(rbegin(cur_func.active_exprs) + expr_loc);

        auto result = std::visit(overload {
            [&](const expression::nothing&) -> eval_result { return {addresses::local{0}}; },
            [&](const expression::stack_id& id) -> eval_result { return {addresses::local{id.addr}}; },
            [&](const expression::function& id) -> eval_result {
                auto t = type{type::closure{expr.type}};
                auto addr = push(t);
                emit(instruction{opcode::SETADR, std::int8_t(addr), std::int16_t(id.addr)});
                return {addresses::local{addr}, [=]{ pop(t); }};
            },
            [&](const expression::constant& id) -> eval_result {
                auto type = expr.type;
                auto addr = push(type);
                std::visit(overload {
                    [&](const type::integer&) {
                        emit(instruction{opcode::ISETC, std::int8_t(addr), std::int16_t(id.addr)});
                    },
                    [&](const auto&) {
                        emit(instruction{opcode::TERMINATE, std::int8_t(terminate_reason::BAD_LITERAL_TYPE)});
                    }
                }, type.t);
                return {addresses::local{addr}, [=]{ pop(type); }};
            },
            [&](const expression::binary& id) -> eval_result {
                auto type = expr.type;
                auto dest = cur_func.stack_top;
                auto cleanup = std::function<void()>{};

                auto rhs_size = get_expr_size(expr_loc + 1);
                auto lhs_result = eval_expr(expr_loc + 1 + rhs_size, loc);
                auto lhs_addr = std::visit(overload {
                    [](const addresses::local& a) { return a.addr; },
                    [](const addresses::global& a) -> int { throw std::runtime_error("Not implemented."); }
                }, lhs_result.address);

                if (lhs_addr != dest) {
                    dest = push(type);
                    cleanup = [=]{ pop(type); };
                } else {
                    cleanup = std::move(lhs_result.emit_cleanup);
                }

                auto rhs_result = eval_expr(expr_loc + 1, loc);
                auto rhs_addr = std::visit(overload {
                    [](const addresses::local& a) { return a.addr; },
                    [](const addresses::global& a) -> int { throw std::runtime_error("Not implemented."); }
                }, rhs_result.address);

                std::visit(overload {
                    [&](const type::integer&) {
                        auto op = [&]{
                            switch (id.op) {
                                case binop::ADD: return opcode::IADD;
                                case binop::SUB: return opcode::ISUB;
                                case binop::MUL: return opcode::IMUL;
                                case binop::DIV: return opcode::IDIV;
                                default: return opcode::TERMINATE;
                            }
                        }();
                        emit(instruction{op, std::int8_t(dest), std::int8_t(lhs_addr), std::int8_t(rhs_addr)});
                    },
                    [&](const auto&) {
                        emit(instruction{opcode::TERMINATE, std::int8_t(terminate_reason::BAD_ARITHMETIC_TYPE)});
                        messages.emplace_back("Bad arithmetic type", loc);
                    }
                }, type.t);

                if (rhs_result.emit_cleanup) {
                    rhs_result.emit_cleanup();
                }

                if (lhs_result.emit_cleanup) {
                    lhs_result.emit_cleanup();
                }

                return {addresses::local{dest}, std::move(cleanup)};
            },
            [&](const expression::call& call) -> eval_result {
                auto type = expr.type;
                auto dest = push(type);
                auto cleanup = push_func_args(expr_loc + 1, call.nargs, loc);
                auto top = cur_func.stack_top;

                emit(instruction{opcode::CALL, std::int8_t(top), std::int8_t(dest), 0});

                cleanup();

                return {addresses::local{dest}, [=]{ pop(type); }};
            },
        }, expr.expr);

        return result;
    }
};

}
