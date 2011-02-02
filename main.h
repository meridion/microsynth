/* Global synthesizer configuration */

#define MSYNTH_VERSION "v0.1.1-devel"

extern struct _msynth_config {
    int exit_code;
    int verbose;

    /* Samplerate settings */
    int
        srate,
        resample;

    /* ALSA device */
    char *device_name;

    /* Buffer and period settings */
    unsigned int
        buffer_time,
        period_time;
} config;

