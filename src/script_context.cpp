#include "script_context.hpp"

#include "state.hpp"

namespace moonflower {

script_context::script_context(state& S) : S(&S) {
    nulltype = std::make_shared<type>(type::nothing{});
}

type::usertype& script_context::new_usertype(const std::string& name) {
    auto t = std::make_shared<type>(type::usertype{});
    global_types[name] = t;
    return std::get<type::usertype>(t->t);
}

type_ptr script_context::get_global_type(const std::string& name) {
    auto iter = global_types.find(name);
    if (iter != end(global_types)) {
        return iter->second;
    } else {
        messages.emplace_back("Type does not exist: " + name, location{});
        return nulltype;
    }
}

void script_context::begin_import(const std::string& module_name) {
    for (std::uint16_t mi = 0; mi < S->modules.size(); ++mi) {
        auto& m = S->modules[mi];
        if (m.name == module_name) {
            current_import_module = mi;
            return;
        }
    }
    current_import_module = std::nullopt;
    messages.emplace_back("Module not loaded: " + module_name, location{});
}

void script_context::import(const std::string& func_name) {
    if (current_import_module) {
        auto& m = S->modules[*current_import_module];
        for (const auto& e : m.exports) {
            if (e.name == func_name) {
                auto adr = program_addr{
                    *current_import_module,
                    e.addr,
                };
                auto type = make_type_ptr(type::function_ptr{make_type_ptr(type::nothing{}), {}});
                static_scope.insert_or_assign(func_name, object{addresses::data{static_cast<std::int16_t>(data.size())}, type});
                auto adr_bytes = reinterpret_cast<std::byte*>(&adr);
                data.insert(data.end(), adr_bytes, adr_bytes + sizeof(adr));
            }
        }
    }
}

void script_context::begin_func(const std::string& name) {
    cur_func = {name};
}

void script_context::end_func() {
    auto entry = std::int16_t(program.size());

    if (cur_func.name == "main") {
        main_entry = entry;
    }

    auto rettype = get_global_type("int");

    auto t = std::make_shared<type>(type::function{rettype, {}});

    static_scope[cur_func.name] = {addresses::global{entry}, t}; // TODO: function types

    for (const auto& instr : cur_func.text) {
        // address fixups go here
        program.push_back(instr);
    }
}

void script_context::add_param(const std::string& name, const location& loc) {
    add_local(name, get_global_type("int"), loc);
}

auto script_context::add_local(const std::string& name, const type_ptr& t, const location& loc) -> const stack_object& {
    if (!cur_func.expr_stack.empty()) {
        messages.emplace_back("Can't add local while expressions are on the stack", loc);
        pop_objects_until(0);
    }
    cur_func.local_stack.emplace_back(name, stack_object{{cur_func.stack_top}, t});
    cur_func.stack_top += value_size(*t);
    return cur_func.local_stack.back().obj;
}

void script_context::promote_local(const std::string& name, const location& loc) {
    if (cur_func.expr_stack.size() != 1) {
        messages.emplace_back("Only one object must be on the stack to promote", loc);
        pop_objects_until(1);
    }
    cur_func.local_stack.emplace_back(name, std::move(cur_func.expr_stack.back()));
    cur_func.expr_stack.pop_back();
}

auto script_context::push_object(const type_ptr& t, const location& loc) -> const stack_object& {
    cur_func.expr_stack.push_back({addresses::local{cur_func.stack_top}, t});
    cur_func.stack_top += value_size(t);
    if (cur_func.stack_top > stack_max) {
        messages.emplace_back("Stack overflow", loc);
    }
    return cur_func.expr_stack.back();
}

void script_context::pop_object(const stack_object& obj) {
    if (cur_func.expr_stack.empty()) {
        throw std::runtime_error("Nothing to pop!");
    }
    if (obj.addr.value != cur_func.expr_stack.back().addr.value) {
        throw std::runtime_error("Popping the wrong object!");
    }
    emit_destroy(obj);
    cur_func.stack_top = obj.addr.value;
    cur_func.expr_stack.pop_back();
}

void script_context::pop_objects_until(int pos, bool skip_destroy) {
    while (cur_func.expr_stack.size() > pos) {
        if (!skip_destroy) {
            emit_destroy(cur_func.expr_stack.back());
        }
        cur_func.stack_top = cur_func.expr_stack.back().addr.value;
        cur_func.expr_stack.pop_back();
    }
}

void script_context::emit_destroy(const object& obj) {
    // nothing needs it yet...
}

void script_context::push_expr(expression expr) {
    cur_func.active_exprs.push_back(expr);
}

stack_object script_context::get_return_object() {
    auto t = get_global_type("int");
    auto s = value_size(t);
    return {{std::int16_t(-8 - s)}, t};
}

std::optional<stack_object> script_context::stack_lookup(const std::string& name) {
    for (const auto& var : cur_func.local_stack) {
        if (var.name == name) {
            return var.obj;
        }
    }
    return std::nullopt;
}

std::optional<object> script_context::static_lookup(const std::string& name) {
    auto iter = static_scope.find(name);
    if (iter != end(static_scope)) {
        return iter->second;
    } else {
        return std::nullopt;
    }
}

int script_context::expr_id(const std::string& name, const location& loc) {
    if (auto idx = stack_lookup(name)) {
        push_expr({expression::stack_id{idx->addr.value}, idx->t, category::OBJECT});
    } else if (auto idx = static_lookup(name)) {
        std::visit(overload {
            [&](const type::function& func) {
                std::visit(overload{
                    [&](const addresses::global& g){
                        push_expr({expression::function{g.value}, std::make_shared<type>(type::function_ptr{func}), category::EXPIRING});
                    },
                    [&](const addresses::data& d){
                        throw std::runtime_error("Not yet implemented");
                    },
                    [&](const addresses::local&){
                        throw std::runtime_error("Not yet implemented");
                    },
                }, idx->addr);
            },
            [&](const type::function_ptr& func_ptr) {
                std::visit(overload{
                    [&](const addresses::global&){
                        throw std::runtime_error("Not yet implemented");
                    },
                    [&](const addresses::data& d){
                        push_expr({expression::imported_function{d.value}, make_type_ptr(func_ptr), category::OBJECT});
                    },
                    [&](const addresses::local&){
                        throw std::runtime_error("Not yet implemented");
                    },
                }, idx->addr);
            },
            [&](const auto&) {
                push_expr({expression::nothing{}, make_type_ptr(type::nothing{}), category::OBJECT});
                messages.emplace_back("Static name is not a function: " + name, loc);
            }
        }, idx->t->t);
    } else {
        push_expr({expression::nothing{}, make_type_ptr(type::nothing{}), category::OBJECT});
        messages.emplace_back("Could not find name: " + name, loc);
    }

    return 1;
}

int script_context::expr_const_int(int val) {
    push_expr({expression::constant{val}, get_global_type("int"), category::EXPIRING});
    return 1;
}

int script_context::expr_const_bool(bool val) {
    push_expr({expression::constant{val}, get_global_type("bool"), category::EXPIRING});
    return 1;
}

int script_context::expr_binop(binop op, int lhs_size, int rhs_size, const location& loc) {
    auto lhs = *(rbegin(cur_func.active_exprs) + rhs_size);
    auto rhs = *rbegin(cur_func.active_exprs);

    std::visit(overload {
        [&](const type::usertype& lhs_ut) {
            auto iter = lhs_ut.binops.find(op);
            if (iter == end(lhs_ut.binops)) {
                push_expr({expression::nothing{}, make_type_ptr(type::nothing{}), category::OBJECT});
                messages.emplace_back("LHS does not have operator", loc);
            } else {
                auto overload_op = std::find_if(begin(iter->second), end(iter->second), [&](const auto& def) {
                    return *def.rhs_type == *rhs.type;
                });

                if (overload_op == end(iter->second)) {
                    push_expr({expression::nothing{}, make_type_ptr(type::nothing{}), category::OBJECT});
                    messages.emplace_back("No suitable overload", loc);
                } else {
                    push_expr({expression::binary{{lhs.type, &*overload_op}}, overload_op->return_type, category::EXPIRING});
                }
            }
        },
        [&](const auto&) {
            push_expr({expression::nothing{}, make_type_ptr(type::nothing{}), category::OBJECT});
            messages.emplace_back("No suitable operation (LHS type is `" + to_string(lhs.type) + "`)", loc);
        }
    }, lhs.type->t);

    return lhs_size + rhs_size + 1;
}

int script_context::get_expr_size(int loc) const {
    auto iter = rbegin(cur_func.active_exprs) + loc;
    const auto& expr = *iter;

    return 1 + std::visit(overload {
        [](expression::nothing) { return 0; },
        [](expression::stack_id) { return 0; },
        [](expression::function) { return 0; },
        [](expression::imported_function) { return 0; },
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
        },
        [&](const expression::dataload& dl) { return 0; },
    }, expr.expr);
}

