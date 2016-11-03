%{
    #include "ast.h"
    #include <cstdio>
    #include <cstdlib>
    NBlock *programBlock; /* the top level root node of our final AST */

    extern int yylex();
    void yyerror(const char *s) { std::printf("Error: %s\n", s);std::exit(1); }
%}

/* Represents the many different ways we can access our data */
%union {
    Node *node;
    NBlock *block;
    NExpression *expr;
    NStatement *stmt;
    NIdentifier *ident;
    std::vector<NIdentifier*> *varvec;
    std::vector<NExpression*> *exprvec;
    std::string *string;
    int token;
}

/* Define our terminal symbols (tokens). This should
   match our tokens.l lex file. We also define the node type
   they represent.
 */
%token <string> TIDENTIFIER TINTEGER
%token <token> TCEQ TCNE TEQUAL
%token <token> TLPAREN TRPAREN TLBRACE TRBRACE TCOMMA
%token <token> TPLUS TMINUS TMUL TDIV
%token <token> TRETURN TEXTERN TIF TELSE

/* Define the type of node our nonterminal symbols represent.
   The types refer to the %union declaration above. Ex: when
   we call an ident (defined by union type ident) we are really
   calling an (NIdentifier*). It makes the compiler happy.
 */
%type <ident> ident
%type <expr> numeric expr 
%type <varvec> func_decl_args
%type <exprvec> call_args
%type <block> program stmts block
%type <stmt> stmt func_decl extern_decl condition_stmt
%type <token> comparison

/* Operator precedence for mathematical operators */
%left TPLUS TMINUS
%left TMUL TDIV

%start program

%%

program:
  stmts { programBlock = $1; }
        ;
        
stmts:
  stmt { $$ = new NBlock(); $$->statements.push_back($1); }
| stmts stmt { $1->statements.push_back($2); }
;

stmt:
  func_decl
| extern_decl
| expr { $$ = new NExpressionStatement(*$1); }
| TRETURN expr { $$ = new NReturnStatement(*$2); }
| condition_stmt
;

condition_stmt:
  TIF TLPAREN expr TRPAREN block TELSE block  { $$ = new NConditionStatement(*$3, *$5, *$7); }
; 

block:
  TLBRACE stmts TRBRACE { $$ = $2; }
| TLBRACE TRBRACE { $$ = new NBlock(); }
;

extern_decl:
  TEXTERN ident TLPAREN func_decl_args TRPAREN { $$ = new NExternDeclaration(*$2, *$4); delete $4; }
;

func_decl:
  ident TLPAREN func_decl_args TRPAREN block { $$ = new NFunctionDeclaration(*$1, *$3, *$5); delete $3; }
;
    
func_decl_args:
  { $$ = new VariableList(); }
| ident { $$ = new VariableList(); $$->push_back($1); }
| func_decl_args TCOMMA ident { $1->push_back($3); }
;

ident:
  TIDENTIFIER { $$ = new NIdentifier(*$1); delete $1; }
;

numeric:
  TINTEGER { $$ = new NInteger(atol($1->c_str())); delete $1; }
;

expr:
  ident TEQUAL expr { $$ = new NAssignment(*$1, *$3); }
| ident TLPAREN call_args TRPAREN { $$ = new NMethodCall(*$1, *$3); delete $3; }
| ident { $$ = $1; }
| numeric
| expr TMUL expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
| expr TDIV expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
| expr TPLUS expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
| expr TMINUS expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
| expr comparison expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
| TLPAREN expr TRPAREN { $$ = $2; }
;
    
call_args:
  { $$ = new ExpressionList(); }
| expr { $$ = new ExpressionList(); $$->push_back($1); }
| call_args TCOMMA expr { $1->push_back($3); }
;

comparison:
  TCEQ
| TCNE
;

%%
