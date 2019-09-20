#include "types.hpp"

#include <cstddef>
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

    auto write = [](const std::string& name) {
        std::cout << std::setw(10) << name << std::setw(15) << "";
    };

    auto write_A = [](const std::string& name, instruction i) {
        std::cout << std::setw(10) << name << std::setw(5) << int(i.A) << std::setw(10) << "";
    };

    auto write_AB = [](const std::string& name, instruction i) {
        std::cout << std::setw(10) << name << std::setw(5) << int(i.A) << std::setw(5) << int(i.BC.B) << std::setw(5) << "";
    };

    auto write_ABC = [](const std::string& name, instruction i) {
        std::cout << std::setw(10) << name << std::setw(5) << int(i.A) << std::setw(5) << int(i.BC.B) << std::setw(5) << int(i.BC.C);
    };

    auto write_D = [](const std::string& name, instruction i) {
        std::cout << std::setw(10) << name << std::setw(5) << "" << std::setw(10) << int(i.D);
    };

    auto write_AD = [](const std::string& name, instruction i) {
        std::cout << std::setw(10) << name << std::setw(5) << int(i.A) << std::setw(10) << int(i.D);
    };

    int entry_point;
    int textsize;

    file.read(reinterpret_cast<char*>(&entry_point), 4);
    file.read(reinterpret_cast<char*>(&textsize), 4);

    for (int i = 0; i < textsize; ++i) {
        if (i == entry_point) {
            std::cout << "__MAIN__:\n";
        }
        bc_entity ent;
        file.read(reinterpret_cast<char*>(&ent), sizeof(bc_entity));

        std::cout << std::setw(5) << i << ": ";

        switch (ent.instr.OP) {
            case opcode::TERMINATE: write_A("terminate", ent.instr); break;
            case opcode::ISETC: write_AD("isetc", ent.instr); break;
            case opcode::FSETC: write_AD("fsetc", ent.instr); break;
            case opcode::SETADR: write_AD("setadr", ent.instr); break;
            case opcode::CPY: write_ABC("cpy", ent.instr); break;
            case opcode::IADD: write_ABC("iadd", ent.instr); break;
            case opcode::ISUB: write_ABC("isub", ent.instr); break;
            case opcode::IMUL: write_ABC("imul", ent.instr); break;
            case opcode::IDIV: write_ABC("idiv", ent.instr); break;
            case opcode::FADD: write_ABC("fadd", ent.instr); break;
            case opcode::FSUB: write_ABC("fsub", ent.instr); break;
            case opcode::FMUL: write_ABC("fmul", ent.instr); break;
            case opcode::FDIV: write_ABC("fdiv", ent.instr); break;
            case opcode::JMP: write_A("jmp", ent.instr); break;
            case opcode::CALL: write_AB("call", ent.instr); break;
            case opcode::RET: write("ret"); break;
            default: write("???"); break;
        }

        std::cout << "  |  " << std::setw(11) << ent.val.i << "\n";
    }
}