int script_context::expr_call(int nargs, const location& loc) {
    auto expr_size = 0;
    for (int i = 0; i < nargs; ++i) {
        expr_size += get_expr_size(expr_size);
    }
    const auto& funcexpr = *(rbegin(cur_func.active_exprs) + expr_size);
    expr_size += get_expr_size(expr_size);
    std::visit(overload {
        [&](const type::function_ptr& func) {
            push_expr({expression::call{nargs}, func.base.ret_type, category::EXPIRING});
        },
        [&](const auto&) {
            cur_func.active_exprs.resize(cur_func.active_exprs.size() - expr_size);
            push_expr({expression::nothing{}, make_type_ptr(type::nothing{}), category::OBJECT});
            messages.emplace_back("Called object is not callable", loc);
        }
    }, funcexpr.type->t);
    return expr_size + 1;
}

std::int16_t script_context::emit(const instruction& instr) {
    auto ret = cur_func.text.size();
    cur_func.text.push_back(instr);
    return static_cast<std::int16_t>(ret);
}

void script_context::emit_return(const location& loc) {
    if (!cur_func.active_exprs.empty()) {
        auto type = cur_func.active_exprs.back().type;
        auto result = eval_expr(0, loc);
        clear_expr();
        std::visit(overload {
            [&](const addresses::local& a) {
                emit_copy(get_return_object(), result);
            },
            [](const addresses::data& a) { throw std::runtime_error("Not implemented."); },
            [](const addresses::global& a) { throw std::runtime_error("Not implemented."); }
        }, result.addr);
    }
    emit(instruction{opcode::RET});
}

