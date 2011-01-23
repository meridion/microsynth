/* Microsynth soundscript grammar */

%{
#include "sampleclock.h"
#include "synth.h"
#include "soundscript_lex.h"
#include "soundscript_parse.h"
#include "soundscript.h"
#include "transform.h"

void yyerror(const char *s);

%}

%union {
    float number;
    char *name;
    msynth_modifier mod;
}

%token <number> NUM
%token <name> IDENT
%token EOL GARBAGE
%type <mod> number expr_deep expr_mul expr_add statement

%%

script:
    | script line
    ;

line: statement EOL {
            /* GC should not delete this */
            soundscript_mark_use($1);

            /* Change synthesizer signal */
            synth_replace($1);
        }
    ;

statement:
    | expr_add {
            $$ = $1;
        }

    | assignment { 
            puts("Assignments are currently discarded, sorry");
            $$ = NULL;
        }
    ;

assignment: IDENT '=' assignment
    | IDENT '=' expr_add
    ;

expr_add: expr_mul
    | expr_mul '+' expr_add { $$ = ssb_add($1, $3); }
    | expr_mul '-' expr_add { $$ = ssb_sub($1, $3); }
    ;

expr_mul: expr_deep
    | expr_deep '*' expr_mul { $$ = ssb_mul($1, $3); }
    | expr_deep '/' expr_mul { $$ = ssb_div($1, $3); }
    ;

expr_deep: IDENT '(' expr_add ')' {
            if (!ssb_can_func1($1)) {
                fprintf(stderr, "No such function: '%s'\n", $1);
                YYERROR;
            }

            $$ = ssb_func1($1, $3);
        }

    | '(' expr_add ')' { $$ = $2; }
    | number { $$ = $1; }
    | IDENT {
            fprintf(stderr, "No such variable: '%s'\n", $1);
            YYERROR;
        }
    ;

number: '-' NUM { $$ = ssb_number(-$2); }
    | NUM { $$ = ssb_number($1); }
    ;

%%

void yyerror(const char *s)
{
    fprintf(stderr, "%s\n", s);

    return;
}

