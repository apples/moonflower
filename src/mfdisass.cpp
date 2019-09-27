#include "types.hpp"

#include <cstddef>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>

int main(int argc, char* argv[]) {
    using namespace moonflower;

    if (argc != 2) {
        std::cerr << "usage: mfdisass <bytecode_file>" << std::endl;
        return EXIT_FAILURE;
    }

    std::ifstream file (argv[1], std::ios::binary);

    if (!file) {
        std::cerr << "error: file does not exist" << std::endl;
        return EXIT_FAILURE;
    }

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

    int entry_point;
    int textsize;

    file.read(reinterpret_cast<char*>(&entry_point), 4);
    file.read(reinterpret_cast<char*>(&textsize), 4);

    for (int i = 0; i < textsize; ++i) {
        if (i == entry_point) {
            std::cout << "__MAIN__:\n";
        }
        instruction instr;
        file.read(reinterpret_cast<char*>(&instr), sizeof(instruction));

        std::cout << std::setw(5) << i << ": ";

        switch (instr.OP) {
            case opcode::TERMINATE: write_A("terminate", instr); break;
            case opcode::ISETC: write_ADI("isetc", instr); break;
            case opcode::FSETC: write_ADF("fsetc", instr); break;
            case opcode::SETADR: write_ADI("setadr", instr); break;
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
