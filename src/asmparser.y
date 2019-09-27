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

    template <typename T>
    struct downcast_r {
        T val;
        bool overflow;
    };

    template <typename T>
    auto downcast(int i) -> downcast_r<T> {
        using limits = std::numeric_limits<T>;
        auto r = downcast_r<T>{};
        r.val = static_cast<T>(i);
        r.overflow = (i > limits::max() || i < limits::min());
        return r;
    }
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

%type <std::int16_t> int16

%start chunk

%%

chunk: statseq EOF;

statseq: stat | statseq stat;

stat: ID[id] ':' { context.add_label($id, @$); }

    | M_ENTRY { context.set_entry(@$); }
    | M_EXPORT ID[name] { context.add_export($name, @$); }
    | M_IMPORT ID[modname] { context.begin_import($modname); } '(' imports ')' { context.end_import(@$); }

    | TERMINATE int16[a] { context.emit({TERMINATE, $a}); }

    | ISETC int16[a] INT[d] { context.emit({ISETC, $a, $d}); }

    | IADD int16[a] int16[b] int16[c] { context.emit({IADD, $a, {$b, $c}}); }

    | RET { context.emit(instruction{RET}); }
    ;

imports: ID[name] { context.import($name); }
       | imports ID[name] { context.import($name); }
       ;

int16: INT[i] {
         auto [result, overflow] = downcast<std::int16_t>($i);
         if (overflow) {
             context.messages.emplace_back("Integer overflow", @$);
         }
         $$ = result;
     }
     ;

%%

void moonflowerasm::parser::error(const moonflowerasm::location& loc, const std::string& msg) {
    throw syntax_error(loc, msg);
}
