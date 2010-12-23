/* microsynth - Sound scripting */
#include "soundscript_lex.h"
#include "soundscript_parse.h"
#include "soundscript.h"

void soundscript_parse(char *line)
{

    char *mod_str;
    int len;
    YY_BUFFER_STATE x;

    /* Build string with appended newline */
    len = strlen(line);
    mod_str = malloc(sizeof(char) * len + 3);
    memcpy(mod_str, line, sizeof(char) * len);
    mod_str[len] = '\n';
    mod_str[len + 1] = '\0';
    mod_str[len + 2] = '\0';

    /* Parse string */
    x = yy_scan_buffer(mod_str, len + 3);
    yyparse();
    yy_delete_buffer(x);

    /* Clean up */
    free(mod_str);

    return;
}

