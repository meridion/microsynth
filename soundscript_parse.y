/* Microsynth soundscript grammar */

%{
#include <glib.h>
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
    struct arg_list {
        msynth_modifier argv[2];
        int argc;
    } args;
}

%token <number> NUM
%token <name> IDENT
%token EOL GARBAGE
%type <mod> number expr_deep expr_mul expr_add statement
%type <args> any_args require_args

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

expr_deep: IDENT '(' any_args ')' {
            switch ($3.argc) {
                case 0:
                    if (!ssb_can_func0($1)) {
                        fprintf(stderr, "No such function: '%s'\n", $1);
                        YYERROR;
                    }
                    $$ = ssb_func0($1);
                    break;

                case 1:
                    if (!ssb_can_func1($1)) {
                        fprintf(stderr, "No such function: '%s'\n", $1);
                        YYERROR;
                    }
                    $$ = ssb_func1($1, $3.argv[0]);
                    break;

                case 2:
                    if (!ssb_can_func2($1)) {
                        fprintf(stderr, "No such function: '%s'\n", $1);
                        YYERROR;
                    }
                    $$ = ssb_func2($1, $3.argv[0], $3.argv[1]);
                    break;

                default:
                    fprintf(stderr,
                        "More than 3 arguments are not supported\n");
                    YYERROR;

            }
        }

    | '(' expr_add ')' { $$ = $2; }
    | number { $$ = $1; }
    | IDENT {
            fprintf(stderr, "No such variable: '%s'\n", $1);
            YYERROR;
        }
    ;

any_args: {
            $$.argc = 0;
        }
    | require_args {
            $$ = $1;
        }
    ;

require_args: expr_add {
            $$.argc = 1;
            $$.argv[0] = $1;
        }

    | require_args ',' expr_add {
            $$ = $1;
            if ($$.argc == 2) {
                fprintf(stderr,
                    "More than 3 arguments are not supported\n");
                YYERROR;
            }
            $$.argv[$$.argc++] = $3;
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

