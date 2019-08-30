%option prefix="moonflowerasm"
%option reentrant bison-bridge bison-locations
%option noyywrap nounput noinput
%option yylineno

%top{
#include <string>
#include "asmparser.hpp"
#define YYSTYPE MOONFLOWERASMSTYPE
#define YYLTYPE MOONFLOWERASMLTYPE
}

%{
#define SAVE_TOKEN yylval->string = new std::string(yytext, yyleng)
#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno;
%}

%%

[ \t\r\n]                           ;
[;].*\n                             ;
":"                                 return ':';
"%entry"                            return TK_M_ENTRY;
"%export"                           return TK_M_EXPORT;
".iconst"                           return TK_D_ICONST;
".fconst"                           return TK_D_FCONST;
".funcdef"                          return TK_D_FUNCDEF;
"terminate"                         return TK_TERMINATE;
"isetc"                             return TK_ISETC;
"fsetc"                             return TK_FSETC;
"iadd"                              return TK_IADD;
"imul"                              return TK_IMUL;
"isub"                              return TK_ISUB;
"idiv"                              return TK_IDIV;
"fadd"                              return TK_FADD;
"fmul"                              return TK_FMUL;
"fsub"                              return TK_FSUB;
"fdiv"                              return TK_FDIV;
"jmp"                               return TK_JMP;
"call"                              return TK_CALL;
"ret"                               return TK_RET;
-?[0-9]+                            yylval->i = std::strtol(yytext, nullptr, 10); return TK_INT;
-?[0-9]+(\.[0-9]+([eE]-?[0-9]+)?)?  yylval->f = std::strtof(yytext, nullptr); return TK_FLOAT;
[.a-zA-Z_][.a-zA-Z0-9_]*            SAVE_TOKEN; return TK_ID;
.                                   printf("Unknown token: `%s`\n", yytext); yyterminate();

%%
