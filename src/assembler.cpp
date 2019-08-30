#include "assembler.hpp"

#include "asm_context.hpp"
#include "asmparser.hpp"
#include "asmlexer.hpp"

namespace moonflower {

translation translate(const char* source, std::size_t len) {
    moonflower::asm_context context;
    yyscan_t scanner;

    context.emit(instruction(TERMINATE, 0));

    moonflowerasmlex_init(&scanner);
    
    auto buffer = moonflowerasm_scan_bytes(source, len, scanner);
    
    moonflowerasmset_lineno(1, scanner);

    bool success = moonflowerasmparse(scanner, context) == 0;
    
    moonflowerasm_delete_buffer(buffer, scanner);
    moonflowerasmlex_destroy(scanner);

    return {
        success ? result::SUCCESS : result::FAIL,
        {
            std::move(context.program),
            {}
        },
        std::move(context.messages),
        context.entry_point
    };
}

}
