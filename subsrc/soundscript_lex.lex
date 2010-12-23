
%{
#include "soundscript_parse.h"
%}

ident           [A-Za-z_][0-9A-Za-z_]*

%option noyywrap

%%

{ident}         return IDENT;
0[0-9]          return NUM;
0x[:xdigit:]+   return NUM;
[0-9]+          return NUM;

    /* Operators */
\+              return '+';
-               return '-';
\*              return '*';
\/              return '/';

    /* Parens */
\(              return '(';
\)              return ')';
\[              return '[';
\]              return ']';

    /* Other stuff */
\n              return EOL;
" "             /* Ignore whitespace */

%%

