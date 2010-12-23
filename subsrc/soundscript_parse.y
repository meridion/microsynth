/* Microsynth soundscript grammar */

%{
#include "soundscript_lex.h"
#include "soundscript_parse.h"

void yyerror(const char *s);

%}

%token HZ NUM IDENT EOL

%%

script:
    | script line
    ;

line: statement EOL
    ;

statement:
    | expr_add
    | assignment
    ;

assignment: IDENT '=' assignment
    | IDENT '=' expr_add
    ;

expr_add: expr_mul
    | expr_mul '+' expr_add
    | expr_mul '-' expr_add
    ;

expr_mul: expr_deep
    | expr_deep '*' expr_mul
    | expr_deep '/' expr_mul
    ;

expr_deep: IDENT '(' expr_add ')'
    | '(' expr_add ')'
    | number
    | IDENT
    ;

number: '-' NUM
    | NUM
    ;

%%

void yyerror(const char *s)
{
    fprintf(stderr, "%s\n", s);

    return;
}

