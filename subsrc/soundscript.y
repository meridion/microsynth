/* Microsynth soundscript grammar */

%%

script:
    | script line
    ;

line: statement EOL
    ;

statement:
    | assignment

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
    | hertz
    | number
    ;

hertz: number HZ
    ;

number: '-' NUM
    | NUM
    ;

