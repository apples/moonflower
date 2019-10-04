#pragma once

#include "types.hpp"
#include "compile_message.hpp"
#include "translation.hpp"

#include <vector>

namespace moonflower {

translation translate(const char* source, std::size_t len);

}