void script_context::clear_expr() {
    cur_func.active_exprs.clear();
}

void script_context::emit_vardecl(const std::string& name, const location& loc) {
    auto type = cur_func.active_exprs.back().type;
    auto dest = cur_func.stack_top;
    auto result = eval_expr(0, loc);
    clear_expr();
    std::visit(overload {
        [&](const addresses::local& a) {
            if (a.value == dest) {
                promote_local(name, loc);
            } else {
                auto local = add_local(name, result.t, loc);
                emit_copy(local, result);
            }
        },
        [&](const addresses::data& a) {
            throw std::runtime_error("Not implemented");
        },
        [&](const addresses::global& a) {
            throw std::runtime_error("Not implemented");
        }
    }, result.addr);
}

void script_context::emit_discard(const location& loc) {
    auto type = cur_func.active_exprs.back().type;
    auto dest = cur_func.stack_top;
    auto result = eval_expr(0, loc);
    clear_expr();
    std::visit(overload {
        [&](const addresses::local& a) {
            emit_destroy(result);
        },
        [&](const addresses::data& a) {
            throw std::runtime_error("Not implemented");
        },
        [&](const addresses::global& a) {
            throw std::runtime_error("Not implemented");
        }
    }, result.addr);
}

void script_context::emit_copy(const object& dest, const object& src) {
    auto dest_addr = std::get<addresses::local>(dest.addr).value;
    auto src_addr = std::get<addresses::local>(src.addr).value;
    emit({opcode::CPY, dest_addr, {src_addr, value_size(dest.t)}});
}

std::int16_t script_context::emit_if(const location& loc) {
    auto bool_type = get_global_type("bool");
    auto type = cur_func.active_exprs.back().type;
    if (type != bool_type) {
        throw std::runtime_error("Not implemented: conversion to bool.");
    }
    auto result = eval_expr(0, loc);
    clear_expr();
    return std::visit(overload {
        [&](const addresses::local& a) {
            return emit({opcode::JMPIFN, a.value, 0});
        },
        [](const addresses::data& a) -> std::int16_t { throw std::runtime_error("Not implemented."); },
        [](const addresses::global& a) -> std::int16_t { throw std::runtime_error("Not implemented."); }
    }, result.addr);
}

std::int16_t script_context::emit_jmp(const location& loc) {
    return emit({opcode::JMP, 0, 0});
}

void script_context::set_jmp(std::int16_t addr, const location& loc) {
    cur_func.text[addr].DI = static_cast<std::int16_t>(cur_func.text.size()) - addr - 1;
}

