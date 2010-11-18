#define _BSD_SOURCE
#include <math.h>

#include "sampleclock.h"
#include "gen.h"

#ifndef M_PI
#define M_PI 3.14159265358f
#endif

/* Generate sinus wave */
float gen_sin(struct sampleclock sc, float hertz)
{
    return sin(M_PI * ((hertz * 2.0f) * sc.cycle));
}

/* Generate triangle wave */
float gen_triangle(struct sampleclock sc, float hertz)
{
    float cycle = sc.cycle - 0.25f;

    if (cycle < 0.0f)
        cycle += 1.0f;

    if (cycle < 0.5f)
        return -1.0f + 2.0f * cycle;
    return 1.0f - 2.0f * (cycle - 0.5f);
}

/* Generate sawtooth wave (|\|\|\) */
float gen_saw(struct sampleclock sc, float hertz)
{
    return 2.0f * fmodf(sc.cycle * hertz, 1.0f) - 1.0f;
}

/* Generate reverse sawtooth wave (/|/|/|) */
float gen_rsaw(struct sampleclock sc, float hertz)
{
    return -gen_saw(sc, hertz);
}

/* Generate pulse wave (|....|....|....) */
float gen_pulse(struct sampleclock sc, float hertz)
{
    float speriod = (float)sc.samplerate / hertz;

    return (fmodf((float)sc.samples, speriod) <
        fmodf((float)(sc.samples + 1), speriod)) ?
        0.0f : 1.0f;
}

/* Generate square wave */
float gen_square(struct sampleclock sc, float hertz)
{
    return (fmodf(sc.cycle * hertz, 1.0f) < 0.5f) ? 1.0f : -1.0f;
}

