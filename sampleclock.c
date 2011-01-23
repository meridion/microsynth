/* Sampleclock helpers */
#include <math.h>
#include "sampleclock.h"

/* Build a sampleclock from a samplerate and current samples */
struct sampleclock sc_from_samples(int samplerate, int samples)
{
    struct sampleclock sc;

    sc.samplerate = samplerate;
    sc.samples = samples;
    sc.seconds = (float)sc.samples / (float)sc.samplerate;
    sc.cycle = fmod(sc.seconds, 1.0f);

    return sc;
}

/* Rewind the sample clock X samples */
struct sampleclock sc_rewind(struct sampleclock sc, int samples)
{
    return sc_from_samples(sc.samplerate, sc.samples - samples);
}

