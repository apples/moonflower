%top{
#include "asmparser.hpp"
%}

%option bison-complete
%option bison-cc-namespace=moonflowerasm
%option bison-cc-parser=parser
%option bison-locations
%option namespace=moonflowerasm
%option lexer=lexer

%%

[ \t\r\n]                           // whitespace
[;].*\n                             // comment
":"                                 |
"@"                                 return parser::symbol_type(text()[0], location());
"%entry"                            return parser::make_M_ENTRY(location());
"%export"                           return parser::make_M_EXPORT(location());
"%import"                           return parser::make_M_IMPORT(location());
".iconst"                           return parser::make_D_ICONST(location());
".fconst"                           return parser::make_D_FCONST(location());
"terminate"                         return parser::make_TERMINATE(location());
"isetc"                             return parser::make_ISETC(location());
"fsetc"                             return parser::make_FSETC(location());
"iadd"                              return parser::make_IADD(location());
"imul"                              return parser::make_IMUL(location());
"isub"                              return parser::make_ISUB(location());
"idiv"                              return parser::make_IDIV(location());
"fadd"                              return parser::make_FADD(location());
"fmul"                              return parser::make_FMUL(location());
"fsub"                              return parser::make_FSUB(location());
"fdiv"                              return parser::make_FDIV(location());
"jmp"                               return parser::make_JMP(location());
"call"                              return parser::make_CALL(location());
"ret"                               return parser::make_RET(location());
-?[0-9]+                            return parser::make_INT(std::strtol(text(), nullptr, 10), location());
-?[0-9]+(\.[0-9]+([eE]-?[0-9]+)?)?  return parser::make_FLOAT(std::strtof(text(), nullptr), location());
[.a-zA-Z_][.a-zA-Z0-9_]*            return parser::make_ID(text(), location());
<<EOF>>                             return parser::make_EOF(location());
.                                   throw parser::syntax_error(location(), "Unknown token.");

%%
