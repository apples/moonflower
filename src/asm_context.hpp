#pragma once

#include "types.hpp"
#include "compile_message.hpp"
#include "location.hpp"

#include <cassert>
#include <charconv>
#include <unordered_map>
#include <vector>

namespace moonflower {

struct asm_context {
    std::vector<instruction> program;
    std::unordered_map<std::string, int> constant_idxs;
    std::unordered_map<std::string, int> labels;
    std::unordered_map<std::string, std::vector<int>> label_todo;
    std::unordered_map<std::string, std::unordered_map<std::string, std::int16_t>> imports;
    std::unordered_map<std::string, int> cur_import;
    std::string cur_import_name;
    std::unordered_map<std::string, std::int16_t> exports;
    std::vector<compile_message> messages;
    int entry_point = -1;

    int addr() const {
        return program.size();
    }

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
                program[pc].DI = addr;
            }

            label_todo.erase(titer);
        }
    }

    int get_label(const std::string& name) {
        auto iter = labels.find(name);
        if (iter != end(labels)) {
            return iter->second;
        } else {
            return -1;
        }
    }

    void await_label(const std::string& name) {
        label_todo[name].push_back(addr());
    }

    void emit(const instruction& instr) {
        program.push_back(instr);
    }

    void set_entry(const location& loc) {
        if (entry_point != -1) {
            messages.emplace_back("Duplicate entry point", loc);
        }
        entry_point = program.size();
    }

    void add_export(std::string name, const location& loc) {
        auto [iter, success] = exports.emplace(std::move(name), addr());
        if (!success) {
            messages.emplace_back("Duplicate export: " + iter->first, loc);
        }
    }

    void begin_import(const std::string& name) {
        assert(cur_import_name.empty());
        cur_import_name = name;
    }

    void import(std::string name) {
        assert(!cur_import_name.empty());
        imports[cur_import_name].emplace(name, addr());
    }

    void end_import(const location& loc) {
        assert(!cur_import_name.empty());
        auto iter = imports.find(cur_import_name);
        if (iter != end(imports)) {
            if (iter->second.empty()) {
                messages.emplace_back(compile_message::WARNING, "Unused import: " + iter->first, loc);
                imports.erase(iter);
            }
        }
    }
};

}
