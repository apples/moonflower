#pragma once

#include "types.hpp"
#include "compile_message.hpp"

#include <vector>

namespace moonflower {

enum class result {
    SUCCESS,
    FAIL,
};

struct translation {
    result r;
    module m;
    std::vector<compile_message> messages;
    int entry_point;
};

translation translate(const char* source, std::size_t len);

}
