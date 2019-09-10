/// Parser grammar
%require "3.3"
%language "c++"

%define api.namespace {moonflowerasm}
%define api.parser.class {parser}
%define api.value.type variant
%define api.token.constructor
%define parse.trace

%defines
%locations

%code requires {
    #include "asm_context.hpp"
    #include "location.hh"
    #include <string>

    namespace moonflowerasm {
        class lexer;
    }
}

%parse-param {moonflowerasm::lexer& lexer} {moonflower::asm_context& context}

%code {
    #include "asmlexer.hpp"

    #undef yylex
    #define yylex lexer.lex

    using namespace moonflower;
}

%define api.token.prefix {TK_}

%token M_ENTRY M_EXPORT M_IMPORT
%token D_ENTRY D_ICONST D_FCONST D_FUNCDEF
%token TERMINATE
%token ISETC FSETC
%token IADD IMUL ISUB IDIV
%token FADD FMUL FSUB FDIV
%token JMP CALL RET

%token <int> INT
%token <float> FLOAT
%token <std::string> ID

%token EOF 0

%start chunk

%%

chunk: statseq EOF;

statseq: stat | statseq stat;

stat: ID[id] ':' { context.add_label($id, @$); }

    | M_ENTRY { context.set_entry(@$); }
    | M_EXPORT ID[name] { context.add_export($name, @$); }
    | M_IMPORT ID[modname] { context.begin_import($modname); } '(' imports ')' { context.end_import(@$); }

    | D_ICONST INT[i] { context.emit(value($i)); }
    | D_FCONST FLOAT[f] { context.emit(value($f)); }

    | TERMINATE INT[a] { context.emit(instruction(TERMINATE, $a)); }

    | ISETC INT[a] INT[d] { context.emit(instruction(ISETC, $a, $d)); }
    | ISETC INT[a] ID[id] {
        auto addr = context.get_label($id);
        if (addr == -1) context.await_label($id);
        context.emit(instruction(ISETC, $a, addr));
    }

    | IADD INT[a] INT[b] INT[c] { context.emit(instruction(IADD, $a, $b, $c)); }

    | RET { context.emit(instruction(RET)); }
    ;

imports: ID[name] { context.import($name); }
       | imports ID[name] { context.import($name); }
       ;

%%

void moonflowerasm::parser::error(const moonflowerasm::location& loc, const std::string& msg) {
    throw syntax_error(loc, msg);
}
