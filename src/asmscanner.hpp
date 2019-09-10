#pragma once

#if ! defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

#include "asmparser.hpp"

namespace moonflowerasm {

class scanner : public yyFlexLexer {
public:
    virtual int yylex(parser::semantic_type* const lval, parser::location_type* location);

private:
    parser::semantic_type* yylval;
    parser::location_type* location;
};

}
