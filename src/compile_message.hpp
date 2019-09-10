#pragma once

#include "location.hpp"

#include <string>
#include <utility>
#include <iostream>
#include <vector>

namespace moonflower {

struct compile_message {
    enum Severity {
        ERROR,
        WARNING
    };

    Severity severity = ERROR;
    std::string message;
    location loc;

    compile_message() = default;
    compile_message(std::string m, location l) : message(std::move(m)), loc(l) {}
    compile_message(Severity s, std::string m, location l) : severity(s), message(std::move(m)), loc(l) {}
};

inline std::ostream& operator<<(std::ostream& out, const compile_message& error) {
    switch (error.severity) {
        case compile_message::Severity::ERROR:
            out << "Error: ";
            break;
        case compile_message::Severity::WARNING:
            out << "Warning: ";
            break;
    }

    out << error.loc.begin.line << "," << error.loc.begin.column << ": ";
    out << error.message << "\n";

    return out;
}

inline std::ostream& operator<<(std::ostream& out, const std::vector<compile_message>& errors) {
    for (const auto& error : errors) {
        out << error;
    }
    return out;
}

}
