#include "assembler.hpp"

#include "asm_context.hpp"
#include "asmparser.hpp"
#include "asmlexer.hpp"

#include <algorithm>

namespace moonflower {

translation translate(const char* source, std::size_t len) {
    auto context = moonflower::asm_context{};

    context.emit(instruction(TERMINATE, 0));

    auto lexer = moonflowerasm::lexer{source};
    auto parser = moonflowerasm::parser{lexer, context};

    parser.set_debug_level(1);

    bool success = parser.parse() == 0;

    module m;
    m.text = std::move(context.program);
    m.exports.reserve(context.exports.size());

    for (const auto& [name, addr] : context.exports) {
        m.exports.push_back({ name, addr });
    }

    for (const auto& [modname, imps] : context.imports) {
        std::vector<symbol> syms;

        for (const auto& [name, addr] : imps) {
            syms.push_back({ name, addr });
        }

        m.imports.push_back({ modname, std::move(syms) });
    }

    return {
        success ? result::SUCCESS : result::FAIL,
        m,
        std::move(context.messages)
    };
}

}
