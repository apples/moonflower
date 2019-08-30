
#include "assembler.hpp"

#include <cstddef>
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {
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
        const auto size = tu.m.text.size() * sizeof(moonflower::bc_entity);
        if (!ofile.write(data, size)) {
            std::cerr << "failed to write to file: " << argv[2] << std::endl;
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
