/* microsynth - Basic waveform generation */

/* C-stdlib */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* microsynth headers */
#include "sampleclock.h"
#include "gen.h"

#ifndef M_PI
#define M_PI 3.14159265358f
#endif

/* Oscillator local storage
 *
 * Contains the local clock and previous frequency
 */
typedef struct _osc_local {
    float
        cycle,
        prev_seconds;
} *osc_local;

/* Convenience function for decoupled oscillators
 *
 * This functions generates a signal of
 * hertz cycles/second.
 */
void osc_advance(osc_local osc, struct sampleclock sc, float hertz)
{
    osc->cycle = fmodf(osc->cycle + (sc.seconds - osc->prev_seconds) * hertz,
        1.0f);
    osc->prev_seconds = sc.seconds;
    return;
}

/* Setup decoupled oscillator structure if it does not exist yet */
static void _gen_setup_storage(void **storage, struct sampleclock sc)
{
    if (!*storage) {
        *storage = malloc(sizeof(struct _osc_local));

        if (!*storage) {
            perror("_gen_setup_storage.malloc");
            exit(1);
        }

        ((osc_local)*storage)->cycle = 0.0f;
        ((osc_local)*storage)->prev_seconds = sc.seconds;
    }

    return;
}

#define DECOUPLED_OSC \
    osc_local osc; \
    _gen_setup_storage(storage, sc); \
    osc = *storage; \
    osc_advance(osc, sc, hertz);
/* Generate sinus wave */
float gen_sin(struct sampleclock sc, void **storage, float hertz)
{
    DECOUPLED_OSC
    return sin(M_PI * 2.0f * osc->cycle);
}

/* Generate cosine wave */
float gen_cos(struct sampleclock sc, void **storage, float hertz)
{
    DECOUPLED_OSC
    return cos(M_PI * 2.0f * osc->cycle);
}

/* Generate triangle wave */
float gen_triangle(struct sampleclock sc, void **storage, float hertz)
{
    float cycle;
    DECOUPLED_OSC
    cycle = osc->cycle - 0.25f;

    if (cycle < 0.0f)
        cycle += 1.0f;

    if (cycle < 0.5f)
        return -1.0f + 2.0f * cycle;
    return 1.0f - 2.0f * (cycle - 0.5f);
}

/* Generate sawtooth wave (|\|\|\) */
float gen_saw(struct sampleclock sc, void **storage, float hertz)
{
    DECOUPLED_OSC
    return 2.0f * osc->cycle - 1.0f;
}

/* Generate reverse sawtooth wave (/|/|/|) */
float gen_rsaw(struct sampleclock sc, void **storage, float hertz)
{
    DECOUPLED_OSC
    return -gen_saw(sc, storage, hertz);
}

/* Generate pulse wave (|....|....|....) */
float gen_pulse(struct sampleclock sc, void **storage, float hertz)
{
    float speriod = (float)sc.samplerate / hertz;
    DECOUPLED_OSC

    return (fmodf((float)sc.samples, speriod) <
        fmodf((float)(sc.samples + 1), speriod)) ?
        0.0f : 1.0f;
}

/* Generate square wave */
float gen_square(struct sampleclock sc, void **storage, float hertz)
{
    DECOUPLED_OSC
    return osc->cycle < 0.5f ? 1.0f : -1.0f;
}

/* Generate whitenoise (sort of) */
float gen_whitenoise(struct sampleclock sc, void **storage)
{
    /*
    if (sc.samples % sc.samplerate == 0)
        printf("%f\n", drand48());
    */
    return (float)((double)random() / (double)RAND_MAX * 2.0 - 1.0);
}

