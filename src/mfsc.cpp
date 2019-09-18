
#include "scriptlexer.hpp"
#include "scriptparser.hpp"
#include "script_context.hpp"
#include "types.hpp"

#include <cstddef>
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) try {
    if (argc != 3) {
        std::cerr << "usage: mfasm <source> <out>" << std::endl;
        return EXIT_FAILURE;
    }

    auto ss = std::stringstream{};

    {
        auto ifile = std::ifstream(argv[1]);
        if (!ifile) {
            std::cerr << "failed to open file: " << argv[1] << std::endl;
            return EXIT_FAILURE;
        }

        ss << ifile.rdbuf();
    }

    auto source = ss.str();

    auto context = moonflower_script::script_context{};
    context.emit(moonflower::instruction{moonflower::TERMINATE});

    auto lexer = moonflower_script::lexer{source};
    auto parser = moonflower_script::parser{lexer, context};

    parser.set_debug_level(1);

    bool success = parser.parse() == 0;
    
    for (const auto& msg : context.messages) {
        std::clog << msg << std::endl;
        if (msg.severity == moonflower::compile_message::ERROR) {
            success = false;
        }
    }

    if (!success) {
        return EXIT_FAILURE;
    }

    {
        auto ofile = std::ofstream(argv[2], std::ios::binary);
        if (!ofile) {
            std::cerr << "failed to open file: " << argv[2] << std::endl;
            return EXIT_FAILURE;
        }
        const auto data = reinterpret_cast<const char*>(context.program.data());
        const int size = context.program.size();
        const int entry_point = 0;

        ofile.write(reinterpret_cast<const char*>(&entry_point), 4);
        ofile.write(reinterpret_cast<const char*>(&size), 4);
        ofile.write(data, size * sizeof(moonflower::bc_entity));

        /*
        for (const auto& exp : tu.m.exports) {
            ofile.write(reinterpret_cast<const char*>(&exp.addr), 4);
            const int nlen = exp.name.length();
            ofile.write(reinterpret_cast<const char*>(&nlen), 4);
            ofile.write(exp.name.data(), nlen);
        }
        */
        const auto endexp = -1;
        ofile.write(reinterpret_cast<const char*>(&endexp), 4);

        if (!ofile) {
            std::cerr << "failed to write to file: " << argv[2] << std::endl;
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
} catch (const moonflower_script::parser::syntax_error& e) {
    std::cerr << "Exception: " << e.what() << "(" << e.location << ")" << std::endl;
    return EXIT_FAILURE;
} catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
