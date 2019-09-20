%top{
#include "scriptparser.hpp"
%}

%option bison-complete
%option bison-cc-namespace=moonflower_script
%option bison-cc-parser=parser
%option bison-locations
%option namespace=moonflower_script
%option lexer=lexer

%%

[ \t\r\n]                           // whitespace
[/][/].*\n                          // comment
","                                 |
"="                                 |
"("                                 |
")"                                 |
"{"                                 |
"}"                                 |
"+"                                 |
"-"                                 |
"*"                                 |
"/"                                 |
":"                                 return parser::symbol_type(text()[0], location());
"import"                            return parser::make_IMPORT(location());
"export"                            return parser::make_EXPORT(location());
"as"                                return parser::make_AS(location());
"func"                              return parser::make_FUNC(location());
"var"                               return parser::make_VAR(location());
"return"                            return parser::make_RETURN(location());
-?[0-9]+                            return parser::make_INTEGER(std::strtol(text(), nullptr, 10), location());
[a-zA-Z_][a-zA-Z0-9_]*              return parser::make_IDENTIFIER(text(), location());
<<EOF>>                             return parser::make_EOF(location());
.                                   throw parser::syntax_error(location(), "Unknown token.");

%%
