#include "compile.hpp"

#include "scriptlexer.hpp"
#include "scriptparser.hpp"
#include "script_context.hpp"

namespace moonflower {

translation compile(state& S, const std::string& name, std::istream& source) {
    auto context = moonflower::script_context{S};
    context.program.push_back(moonflower::instruction{moonflower::TERMINATE});

    auto& bool_type = context.new_usertype("bool");
    auto bool_type_ptr = context.get_global_type("bool");
    bool_type.size = sizeof(bool);
    bool_type.align = alignof(bool);
    bool_type.emit_boolean = [](script_context& context, const address& dest, const address& source) {
        auto dest_l = std::get<addresses::local>(dest).value;
        auto source_l = std::get<addresses::local>(source).value;
        if (dest_l != source_l) {
            context.emit({opcode::CPY, dest_l, {source_l, static_cast<std::int16_t>(sizeof(bool))}});
        }
    };

    auto& int_type = context.new_usertype("int");
    auto int_type_ptr = context.get_global_type("int");
    int_type.size = sizeof(int);
    int_type.align = alignof(int);
    int_type.binops[binop::ADD] = {
        { int_type_ptr, int_type_ptr,
        [](script_context& context, const address& dest, const address& lhs, const address& rhs) {
            auto dest_l = std::get<addresses::local>(dest).value;
            auto lhs_l = std::get<addresses::local>(lhs).value;
            auto rhs_l = std::get<addresses::local>(rhs).value;
            context.emit({opcode::IADD, dest_l, {lhs_l, rhs_l}});
        },
        [](script_context& context, const address& dest, const address& lhs, std::int16_t rhs) {
            auto dest_l = std::get<addresses::local>(dest).value;
            auto lhs_l = std::get<addresses::local>(lhs).value;
            context.emit({opcode::IADDC, dest_l, {lhs_l, rhs}});
        }}
    };
    int_type.binops[binop::SUB] = {
        { int_type_ptr, int_type_ptr,
        [](script_context& context, const address& dest, const address& lhs, const address& rhs) {
            auto dest_l = std::get<addresses::local>(dest).value;
            auto lhs_l = std::get<addresses::local>(lhs).value;
            auto rhs_l = std::get<addresses::local>(rhs).value;
            context.emit({opcode::ISUB, dest_l, {lhs_l, rhs_l}});
        },
        [](script_context& context, const address& dest, const address& lhs, std::int16_t rhs) {
            auto dest_l = std::get<addresses::local>(dest).value;
            auto lhs_l = std::get<addresses::local>(lhs).value;
            context.emit({opcode::IADDC, dest_l, {lhs_l, static_cast<std::int16_t>(-rhs)}});
        }}
    };
    int_type.binops[binop::MUL] = {
        { int_type_ptr, int_type_ptr,
        [](script_context& context, const address& dest, const address& lhs, const address& rhs) {
            auto dest_l = std::get<addresses::local>(dest).value;
            auto lhs_l = std::get<addresses::local>(lhs).value;
            auto rhs_l = std::get<addresses::local>(rhs).value;
            context.emit({opcode::IMUL, dest_l, {lhs_l, rhs_l}});
        }}
    };
    int_type.binops[binop::DIV] = {
        { int_type_ptr, int_type_ptr,
        [](script_context& context, const address& dest, const address& lhs, const address& rhs) {
            auto dest_l = std::get<addresses::local>(dest).value;
            auto lhs_l = std::get<addresses::local>(lhs).value;
            auto rhs_l = std::get<addresses::local>(rhs).value;
            context.emit({opcode::IDIV, dest_l, {lhs_l, rhs_l}});
        }}
    };
    int_type.binops[binop::CLT] = {
        { int_type_ptr, bool_type_ptr,
        [](script_context& context, const address& dest, const address& lhs, const address& rhs) {
            auto dest_l = std::get<addresses::local>(dest).value;
            auto lhs_l = std::get<addresses::local>(lhs).value;
            auto rhs_l = std::get<addresses::local>(rhs).value;
            context.emit({opcode::ICLT, dest_l, {lhs_l, rhs_l}});
        },
        [](script_context& context, const address& dest, const address& lhs, std::int16_t rhs) {
            auto dest_l = std::get<addresses::local>(dest).value;
            auto lhs_l = std::get<addresses::local>(lhs).value;
            context.emit({opcode::ICLTC, dest_l, {lhs_l, rhs}});
        }}
    };

    auto lexer = moonflower_script::lexer{source};
    auto parser = moonflower_script::parser{lexer, context};

    //parser.set_debug_level(1);

    bool success = parser.parse() == 0;

    for (auto& msg : context.messages) {
        if (msg.severity == compile_message::ERROR) {
            success = false;
            break;
        }
    }

    module m;
    m.name = name;
    m.text = std::move(context.program);
    m.data = std::move(context.data);
    m.entry_point = context.main_entry;

    return {
        success ? result::SUCCESS : result::FAIL,
        std::move(m),
        std::move(context.messages)
    };
}

}