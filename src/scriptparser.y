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
%define api.location.type {moonflower::location}

%code requires {
    #include "script_context.hpp"
    #include "location.hpp"
    #include <string>
    #include <cstdint>

    namespace moonflower_script {
        class lexer;
        using location = moonflower::location;
    }
}

%parse-param {moonflower_script::lexer& lexer} {moonflower::script_context& context}

%code {
    #include "scriptlexer.hpp"

    #undef yylex
    #define yylex lexer.lex

    using namespace moonflower;
}

%define api.token.prefix {TK_}

%token IMPORT EXPORT AS
%token FUNC RETURN
%token VAR
%token IF ARROW

%token <std::string> IDENTIFIER
%token <int> INTEGER
%token <bool> BOOLEAN

%left '+' '-'
%left '*' '/'

%token EOF 0

%type <int> expr prefixexpr literal binaryop functioncall
%type <int> arguments argumentseq
%type <int> blockstatseq
%type <std::int16_t> ifcase

%start chunk

%%

chunk: topblock EOF;

topblock: %empty
        | topheader
        | topstatseq
        | topheader topstatseq
        ;

topheader: import
         | topheader import
         ;

import: IMPORT IDENTIFIER[id] '{' { context.begin_import($id); } importnames '}';

importnames: importname
           | importnames importname
           ;

importname: IDENTIFIER[name] { context.import($name); };

topstatseq: topstatement
          | topstatseq topstatement
          ;

topstatement: funcdecl
            ;

funcdecl: FUNC IDENTIFIER[id] { context.begin_func($id); } funcbody { context.end_func(); };

funcbody: '(' funcparams ')' ':' type block;

funcparams: %empty
          | paramseq
          ;

paramseq: paramdecl
        | paramseq ',' paramdecl
        ;

paramdecl: IDENTIFIER[id] ':' type { context.add_param($id, @$); };

block: '{' '}'
     | '{' retstat '}'
     | '{' blockstatseq '}' { context.end_block($2, true, @$); }
     | '{' blockstatseq retstat '}' { context.end_block($2, false, @$); }
     ;

blockstatseq: <int>{ $$ = context.begin_block(@$); } statseq { $$ = $1; }
            ;

statseq: statement
       | statseq statement
       ;

retstat: RETURN { context.emit_return(@$); }
       | RETURN expr { context.emit_return(@$); }
       ;

type: IDENTIFIER;

statement: vardecl
         | functioncall { context.emit_discard(@$); }
         | ifstatement
         ;

ifstatement: IF '{' ifcaseseq '}'
           | IF expr <std::int16_t>{ $$ = context.emit_if(@$); } block { context.set_jmp($2, @$); }
           ;

ifcaseseq: ifcase { context.set_jmp($1, @$); }
         | ifcase ifcaseseq { context.set_jmp($1, @$); }
         | ifcasedefault
         ;

ifcase: expr <std::int16_t>{ $$ = context.emit_if(@$); } block { $$ = context.emit_jmp(@$); context.set_jmp($2, @$); }
      ;

ifcasedefault: '_' block
             ;

/*
switchblock: switchcase
           | switchcase 
           ;

switchcaseseq: switchcase
             | switchcase switchcaseseq { context.set_jmp($1, @$); $$ = $2; }
             ;

switchcase: expr <std::int16_t>{ $$ = context.emit_if(@$); } ARROW '{' block '}' { $$ = context.emit_jmp(@$); context.set_jmp($2, @$); }
          | '_' ARROW '{' block '}' { $$ = context.emit_jmp(@$); }
          ;

switchcasedefault: '_' ARROW '{' block '}'
                 ;

ifstatementfull: ifstatement { context.set_jmp($1, @$); }
               | ifstatement <std::int16_t>{ $$ = context.emit_jmp(@$); context.set_jmp($1, @$); } elsestatement { context.set_jmp($2, @$); }
               ;

ifstatement: IF expr <std::int16_t>{ $$ = context.emit_if(@$); } '{' block '}' { $$ = $3; }
           ;

elsestatement: ELSE '{' block '}'
             | ELSE ifstatementfull
             ;
*/

vardecl: VAR IDENTIFIER '=' expr { context.emit_vardecl($IDENTIFIER, @$); }
       ;

expr: prefixexpr { $$ = $1; }
    | binaryop { $$ = $1; }
    ;

prefixexpr: literal { $$ = $1; }
          | functioncall { $$ = $1; }
          | IDENTIFIER { $$ = context.expr_id($IDENTIFIER, @$); }
          ;

literal: INTEGER { $$ = context.expr_const_int($INTEGER); }
       | BOOLEAN { $$ = context.expr_const_bool($BOOLEAN); }
       ;

binaryop: expr[lhs] '+' expr[rhs] { $$ = context.expr_binop(binop::ADD, $lhs, $rhs, @$); }
        | expr[lhs] '-' expr[rhs] { $$ = context.expr_binop(binop::SUB, $lhs, $rhs, @$); }
        | expr[lhs] '*' expr[rhs] { $$ = context.expr_binop(binop::MUL, $lhs, $rhs, @$); }
        | expr[lhs] '/' expr[rhs] { $$ = context.expr_binop(binop::DIV, $lhs, $rhs, @$); }
        ;

functioncall: prefixexpr '(' arguments ')' { $$ = context.expr_call($arguments, @$); };

arguments: %empty { $$ = 0; }
         | argumentseq { $$ = $1; }
         ;

argumentseq: expr { $$ = 1; }
           | argumentseq[sum] ',' expr { $$ = $sum + 1; }
           ;

%%

void moonflower_script::parser::error(const moonflower_script::location& loc, const std::string& msg) {
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
