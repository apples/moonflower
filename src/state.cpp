#include "state.hpp"

#include "compile.hpp"
#include "interp.hpp"

namespace moonflower {

load_result state::load(std::istream& source_code) {
    auto tu = compile(source_code);

    if (tu.r == result::SUCCESS) {
        modules.push_back(std::move(tu.m));

        return {modules.size() - 1, std::move(tu.messages)};
    } else {
        return {std::nullopt, std::move(tu.messages)};
    }
}

int state::execute(std::int16_t mod_idx, std::int16_t func_addr, int ret_size) {
    return interp(*this, mod_idx, func_addr, ret_size);
}

std::int16_t state::get_entry_point(std::int16_t mod_idx) const {
    return modules[mod_idx].entry_point;
}

}
