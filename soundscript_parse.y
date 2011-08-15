/* Microsynth soundscript grammar */

/* NOTE:
 *
 * In current config all code below will be executed while holding the synth
 * lock. In other words, there will be absolutely no synthesis while parsing
 * code.
 */
%{
#include <glib.h>
#include <math.h>
#include "sampleclock.h"
#include "synth.h"
#include "soundscript_lex.h"
#include "soundscript_parse.h"
#include "soundscript.h"
#include "transform.h"

void yyerror(const char *s);
static void put_recursion_error() {
    yyerror("error: All recursive variables must be referenced with a delay");
    yyerror("       of at least 1 sample, to break infinite feedback.");
}

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
%token EOL GARBAGE VOLUME
%type <mod> number expr_deep expr_mul expr_add
%type <args> any_args require_args

%%

script:
    | script line
    ;

line: EOL
    /* Normal assignment */
    | IDENT ':' {
            /* In case of an expression like: x := .... x ..
             * with x not yet defined, insert a dummy that allows the recursive
             * definition. The dummy shall be the variable initialized to 0.
             */
            ssv_set_dummy($1);
        }

        /* Handle the rest of the assignment */
        expr_add {

            /* Validate recursive references */
            if (ssv_validate_recursion($4, NULL)) {
                put_recursion_error();
                YYERROR;
            }

            /* Verify there is no recursion in normal variables */
            if (ssv_speculate_cycle($1, $4)) {
                yyerror("Assignment would cause cycle in soundgraph,"
                    " use recursive variables instead");
                YYERROR;
            }

            /* Perform assignment */
            soundscript_mark_use($4);
            ssv_set_var($1, $4);
        }

    /* Recursive assignment */
    | IDENT '=' {
            /* In case of an expression like: x := .... x ..
             * with x not yet defined, insert a dummy that allows the recursive
             * definition. The dummy shall be the variable initialized to 0.
             */
            ssv_set_dummy($1);
        }

        /* Handle the rest of the assignment */
        expr_add {

            /* Validate recursion, using recursive target */
            if (ssv_validate_recursion($4, $1)) {
                put_recursion_error();
                YYERROR;
            }

            /* Perform assignment */
            soundscript_mark_use($4);
            ssv_set_var_recursive($1, $4);
        }
    | expr_add EOL {
            /* Validate recursion */
            if (ssv_validate_recursion($1, NULL)) {
                put_recursion_error();
                YYERROR;
            }

            /* GC should not delete this */
            soundscript_mark_use($1);

            /* Change synthesizer signal */
            synth_replace($1);
        }
    | VOLUME EOL {
            printf("Current volume: %.1f%%\n", synth_get_volume());
        }
    | VOLUME NUM EOL {
            if (0.f <= $2 && $2 <= 100.f)
                synth_set_volume($2);
            else
                puts("Volume must be percentage from 0% to 100%");
        }
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
            if (!ssv_get_var($1)) {
                fprintf(stderr, "No such variable: '%s'\n", $1);
                YYERROR;
            }
            $$ = ssb_variable($1);
        }

    /* Upgraded delay for recursive variables */
    | expr_deep '[' NUM ']' {
            /* Modify recursive delays */
            if ($1->type == MSMT_VARIABLE &&
                    ssv_get_var($1->data.varname)->recursive) {

                /* 0 delay is invalid */
                if (roundf($3) == 0) {
                    fprintf(stderr, "Referencing recursive variable '%s'"
                        " with a delay of 0 samples, invalid.\n",
                        $1->data.varname);
                    YYERROR;

                /* Reference minus 1 */
                } else {
                    $$ = ssb_delay($1, (unsigned int)roundf($3) - 1);
                }
            } else {

                /* Normal delay */
                $$ = ssb_delay($1, (unsigned int)roundf($3));
            }
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