auto script_context::push_func_args(int expr_loc, int nargs, const location& loc) -> int {
    if (nargs > 0) {
        auto expr_size = get_expr_size(expr_loc);
        auto func_loc = push_func_args(expr_loc + expr_size, nargs - 1, loc);

        auto dest = cur_func.stack_top;
        const auto& expr = *(rbegin(cur_func.active_exprs) + expr_loc);
        auto type = expr.type;
        auto result = eval_expr(expr_loc, loc);

        std::visit(overload {
            [&](const addresses::local& a) {
                if (a.value != dest) {
                    auto dest = push_object(type, loc);
                    emit_copy(dest, result);
                }
            },
            [&](const addresses::data&) {
                throw std::runtime_error("Not implemented");
            },
            [&](const addresses::global&) {
                throw std::runtime_error("Not implemented");
            },
        },
        result.addr);

        return func_loc;
    }

    return expr_loc;
}

object script_context::eval_expr(int expr_loc, const location& loc) {
    const auto& expr = *(rbegin(cur_func.active_exprs) + expr_loc);

    auto result = std::visit(overload {
        [&](const expression::nothing&) -> object { return {addresses::local{0}, make_type_ptr(type::nothing{})}; },
        [&](const expression::stack_id& id) -> object { return {addresses::local{id.addr}, expr.type}; },
        [&](const expression::function& id) -> object {
            auto t = make_type_ptr(type::function_ptr{expr.type});
            auto func = push_object(t, loc);
            emit(instruction{opcode::SETADR, std::int16_t(func.addr.value), std::int16_t(id.addr)});
            return func;
        },
        [&](const expression::imported_function& id) -> object {
            auto t = make_type_ptr(type::function_ptr{expr.type});
            auto func = push_object(t, loc);
            emit(instruction{opcode::SETDAT, std::int16_t(func.addr.value), {std::int16_t(id.data_addr), value_size(t)}});
            return func;
        },
        [&](const expression::constant& id) -> object {
            auto type = expr.type;
            auto val = push_object(type, loc);
            std::visit(overload {
                [&](const int& i) {
                    emit({opcode::ISETC, val.addr.value, i});
                },
                [&](const float& f) {
                    emit({opcode::FSETC, val.addr.value, f});
                },
                [&](const bool& b) {
                    emit({opcode::BSETC, val.addr.value, b});
                },
            }, id.val);
            return val;
        },
        [&](const expression::binary& id) -> object {
            auto dest = stack_object{{cur_func.stack_top}, expr.type};

            auto rhs_size = get_expr_size(expr_loc + 1);
            auto lhs_result = eval_expr(expr_loc + 1 + rhs_size, loc);
            auto lhs_addr = std::visit(overload {
                [](const addresses::local& a) { return a.value; },
                [](const addresses::data& a) -> std::int16_t { throw std::runtime_error("Not implemented."); },
                [](const addresses::global& a) -> std::int16_t { throw std::runtime_error("Not implemented."); }
            }, lhs_result.addr);

            if (lhs_addr != dest.addr.value) {
                dest = push_object(dest.t, loc);
            }

            auto unwind_loc = cur_func.expr_stack.size();

            auto rhs_result = eval_expr(expr_loc + 1, loc);
            auto rhs_addr = std::visit(overload {
                [](const addresses::local& a) { return a.value; },
                [](const addresses::data& a) -> std::int16_t { throw std::runtime_error("Not implemented."); },
                [](const addresses::global& a) -> std::int16_t { throw std::runtime_error("Not implemented."); }
            }, rhs_result.addr);

            id.def->emit(*this, dest.addr, lhs_result.addr, rhs_result.addr);

            pop_objects_until(unwind_loc);

            return dest;
        },
        [&](const expression::call& call) -> object {
            auto return_type = expr.type;
            auto dest = push_object(return_type, loc);
            auto unwind_loc = cur_func.expr_stack.size();
            auto new_stack = cur_func.stack_top;
            push_object({get_global_type("int")}, loc); // return address
            push_object({get_global_type("int")}, loc); // return stck
            auto func_loc = push_func_args(expr_loc + 1, call.nargs, loc);

            auto func_obj = eval_expr(func_loc, loc);

            auto target = std::visit(overload {
                [&](const addresses::local& a) { return a.value; },
                [&](const addresses::data& d) -> std::int16_t {
                    throw std::runtime_error("Not implemented");
                },
                [&](const addresses::global&) -> std::int16_t {
                    throw std::runtime_error("Not implemented");
                },
            }, func_obj.addr);

            emit({opcode::CALL, new_stack, {target, 0}});

            pop_objects_until(unwind_loc, true);

            return dest;
        },
        [&](const expression::dataload& dl) -> object {
            auto type = expr.type;
            auto val = push_object(type, loc);
            emit({opcode::SETDAT, val.addr.value, {dl.addr, value_size(*type)}});
            return val;
        },
    }, expr.expr);

    return result;
}

} // namespace moonflower
