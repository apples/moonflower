#pragma once

#include "translation.hpp"

#include <string>

namespace moonflower {

class state;

translation compile(state& S, const std::string& name, std::istream& source);

}
