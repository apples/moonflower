
#include "interp.hpp"
#include "state.hpp"
#include "scriptparser.hpp"

#include <cstddef>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>

#include <iomanip>

void print_i(moonflower::state* s, std::byte* stk) {
    std::cout << "print_i: " << (void*)s << ", " << (void*)stk << std::endl;
}

template <typename R, typename... Ts>
void cfunc_to_mf(moonflower::module& m, const std::string& name, R(*func)(Ts...)) {
    using namespace moonflower;

    auto text_loc = static_cast<std::uint16_t>(m.text.size());
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
    m.name = "print";
    cfunc_to_mf(m, "print_i", print_i);
    S.load(m);
}

void disass(const moonflower::module& mod) {
    using namespace moonflower;

    std::cout << "[[" << mod.name << "]]\n";

    constexpr auto cw = 7;

    auto write = [](const std::string& name) {
        std::cout << std::setw(10) << name << std::setw(cw*3) << "";
    };

    auto write_A = [](const std::string& name, instruction i) {
        std::cout << std::setw(10) << name << std::setw(cw) << i.A << std::setw(cw*2) << "";
    };

    auto write_AB = [](const std::string& name, instruction i) {
        std::cout << std::setw(10) << name << std::setw(cw) << i.A << std::setw(cw) << i.BC.B << std::setw(cw) << "";
    };

    auto write_ABC = [](const std::string& name, instruction i) {
        std::cout << std::setw(10) << name << std::setw(cw) << i.A << std::setw(cw) << i.BC.B << std::setw(cw) << i.BC.C;
    };

    auto write_DI = [](const std::string& name, instruction i) {
        std::cout << std::setw(10) << name << std::setw(cw) << "" << std::setw(cw*2) << i.DI;
    };

    auto write_DF = [](const std::string& name, instruction i) {
        std::cout << std::setw(10) << name << std::setw(cw) << "" << std::setw(cw*2) << i.DF;
    };

    auto write_ADI = [](const std::string& name, instruction i) {
        std::cout << std::setw(10) << name << std::setw(cw) << i.A << std::setw(cw*2) << i.DI;
    };

    auto write_ADF = [](const std::string& name, instruction i) {
        std::cout << std::setw(10) << name << std::setw(cw) << i.A << std::setw(cw*2) << i.DF;
    };

    int entry_point = mod.entry_point;
    int textsize = mod.text.size();

    for (int i = 0; i < textsize; ++i) {
        if (i == entry_point) {
            std::cout << "__MAIN__:\n";
        }
        instruction instr = mod.text[i];

        std::cout << std::setw(5) << i << ": ";

        switch (instr.OP) {
            case opcode::TERMINATE: write_A("terminate", instr); break;
            case opcode::ISETC: write_ADI("isetc", instr); break;
            case opcode::FSETC: write_ADF("fsetc", instr); break;
            case opcode::SETADR: write_ADI("setadr", instr); break;
            case opcode::SETDAT: write_ABC("setdat", instr); break;
            case opcode::CPY: write_ABC("cpy", instr); break;
            case opcode::IADD: write_ABC("iadd", instr); break;
            case opcode::ISUB: write_ABC("isub", instr); break;
            case opcode::IMUL: write_ABC("imul", instr); break;
            case opcode::IDIV: write_ABC("idiv", instr); break;
            case opcode::FADD: write_ABC("fadd", instr); break;
            case opcode::FSUB: write_ABC("fsub", instr); break;
            case opcode::FMUL: write_ABC("fmul", instr); break;
            case opcode::FDIV: write_ABC("fdiv", instr); break;
            case opcode::JMP: write_A("jmp", instr); break;
            case opcode::CALL: write_AB("call", instr); break;
            case opcode::RET: write("ret"); break;
            case opcode::CFLOAD: write_AB("cfload", instr); break;
            case opcode::CFCALL: write_A("cfcall", instr); break;
            case opcode::PFCALL: write_AB("pfcall", instr); break;
            default: write("???"); break;
        }

        std::byte bytes[8];
        std::memcpy(bytes, &instr, 8);

        std::cout << "  |  ";

        std::cout << std::setfill('0') << std::hex;
        for (auto i = 0; i < 7; ++i) {
            std::cout << std::setw(2) << std::to_integer<int>(bytes[i]) << " ";
        }
        std::cout << std::setw(2) << std::to_integer<int>(bytes[7]);
        std::cout << std::setfill(' ') << std::dec;

        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) try {
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

    {
        moonflower::module m;
        m.name = "$null";
        m.text.push_back(moonflower::instruction{moonflower::opcode::TERMINATE, 0});
        S.load(m);
    }

    load_core(S);

    auto [mod_idx, messages] = S.load(argv[1], file);

    for (const auto& msg : messages) {
        std::clog << msg << std::endl;
    }

    if (!mod_idx) {
        return EXIT_FAILURE;
    }

    for (auto& mod : S.modules) {
        disass(mod);
    }

    S.stacksize = 64 * 1024 * 1024; // 64 MB stack
    S.stack = std::make_unique<std::byte[]>(S.stacksize);

    for (int i = 0; i < argc-2; ++i) {
        *reinterpret_cast<int*>(&S.stack[12+i*4]) = std::stoi(argv[2+i]);
    }

    auto entry_point = S.get_entry_point(*mod_idx);

    auto ret = moonflower::interp(S, *mod_idx, entry_point, sizeof(int));

    if (ret.retval != 0) {
        std::cerr << "interp error: " << ret.error << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "result (i) = " << *reinterpret_cast<int*>(&S.stack[0]) << std::endl;
    std::cout << "result (f) = " << *reinterpret_cast<float*>(&S.stack[0]) << std::endl;

    return EXIT_SUCCESS;
} catch (const moonflower_script::parser::syntax_error& e) {
    std::cerr << "Exception: " << e.what() << "(" << e.location << ")" << std::endl;
    return EXIT_FAILURE;
} catch (const std::exception& e) {
    std::cerr << "EXCEPTION: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
