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
#include "soundscript.h"

int main()
{
    char *line;

    /* Setup synthesizer */
    msynth_init();
    soundscript_init();

    line = readline("msynth> ");

    while (line) {
        if (!strcmp(line, "quit"))
            break;
        add_history(line);
        soundscript_parse(line);
        free(line);
        line = readline("msynth> ");
    }

    if (line)
        free(line);
    else
        puts("");

    /* Shutdown synthesizer */
    soundscript_shutdown();
    msynth_shutdown();

    return 0;
}

