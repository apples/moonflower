#pragma once

#include "types.hpp"
#include "state.hpp"
#include "interp_result.hpp"

namespace moonflower {

interp_result interp(state& S, std::uint16_t mod_idx, std::uint16_t func_addr, int retc);

}
