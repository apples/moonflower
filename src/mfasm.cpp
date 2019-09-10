
#include "assembler.hpp"
#include "asmparser.hpp"

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

    auto tu = moonflower::translate(source.data(), source.length());

    for (const auto& msg : tu.messages) {
        std::clog << msg << std::endl;
    }

    if (tu.r == moonflower::result::FAIL) {
        return EXIT_FAILURE;
    }

    {
        auto ofile = std::ofstream(argv[2], std::ios::binary);
        if (!ofile) {
            std::cerr << "failed to open file: " << argv[2] << std::endl;
            return EXIT_FAILURE;
        }
        const auto data = reinterpret_cast<const char*>(tu.m.text.data());
        const int size = tu.m.text.size();

        ofile.write(reinterpret_cast<const char*>(&tu.entry_point), 4);
        ofile.write(reinterpret_cast<const char*>(&size), 4);
        ofile.write(data, size * sizeof(moonflower::bc_entity));

        for (const auto& exp : tu.m.exports) {
            ofile.write(reinterpret_cast<const char*>(&exp.addr), 4);
            const int nlen = exp.name.length();
            ofile.write(reinterpret_cast<const char*>(&nlen), 4);
            ofile.write(exp.name.data(), nlen);
        }
        const auto endexp = -1;
        ofile.write(reinterpret_cast<const char*>(&endexp), 4);

        if (!ofile) {
            std::cerr << "failed to write to file: " << argv[2] << std::endl;
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
} catch (const moonflowerasm::parser::syntax_error& e) {
    std::cerr << "Exception: " << e.what() << "(" << e.location << ")" << std::endl;
    return EXIT_FAILURE;
} catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
