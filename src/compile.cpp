#include "compile.hpp"

#include "scriptlexer.hpp"
#include "scriptparser.hpp"
#include "script_context.hpp"

namespace moonflower {

translation compile(std::istream& source) {
    auto context = moonflower::script_context{};
    context.program.push_back(moonflower::instruction{moonflower::TERMINATE});

    auto& int_type = context.new_usertype("int");
    auto int_type_ptr = context.get_global_type("int");
    int_type.size = sizeof(int);
    int_type.binops[binop::ADD] = {
        { int_type_ptr, int_type_ptr,
        [](script_context& context, const address& dest, const address& lhs, const address& rhs) {
            auto dest_l = std::get<addresses::local>(dest).value;
            auto lhs_l = std::get<addresses::local>(lhs).value;
            auto rhs_l = std::get<addresses::local>(rhs).value;
            context.emit({opcode::IADD, dest_l, {lhs_l, rhs_l}});
        }}
    };
    int_type.binops[binop::SUB] = {
        { int_type_ptr, int_type_ptr,
        [](script_context& context, const address& dest, const address& lhs, const address& rhs) {
            auto dest_l = std::get<addresses::local>(dest).value;
            auto lhs_l = std::get<addresses::local>(lhs).value;
            auto rhs_l = std::get<addresses::local>(rhs).value;
            context.emit({opcode::ISUB, dest_l, {lhs_l, rhs_l}});
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
    m.text = std::move(context.program);
    m.entry_point = context.main_entry;

    return {
        success ? result::SUCCESS : result::FAIL,
        std::move(m),
        std::move(context.messages)
    };
}

}