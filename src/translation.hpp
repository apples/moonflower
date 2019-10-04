#pragma once

#include "types.hpp"
#include "compile_message.hpp"

namespace moonflower {

enum class result {
    SUCCESS,
    FAIL,
};

struct translation {
    result r;
    module m;
    std::vector<compile_message> messages;
};

}
