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
    std::vector<compile_message> messages;

    void add_label(const std::string& name, const location& loc) {
        if (!labels.emplace(name, program.size()).second) {
            labels[name] = program.size();
            messages.emplace_back(compile_message::WARNING, "Shadowing label: " + name, loc);
        }
    }

    void emit(const bc_entity& ent) {
        program.push_back(ent);
    }
};

}
