
%{
#include <assert.h>
#include <glib.h>
#include <string.h>
#include "sampleclock.h"
#include "synth.h"
#include "soundscript_parse.h"

static char *do_dup(char *str);

static GSList *str_list;
%}

ident           [A-Za-z_][0-9A-Za-z_]*

%option noyywrap

%%

    /* keywords */
volume          return VOLUME;

    /* Basic types */
{ident}             yylval.name = do_dup(yytext); return IDENT;
[0-9]+\.[0-9]+(e-?[0-9]+)?f?      {
        yylval.number = (float)atof(yytext);
        return NUM;
    }
0[0-9]              {
        yylval.number = (float)strtol(yytext, NULL, 8);
        return NUM;
    }
0x[:xdigit:]+       {
        yylval.number = (float)strtol(yytext, NULL, 16);
        return NUM;
    }
[0-9]+              {
        yylval.number = (float)strtol(yytext, NULL, 10);
        return NUM;
    }

    /* Operators */
\+                  return '+';
-                   return '-';
\*                  return '*';
\/                  return '/';
=                   return '=';

    /* Parens */
\(                  return '(';
\)                  return ')';
\[                  return '[';
\]                  return ']';
,                   return ',';

    /* Other stuff */
\n                  return EOL;
" "             /* Ignore whitespace */
.                   return GARBAGE;

%%

/* dup string and store pointer */
char *do_dup(char *str)
{
    char *r = strdup(str);
    assert(r);

    str_list = g_slist_prepend(str_list, r);
    return r;
}

/* Clear strings dupped while parsing the previous line */
void soundscript_clear_dups()
{
    /* Keep removing while list is not empty (NULL) */
    while (str_list) {
        /* Free string */
        free(g_slist_nth_data(str_list, 0));

        /* Remove head */
        str_list = g_slist_delete_link(str_list, str_list);
    }

    return;
}

