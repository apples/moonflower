#pragma once

#include "types.hpp"
#include "state.hpp"

namespace moonflower {

int interp(state& S, std::uint16_t mod_idx, std::uint16_t func_addr, int retc);

}
