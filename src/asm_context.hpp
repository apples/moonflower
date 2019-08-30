#pragma once

#include "types.hpp"
#include "compile_message.hpp"
#include "location.hpp"

#include <charconv>
#include <unordered_map>
#include <vector>

namespace moonflower {

struct asm_context {
    std::vector<bc_entity> program;
    std::unordered_map<std::string, int> constant_idxs;
    std::unordered_map<std::string, int> labels;
    std::unordered_map<std::string, std::vector<int>> label_todo;
    std::vector<compile_message> messages;
    int entry_point = -1;

    void add_label(const std::string& name, const location& loc) {
        const auto addr = program.size();
        auto iter = labels.find(name);

        if (iter == end(labels)) {
            iter = labels.emplace(name, addr).first;
        } else {
            iter->second = addr;
            messages.emplace_back(compile_message::WARNING, "Shadowing label: " + name, loc);
        }

        auto titer = label_todo.find(name);

        if (titer != end(label_todo)) {
            for (const auto& pc : titer->second) {
                auto& instr = program[pc].instr;
                switch (instr.OP) {
                    case JMP:
                        instr = instruction(JMP, 0, addr);
                        break;
                }
            }

            label_todo.erase(titer);
        }
    }

    void emit(const bc_entity& ent) {
        program.push_back(ent);
    }

    void set_entry(const location& loc) {
        if (entry_point != -1) {
            messages.emplace_back("Duplicate entry point", loc);
        }
        entry_point = program.size();
    }
};

}
