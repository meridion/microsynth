/* microsynth application entrypoint */

/* C-stdlib */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Readline */
#include <readline/readline.h>
#include <readline/history.h>

/* microsynth stuff */
#include "sampleclock.h"
#include "synth.h"

int main()
{
    char *line;

    /* Setup synthesizer */
    msynth_init();

    line = readline("msynth> ");

    while (line) {
        if (!strcmp(line, "quit"))
            break;
        add_history(line);
        free(line);
        line = readline("msynth> ");
    }

    free(line);

    /* Shutdown synthesizer */
    msynth_shutdown();

    return 0;
}

