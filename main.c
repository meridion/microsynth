/* microsynth application entrypoint */

/* POSIX */
#include <unistd.h>

/* C-stdlib */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Readline */
#include <readline/readline.h>
#include <readline/history.h>

/* GLib */
#include <glib.h>

/* microsynth stuff */
#include "main.h"
#include "sampleclock.h"
#include "synth.h"
#include "soundscript.h"

static int msynth_parse_args(int argc, char *argv[]);
struct _msynth_config config;

int main(int argc, char *argv[])
{
    char *line;

    srandom(20091989);

    /* Handle commandline */
    if (msynth_parse_args(argc, argv))
        return config.exit_code;

    /* Setup synthesizer */
    msynth_init();
    soundscript_init();
    puts("microsynth " MSYNTH_VERSION);

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

    return config.exit_code;
}

static int msynth_parse_args(int argc, char *argv[])
{
    int arg;
    config.exit_code = EXIT_SUCCESS;
    config.srate = -1;
    config.resample = 0;
    config.buffer_time = config.period_time = -1;
    config.device_name = "default";
    config.verbose = 0;

    while ((arg = getopt(argc, argv, "s:rvb:p:d:h")) != -1) {
        switch (arg) {
            case 's':
                config.srate = atoi(optarg);
                break;

            case 'r':
                config.resample = 1;
                break;

            case 'v':
                config.verbose = 1;
                break;

            case 'b':
                config.buffer_time = atoi(optarg);
                break;

            case 'p':
                config.period_time = atoi(optarg);
                break;

            case 'd':
                config.device_name = optarg;
                break;

            case 'h':
                printf("Usage %s:\n"
                    "    -s Set samplerate (usually 48000 or 44100)\n"
                    "    -r Enable software resampling\n"
                    "    -b Set buffer time in microseconds\n"
                    "       This is used to determine the amount of memory\n"
                    "       used to buffer samples to. (e.g. 500000)\n"
                    "    -p Set period time in microseconds.\n"
                    "       The period is the amount of samples written at\n"
                    "       once to the soundcard. Is usually smaller than"
                    " half\n"
                    "       the buffer time. (e.g. 250000)\n", argv[0]);
                printf(
                    "\n"
                    "    By default the buffer time will be minimized to\n"
                    "    to reduce latency, and the period time will be\n"
                    "    maximized to reduce synthesis overhead.\n"
                    "\n"
                    "    -d Set ALSA device name (usually default or hw:0,0)\n"
                    "    -h Show this help.\n");
                return 1;

            case '?':
            default:
                config.exit_code = EXIT_FAILURE;
                return 1;
        }
    }

    /* Verify config is valid */
    if (config.resample && config.srate == -1) {
        printf("When enabling software resampling, you are required to\n"
            "also specify a samplerate using -s.\n");
        config.exit_code = EXIT_FAILURE;
        return 1;
    }

    return 0;
}

