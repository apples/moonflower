#pragma once

#include "types.hpp"
#include "compile_message.hpp"
#include "script_context.hpp"

#include <iostream>
#include <optional>
#include <unordered_map>
#include <vector>

namespace moonflower {

struct load_result {
    std::optional<std::int16_t> mod_idx;
    std::vector<compile_message> messages;
};

class state {
public:
    std::unique_ptr<std::byte[]> stack;
    std::size_t stacksize;
    std::vector<module> modules;

    load_result load(std::istream& source_code);

    int execute(std::int16_t mod_idx, std::int16_t func_addr, int ret_size);

    std::int16_t get_entry_point(std::int16_t mod_idx) const;
};

}
