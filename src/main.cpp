
#include "interp.hpp"

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

    int entry_point;
    moonflower::module M;

    int textsize;

    file.read(reinterpret_cast<char*>(&entry_point), 4);
    file.read(reinterpret_cast<char*>(&textsize), 4);

    M.text.resize(textsize);

    file.read(reinterpret_cast<char*>(M.text.data()), textsize * sizeof(moonflower::instruction));

    moonflower::symbol exp;

    file.read(reinterpret_cast<char*>(&exp.addr), 4);

    while (exp.addr != -1) {
        int nlen;
        file.read(reinterpret_cast<char*>(&nlen), 4);
        exp.name.resize(nlen);
        file.read(exp.name.data(), nlen);
        file.read(reinterpret_cast<char*>(&exp.addr), 4);
    }

    moonflower::state S;

    S.modules.push_back(std::move(M));
    S.stacksize = 64 * 1024 * 1024; // 64 MB stack
    S.stack = std::make_unique<std::byte[]>(S.stacksize);

    for (int i = 0; i < argc-2; ++i) {
        *reinterpret_cast<int*>(&S.stack[12+i*4]) = std::stoi(argv[2+i]);
    }

    auto ret = moonflower::interp(S, 0, entry_point, sizeof(int));

    if (ret != 0) {
        std::cerr << "error: terminated: " << ret << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "result (i) = " << *reinterpret_cast<int*>(&S.stack[0]) << std::endl;
    std::cout << "result (f) = " << *reinterpret_cast<float*>(&S.stack[0]) << std::endl;

    return EXIT_SUCCESS;
}
