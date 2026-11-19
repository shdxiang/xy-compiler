%code requires {
    #include "ast.h"
}

%code {
    #include <cerrno>
    #include <cstdio>
    #include <cstdlib>
    NBlock *programBlock; /* the top level root node of our final AST */

    extern int yylex();
    void yyerror(const char *s);

    /* Report an error found by a grammar action */
    static void actionError(int line, const char *s) {
        std::fprintf(stderr, "line %d: error: %s\n", line, s);
    }

    /* Function parameters are parsed like call arguments (see func_decl);
       this checks that every argument is a plain identifier. */
    static bool toParameters(ExpressionList &args, VariableList &params) {
        for (auto &arg : args) {
            if (!dynamic_cast<NIdentifier *>(arg.get())) {
                return false;
            }
        }
        for (auto &arg : args) {
            params.emplace_back(static_cast<NIdentifier *>(arg.release()));
        }
        return true;
    }
}

%define parse.error verbose
%define parse.trace

/* Represents the many different ways we can access our data */
%union {
    NBlock *block;
    NExpression *expr;
    NStatement *stmt;
    NIdentifier *ident;
    VariableList *varvec;
    ExpressionList *exprvec;
    std::string *string;
    int token;
}

/* Define our terminal symbols (tokens). This should
   match our lexical.l lex file. We also define the node type
   they represent.
 */
%token <string> TIDENTIFIER "identifier" TINTEGER "integer"
%token <token> TCEQ "==" TCNE "!=" TEQUAL "="
%token <token> TLPAREN "(" TRPAREN ")" TLBRACE "{" TRBRACE "}" TCOMMA ","
%token <token> TPLUS "+" TMINUS "-" TMUL "*" TDIV "/"
%token <token> TRETURN "return" TEXTERN "extern"
%token <token> TINVALID "invalid character"

/* Define the type of node our nonterminal symbols represent.
   The types refer to the %union declaration above. Ex: when
   we call an ident (defined by union type ident) we are really
   calling an (NIdentifier*). It makes the compiler happy.
 */
%type <ident> ident
%type <expr> numeric expr
%type <varvec> func_decl_args
%type <exprvec> call_args
%type <block> stmts block
%type <stmt> stmt func_decl extern_decl

/* Free semantic values discarded during error recovery */
%destructor { delete $$; } <block> <expr> <stmt> <ident> <varvec> <exprvec> <string>

/* Operator precedence, from lowest to highest */
%precedence TEQUAL
%nonassoc TCEQ TCNE
%left TPLUS TMINUS
%left TMUL TDIV
/* An identifier followed by "(" is always a call, never a variable
   followed by a parenthesized expression statement */
%precedence VARIABLE
%precedence TLPAREN

%start program

%%

program:
  stmts { programBlock = $1; }
;

stmts:
  %empty { $$ = new NBlock(); }
| stmts stmt { $1->statements.emplace_back($2); $$ = $1; }
;

stmt:
  func_decl
| extern_decl
| expr { $$ = new NExpressionStatement($1); }
| TRETURN expr { $$ = new NReturnStatement($2); }
;

block:
  TLBRACE stmts TRBRACE { $$ = $2; }
;

extern_decl:
  TEXTERN ident TLPAREN func_decl_args TRPAREN { $$ = new NExternDeclaration($2, std::move(*$4)); delete $4; }
;

/* The parameter list is parsed as call_args so the parser does not have to
   decide between a declaration and a call before it sees the "{" */
func_decl:
  ident TLPAREN call_args TRPAREN block {
      VariableList params;
      if (!toParameters(*$3, params)) {
          actionError($1->line, "function parameters must be identifiers");
          delete $1; delete $3; delete $5;
          YYERROR;
      }
      $$ = new NFunctionDeclaration($1, std::move(params), $5);
      delete $3;
  }
;

func_decl_args:
  %empty { $$ = new VariableList(); }
| ident { $$ = new VariableList(); $$->emplace_back($1); }
| func_decl_args TCOMMA ident { $1->emplace_back($3); $$ = $1; }
;

ident:
  TIDENTIFIER { $$ = new NIdentifier(*$1); delete $1; }
;

numeric:
  TINTEGER {
      errno = 0;
      long long value = std::strtoll($1->c_str(), nullptr, 10);
      delete $1;
      if (errno == ERANGE) {
          actionError(yylineno, "integer literal out of range");
          YYERROR;
      }
      $$ = new NInteger(value);
  }
;

expr:
  ident TEQUAL expr { $$ = new NAssignment($1, $3); }
| ident TLPAREN call_args TRPAREN { $$ = new NMethodCall($1, std::move(*$3)); delete $3; }
| ident %prec VARIABLE { $$ = $1; }
| numeric
| expr TMUL expr { $$ = new NBinaryOperator($1, $2, $3); }
| expr TDIV expr { $$ = new NBinaryOperator($1, $2, $3); }
| expr TPLUS expr { $$ = new NBinaryOperator($1, $2, $3); }
| expr TMINUS expr { $$ = new NBinaryOperator($1, $2, $3); }
| expr TCEQ expr { $$ = new NBinaryOperator($1, $2, $3); }
| expr TCNE expr { $$ = new NBinaryOperator($1, $2, $3); }
| TLPAREN expr TRPAREN { $$ = $2; }
;

call_args:
  %empty { $$ = new ExpressionList(); }
| expr { $$ = new ExpressionList(); $$->emplace_back($1); }
| call_args TCOMMA expr { $1->emplace_back($3); $$ = $1; }
;

%%

extern char *yytext;

void yyerror(const char *s) {
    if (yychar == YYEOF || yychar == YYEMPTY) {
        std::fprintf(stderr, "line %d: error: %s\n", yylineno, s);
    } else {
        std::fprintf(stderr, "line %d: error: %s, near '%s'\n", yylineno, s, yytext);
    }
}
