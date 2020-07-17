
#include "interp.hpp"
#include "state.hpp"

#include <cstddef>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>

void print_i(moonflower::state* s, std::byte* stk) {
    std::cout << "print_i: " << (void*)s << ", " << (void*)stk << std::endl;
}

template <typename R, typename... Ts>
void cfunc_to_mf(moonflower::module& m, const std::string& name, R(*func)(Ts...)) {
    using namespace moonflower;

    auto text_loc = static_cast<std::int16_t>(m.text.size());
    auto data_loc = static_cast<std::int16_t>(m.data.size());
    auto b = reinterpret_cast<const std::byte*>(&func);
    auto e = b + sizeof(func);

    m.data.insert(m.data.end(), b, e);
    m.exports.push_back({name, text_loc});

    m.text.push_back(instruction{opcode::SETDAT, 0, {data_loc, static_cast<std::int16_t>(sizeof(func))}});
    m.text.push_back(instruction{opcode::CFCALL, 0});
}


void load_core(moonflower::state& S) {
    using namespace moonflower;
    module m;
    cfunc_to_mf(m, "print_i", print_i);
    S.load(m);
}

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

    load_core(S);

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
