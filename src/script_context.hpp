#pragma once

#include "asm_context.hpp"
#include "types.hpp"
#include "compile_message.hpp"
#include "location.hpp"

#include <cassert>
#include <charconv>
#include <unordered_map>
#include <vector>

namespace moonflower {

struct script_context {
    struct stack_obj {
        int loc;
        type type;
    };

    struct constant {
        int loc;
        type type;
        union {
            int i;
        } val;
    };

    struct function_context {
        std::vector<constant> constants;
        int next_constant = 0;
    };

    asm_context assembler;
    function_context cur_func;

    int get_or_make_const_int(int val) {
        for (const auto& c : cur_func.constants) {
            if (c.type.kind == type::INTEGER && c.val.i == val) {
                return c.loc;
            }
        }

        constant c;
        c.loc = cur_func.next_constant;
        c.type.kind = type::INTEGER;
        c.val.i = val;

        cur_func.constants.push_back(c);
        cur_func.next_constant += 1;
        
        return c.loc;
    }

    stack_obj load_const_int(int val) {
        const auto c_loc = get_or_make_const_int(val);
        
    }
};

}
