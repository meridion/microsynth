/* microsynth - Basic waveform generation */

/* C-stdlib */
#include <math.h>
#include <stdio.h>

/* microsynth headers */
#include "sampleclock.h"
#include "gen.h"

#ifndef M_PI
#define M_PI 3.14159265358f
#endif

/* Generate sinus wave */
float gen_sin(struct sampleclock sc, float hertz)
{
    return sin(M_PI * 2.0f * hertz * sc.seconds);
}

/* Generate cosine wave */
float gen_cos(struct sampleclock sc, float hertz)
{
    return cos(M_PI * 2.0f * hertz * sc.seconds);
}

/* Generate triangle wave */
float gen_triangle(struct sampleclock sc, float hertz)
{
    float cycle = fmod(sc.seconds * hertz, 1.0f) - 0.25f;

    if (cycle < 0.0f)
        cycle += 1.0f;

    if (cycle < 0.5f)
        return -1.0f + 2.0f * cycle;
    return 1.0f - 2.0f * (cycle - 0.5f);
}

/* Generate sawtooth wave (|\|\|\) */
float gen_saw(struct sampleclock sc, float hertz)
{
    //return 2.0f * fmodf(sc.seconds * hertz, 1.0f) - 1.0f;
    float r = 2.0f * fmodf(sc.seconds * hertz, 1.0f) - 1.0f;

/*
    if (((sc.samples + 1) % sc.samplerate) == 0)
        printf("saw: %f\n", r);
        */

    return r;
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
    return (fmodf(sc.seconds * hertz, 1.0f) < 0.5f) ? 1.0f : -1.0f;
}

