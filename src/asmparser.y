/// Parser grammar

%define api.prefix {moonflowerasm}
%define parse.error verbose

// Defines a reentrant parser and lexer.
// The first parse-param should be a `yyscan_t`,
// but due to a circular dependency, this seems impossible.
%define api.pure full
%lex-param {yyscan_t scanner}
%parse-param {void* scanner} {moonflower::asm_context& context}

%locations

%code requires {
    #include "asm_context.hpp"
    #include "location.hpp"
    #include <string>

    using MOONFLOWERASMLTYPE = moonflower::location;
    #define MOONFLOWERASMLTYPE_IS_DECLARED 1
    #define MOONFLOWERASMLTYPE_IS_TRIVIAL 1
}

%union {
    int i;
    float f;
    std::string* string;
}

%code {
    #include "asmlexer.hpp"

    using namespace moonflower;

    static void yyerror(YYLTYPE* loc, void* scanner, asm_context& context, const char *s) {
        context.messages.emplace_back(s, *loc);
    }
}

%destructor { delete $$; } <string>

%token TK_M_ENTRY TK_M_EXPORT
%token TK_D_ENTRY TK_D_ICONST TK_D_FCONST TK_D_FUNCDEF
%token TK_TERMINATE
%token TK_ISETC TK_FSETC
%token TK_IADD TK_IMUL TK_ISUB TK_IDIV
%token TK_FADD TK_FMUL TK_FSUB TK_FDIV
%token TK_JMP TK_CALL TK_RET

%token <i> TK_INT
%token <f> TK_FLOAT
%token <string> TK_ID

%start chunk

%%

chunk: statseq;

statseq: stat | statseq stat;

stat: TK_ID[id] ':' { context.add_label(*$id, @$); }
    | TK_M_ENTRY { context.set_entry(@$); }
    | TK_D_ICONST TK_INT[i] { context.emit(value($i)); }
    | TK_D_FCONST TK_FLOAT[f] { context.emit(value($f)); }
    | TK_D_FUNCDEF TK_INT[coff] { context.emit(value(function_def($coff))); }
    | TK_TERMINATE TK_INT[a] { context.emit(instruction(TERMINATE, $a)); }
    | TK_ISETC TK_INT[a] TK_INT[b] { context.emit(instruction(ISETC, $a, $b, 0)); }
    | TK_IADD TK_INT[a] TK_INT[b] TK_INT[c] { context.emit(instruction(IADD, $a, $b, $c)); }
    | TK_RET { context.emit(instruction(RET)); }
    ;

%%
