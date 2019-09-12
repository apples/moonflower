/// Parser grammar
%require "3.3"
%language "c++"

%define api.namespace {moonflower_script}
%define api.parser.class {parser}
%define api.value.type variant
%define api.token.constructor
%define parse.trace

%defines
%locations
%define api.location.type {moonfloar::location}

%code requires {
    #include "script_context.hpp"
    #include "location.hpp"
    #include <string>

    namespace moonflower_script {
        class lexer;
    }
}

%parse-param {moonflower_script::lexer& lexer} {moonflower::script_context& context}

%code {
    #include "asmlexer.hpp"

    #undef yylex
    #define yylex lexer.lex

    using namespace moonflower;
}

%define api.token.prefix {TK_}

%token IMPORT EXPORT AS
%token FUNC RETURN
%token VAR

%token <std::string> IDENTIFIER
%token <int> INTEGER

%left '+' '-'
%left '*' '/'

%token EOF 0

%type <int> literal expr binaryop

%start chunk

%%

chunk: imports topblock EOF;

imports: %empty
       | import
       | imports import
       ;

import: IMPORT modname AS IDENTIFIER;

topblock: %empty
        | topstatement
        | topblock topstatement
        ;

topstatement: funcdecl
            | EXPORT funcdecl
            ;

funcdecl: FUNC IDENTIFIER funcbody;

funcbody: '(' funcparams ')' ':' type '{' block '}';

funcparams: %empty
          | paramseq
          ;

paramseq: IDENTIFIER ':' type
        | paramseq ',' IDENTIFIER ':' type
        ;

block: %empty
     | block statement
     | block retstat
     ;

retstat: RETURN
       | RETURN expr
       ;

type: IDENTIFIER;

statement: assignment
         | vardecl
         ;

vardecl: VAR IDENTIFIER '=' expr {
           context.declare($IDENTIFIER);
       }
       ;

expr: literal { $$ = $1; }
    | binaryop { $$ = $1; }
    | IDENTIFIER { $$ = context.lookup($IDENTIFIER); }
    ;

literal: INTEGER { $$ = context.load_const_int($INTEGER); }
       ;

binaryop: expr[lhs] '+' expr[rhs] { $$ = context.binop(binop::ADD, $lhs, $rhs); }
        | expr[lhs] '-' expr[rhs] { $$ = context.binop(binop::SUB, $lhs, $rhs); }
        | expr[lhs] '*' expr[rhs] { $$ = context.binop(binop::MUL, $lhs, $rhs); }
        | expr[lhs] '/' expr[rhs] { $$ = context.binop(binop::DIV, $lhs, $rhs); }
        ;

%%

void moonflowerasm::parser::error(const moonflowerasm::location& loc, const std::string& msg) {
    throw syntax_error(loc, msg);
}

/*
chunk ::= importstat* topblock
importstat ::= "import" MODNAME [ "as" IDENT ]
topblock ::= { topstatement }
topstatement ::=
statement |
"export" funcdecl |
"export" typedecl
block ::= { statement } [ retstat ]
statement ::=
    assignment |
    functioncall |
    "{" block "}" |
    whilestat |
    forstat |
    vardecl |
    funcdecl |
    typedecl
retstat ::= "return" [ expr { "," expr } ]
assignment ::= lvalue { "," lvalue } = expr { "," expr }
functioncall ::= prefixexpr "(" [ expr { "," expr } ] ")"
whilestat ::= "while" expr "{" block "}"
forstat ::= "for" vardeclname "in" expr "{" block "}"
vardecl ::= "var" IDENT { "," IDENT } "=" expr { "," expr }
funcdecl ::= "func" IDENT funcbody
expr ::=
    prefixexpr |
    funcdef |
    newexpr |
    caseexpr |
    literal
literal ::=
    BOOL |
    NUMBER |
    STRING |
    arrayliteral |
    tableliteral |
    structliteral |
    vecliteral
lvalue ::=
IDENT |
lvalue "." IDENT |
lvalue "[" expr "]"
prefixexpr ::=
lvalue |
functioncall |
"(" expr ")"
funcparam ::= IDENT [ ":" type ]
arrayliteral ::= "[" [ expr { "," expr } ] "]"
tableliteral ::= "{" [ expr "=" expr { "," expr "=" expr } [ "," ] ] "}"
structliteral ::= "{" "." IDENT "=" expr { "." IDENT "=" expr } [ "," ] "}"
vecliteral ::= "<" expr [ "," expr [ "," expr [ "," expr ] ] ] ">"
funcdef ::=
"func" funcbody |
"\" "(" funcparams ")" "=>" expr
funcbody ::= "(" funcparams ")" [ ":" type { "," type } ] "{" block "}"
funcparams ::= [ funcparam { "," funcparam } ]
newexpr ::= "new" [ "shared" | "local" ] type [ "(" args ")" ]
type ::=
IDENT |
type "[" "]" |
type "[" type "]" |
"(" funcparams ")" ":" type { "," type } |
type "?"
typedecl ::= "type" IDENT "=" typedef
typedef ::=
type |
"struct" "{" { structfield } "}" |
"enum" "{" { IDENT [ "=" expr ] } "}"
structfield ::=
[ "static" ] IDENT ":" type [ "=" expr ] |
[ "static" | "const" ] funcdecl |
"constructor" funcbody
caseexpr ::=
"case" "{" { caseentry } "}" |
"case" expr "in" "{" { caseentry } "}" |
"case" expr "->" "{" body "}"
caseentry ::=
casematch "->" expr |
casematch "->" "{" block "}"
casematch ::= caseterm { "," caseterm }
caseterm ::=
    "_" |
    IDENT |
    literal |
    "(" expr ")"

*/
