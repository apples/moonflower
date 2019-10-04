
#include "interp.hpp"
#include "state.hpp"

#include <cstddef>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: moonflower <bytecode_file> [args...]" << std::endl;
        return EXIT_FAILURE;
    }

    std::ifstream file (argv[1], std::ios::binary);

    if (!file) {
        std::cerr << "error: file does not exist" << std::endl;
        return EXIT_FAILURE;
    }

    moonflower::state S;

    auto [mod_idx, messages] = S.load(file);

    for (const auto& msg : messages) {
        std::clog << msg << std::endl;
    }

    if (!mod_idx) {
        return EXIT_FAILURE;
    }

    S.stacksize = 64 * 1024 * 1024; // 64 MB stack
    S.stack = std::make_unique<std::byte[]>(S.stacksize);

    for (int i = 0; i < argc-2; ++i) {
        *reinterpret_cast<int*>(&S.stack[12+i*4]) = std::stoi(argv[2+i]);
    }

    auto entry_point = S.get_entry_point(*mod_idx);

    auto ret = moonflower::interp(S, *mod_idx, entry_point, sizeof(int));

    if (ret != 0) {
        std::cerr << "error: terminated: " << ret << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "result (i) = " << *reinterpret_cast<int*>(&S.stack[0]) << std::endl;
    std::cout << "result (f) = " << *reinterpret_cast<float*>(&S.stack[0]) << std::endl;

    return EXIT_SUCCESS;
}
