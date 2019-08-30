
#include "interp.hpp"

#include <cstddef>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: moonflower <bytecode_file>" << std::endl;
        return EXIT_FAILURE;
    }

    std::ifstream file (argv[1], std::ios::binary);

    if (!file) {
        std::cerr << "error: file does not exist" << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<char> data(std::istreambuf_iterator<char>(file), {});

    auto program = reinterpret_cast<const moonflower::bc_entity*>(data.data());

    moonflower::state S;

    S.text = std::make_unique<moonflower::bc_entity[]>(data.size());
    S.textsize = data.size();
    S.stacksize = 8 * 1024 * 1024; // 64 MB stack
    S.stack = std::make_unique<moonflower::value[]>(S.stacksize);

    auto ret = moonflower::interp(S); // +1 because expecting 1 result

    if (ret != 0) {
        std::cerr << "error: terminated: " << ret << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "result (i) = " << S.stack[0].i << std::endl;
    std::cout << "result (f) = " << S.stack[0].f << std::endl;

    return EXIT_SUCCESS;
}
