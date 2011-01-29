/* Various basic sound transforms */
#include <math.h>

#include "sampleclock.h"
#include "synth.h"
#include "transform.h"

/* Multiply samples */
float tf_mul(struct sampleclock sc, void ** storage, float a, float b)
{
    return a * b;
}

/* Add samples */
float tf_add(struct sampleclock sc, void **storage, float a, float b)
{
    return a + b;
}

/* Divide samples */
float tf_div(struct sampleclock sc, void **storage, float a, float b)
{
    if (b == 0.0f)
        return 0.0f;
    return a / b;
}

/* Subtract samples */
float tf_sub(struct sampleclock sc, void **storage, float a, float b)
{
    return a - b;
}

/* Transform input sample to chiptune (8-bit) */
float tf_chipify(struct sampleclock sc, void **storage, float in)
{
    in *= 128.0f;

    if (in > 127.0f)
        in = 127.0f;
    if (in < -128.0f)
        in = -128.0f;

    in = roundf(in);
    in /= 128.0f;

    return in;
}

/* Delays sample 'in' by X samples
 *
 * Like add, sub, etc. this function is also inherit in
 * microsynth's syntax.
 * A statement of the form:
 *      <wave-eq>[integer]
 *
 * will inject a modifier containing a tf_delay function and
 * the appropriate configuration in storage into the <wave-eq>
 * sound path.
 * */
float tf_delay(struct sampleclock sc, void **storage, float in)
{
    tf_delay_info di = (tf_delay_info)*storage;
    float
        *history = (float*)(di + 1),
        r;

    if (di->delay) {
        r = history[di->pos];
        history[di->pos] = in;
        di->pos = (di->pos + 1) % di->delay;
    } else {
        r = in;
    }

    return r;
}

/* Minimum of input signals */
float tf_min(struct sampleclock sc, void **storage, float a, float b)
{
    return fminf(a, b);
}

/* Maximum of input signals */
float tf_max(struct sampleclock sc, void **storage, float a, float b)
{
    return fmaxf(a, b);
}

/* Absolute of input signal */
float tf_abs(struct sampleclock sc, void **storage, float in)
{
    return fabsf(in);
}

/* Clamp input signal to other input signal */
float tf_clamp(struct sampleclock sc, void **storage, float a, float b)
{
    if (fabs(a) > fabs(b))
        return b;
    else
        return a;
}

/* Mathematical floor */
float tf_floor(struct sampleclock sc, void **storage, float in)
{
    return floorf(in);
}

/* Mathematical ceil */
float tf_ceil(struct sampleclock sc, void **storage, float in)
{
    return ceilf(in);
}

