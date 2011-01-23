/* microsynth - synthesizer core */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <asoundlib.h>
#include <pthread.h>

/* msynth headers */
#include "sampleclock.h"
#include "gen.h"
#include "synth.h"

/* msynth NULL signal */
struct _msynth_modifier msynth_null_signal;

static void *_msynth_thread_main(void *arg);

/* pthread globals */
static pthread_t synthread;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/* ALSA stuff */
static snd_pcm_t *pcm;
static snd_pcm_hw_params_t *hw_p;
static snd_pcm_sw_params_t *sw_p;
static unsigned int srate = 44100;
static int dir = 0;
static snd_pcm_uframes_t
    buffer_size = 0,
    period_size = 0;

/* microsynth settings */
static unsigned int buffer_usec = 500000;
static unsigned int period_usec = 250000;
static volatile int shutdown = 0;
static float volume = 0.5f;
static struct _msynth_modifier *root = &msynth_null_signal;

/* microsynth stats */
static int recover_resumes = 0, recover_xruns = 0;

void msynth_init()
{
    /* Setup null signal */
    msynth_null_signal.type = MSMT_CONSTANT;
    msynth_null_signal.data.constant = 0.0f;

    /* Start synth thread */
    if (pthread_create(&synthread, NULL, _msynth_thread_main, NULL)) {
        fprintf(stderr, "error: Cannot start synthread\n");
        exit(1);
    }

    /* Wait for synth to initialize */
    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);

    return;
}

void msynth_shutdown()
{
    shutdown = 1;
    pthread_join(synthread, NULL);
    return;
}

/* Convenience macro for ALSA initialization */
#define ALSERT(MSG) \
    if (err < 0) { \
        fprintf(stderr, "synthread: Error while " MSG ": %s\n", \
            snd_strerror(err)); \
        exit(EXIT_FAILURE); \
    }

static void *_msynth_thread_main(void *arg)
{
    int i;
    int err;
    int sample;
    int processed;

    struct sampleclock sc = {0, 0, 0.0f, 0.0f};
    msynth_frame fb = NULL;

    puts("synthread: started");

    /* Begin initialization of ALSA */
    /* snd_pcm_open(pcm_handle, device_name, stream_type, open_mode) */
    err = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
    ALSERT("opening audio device");

    /* First ALSA hardware settings */

    /* Allocate HW params */
    err = snd_pcm_hw_params_malloc(&hw_p);
    ALSERT("allocating hw params");

    /* Get default HW parameters */
    err = snd_pcm_hw_params_any(pcm, hw_p);
    ALSERT("requesting hw default params");

    /* Disable software resampling */
    err = snd_pcm_hw_params_set_rate_resample(pcm, hw_p, 0);
    ALSERT("disabling software resampling");

    /* Get maximum hardware samplerate */
    err = snd_pcm_hw_params_get_rate_max(hw_p, &srate, &dir);
    ALSERT("requesting maximum hardware samplerate");

    /* Set sample rate 
     * snd_pcm_hw_params_set_rate_near(pcn, hw_p, *rate,
     *      0 (== set exact rate))
     */
    err = snd_pcm_hw_params_set_rate_near(pcm, hw_p, &srate, 0);
    ALSERT("setting samplerate");
    printf("synthread: Detected samplerate of %u\n", srate);

    /* RW interleaved access (means we will use the snd_pcm_writei function) */
    err = snd_pcm_hw_params_set_access(pcm, hw_p,
        SND_PCM_ACCESS_RW_INTERLEAVED);
    ALSERT("setting access mode");

    /* native-endian 16-bit signed sample format */
    err = snd_pcm_hw_params_set_format(pcm, hw_p, SND_PCM_FORMAT_S16);
    ALSERT("setting sample format");

    /* Switch to 2.0ch audio */
    err = snd_pcm_hw_params_set_channels(pcm, hw_p, 2);
    ALSERT("switching to 2.0ch audio");

    /* Since we want the interactive synthesizer to be very responsive
     * select a minimum buffer size.
     *
     * FIXME: since a minimum latency buffer results in non-stop XRUNs
     * for now make do with approximately 0.2s latency, which is acceptable.
     */
    err = snd_pcm_hw_params_set_buffer_time_near(pcm, hw_p, &buffer_usec, &dir);
    ALSERT("setting buffer time");

    /* Retrieve resulting buffer size */
    err = snd_pcm_hw_params_get_buffer_size(hw_p, &buffer_size);
    ALSERT("getting buffer size");
    printf("synthread: Selected buffersize of %lu\n", buffer_size);
    printf("synthread: Response delay is approximately %.2f ms\n",
        (double)buffer_size / (double)srate * 1000.0);

    /* Select a period time of half the buffer time
     * since this improves processing performance
     */
    err = snd_pcm_hw_params_set_period_time_near(pcm, hw_p, &period_usec, &dir);
    ALSERT("setting period time");

    /* Retrieve resulting period size */
    err = snd_pcm_hw_params_get_period_size(hw_p, &period_size, &dir);
    ALSERT("getting period size");
    if (dir)
        printf("synthread: Selected period size near %lu\n", period_size);
    else
        printf("synthread: Selected period size of %lu\n", period_size);

    /* write hw parameters to device */
    err = snd_pcm_hw_params(pcm, hw_p);
    ALSERT("writing hw params");

    /* Begin ALSA software side setup */

    /* Allocate SW params */
    err = snd_pcm_sw_params_malloc(&sw_p);
    ALSERT("allocating sw params");

    /* Request software current settings */
    err = snd_pcm_sw_params_current(pcm, sw_p);
    ALSERT("requesting current sw params");

    /* Automatically start playing after the first period is written */
    err = snd_pcm_sw_params_set_start_threshold(pcm, sw_p, period_size);
    ALSERT("settings start threshold");

    /* XRUN when buffer is empty */
    err = snd_pcm_sw_params_set_stop_threshold(pcm, sw_p, buffer_size * 2);
    ALSERT("setting stop threshold");

    /* Block synthesizer when there is not at least period frames available */
    err = snd_pcm_sw_params_set_avail_min(pcm, sw_p, period_size);
    ALSERT("setting minimum free frames");

    /* Write software parameters */
    err = snd_pcm_sw_params(pcm, sw_p);
    ALSERT("writing sw params");
    printf("synthread: Audio system configured.\n");

    /* Prepare device for playback */
    err = snd_pcm_prepare(pcm);
    ALSERT("preparing device");

    /* Allocate a period sized framebuffer
     * (yup an audio buffer is in ALSA speak indeed called a framebuffer)
     */
    fb = malloc(sizeof(struct _msynth_frame) * period_size);
    if (!fb) {
        perror("malloc framebuffer failed");
        exit(1);
    }

    /* Set sampleclock to 0 */
    sc.samples = 0;
    sc.samplerate = srate;
    sc.cycle = 0.0f;
    sc.seconds = 0.0f;

    /* Initially the <root> variable describing the flow of sound within
     * the synthesizer is configured to be a NULL signal. (silence)
     */

    /* Signal successful completion of initialization */
    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    /* -------------- Main loop --------------- */
    while (!shutdown) {
        /* Only during generation we need the synth tree to be static */
        pthread_mutex_lock(&mutex);
        for (i = 0; i < period_size; i++) {
            sample = (int)(32767.5 * synth_eval(root, sc) * volume);

            /* Clip samples */
            if (sample > 32767) sample = 32767;
            if (sample < -32768) sample = -32768;

            fb[i].left = fb[i].right = (short)sample;
            sc = sc_from_samples(sc.samplerate, sc.samples + 1);
        }
        pthread_mutex_unlock(&mutex);

        /* Send audio to sound card */
        processed = 0;
        while (processed != period_size) {
            err = snd_pcm_writei(pcm, fb + processed, period_size - processed);

            /* Retry on interruption by signal */
            if (err == -EAGAIN)
                continue;

            /* Recover from XRUN/suspend */
            if (err < 0) {
                err = synth_recover(err);
                ALSERT("sending audio");
                continue;
            }

            /* Update processed samples */
            processed += err;
        }
    }

    puts("synthread: shutting down");

    /* Dump any statistics recorded */
    if (recover_resumes)
        printf("synthread: Device was resumed %i times\n", recover_resumes);
    if (recover_xruns)
        printf("synthread: %i xrun recoveries were needed\n", recover_xruns);
    printf("synthread: processed %i samples\n", sc.samples);

    /* Wait for playback to complete */
    err = snd_pcm_drain(pcm);
    ALSERT("draining device");

    /* Shutdown PCM device */
    err = snd_pcm_close(pcm);
    ALSERT("closing audio device");

    /* Clean up parameters */
    snd_pcm_hw_params_free(hw_p);
    snd_pcm_sw_params_free(sw_p);

    return NULL;
}

/* Recover from suspend/underrun */
int synth_recover(int err)
{
    /* Recover from underrun */
    if (err == -EPIPE) {
        err = snd_pcm_prepare(pcm);
        ALSERT("recovering from underrun");

        recover_xruns++;
        return 0;
    }

    if (err == -ESTRPIPE) {
        /* Wait for the device to become resumable */
        while ((err = snd_pcm_resume(pcm)) == -EAGAIN)
            sleep(1);
        ALSERT("recovering from suspend");

        /* Finally prepare device for playback */
        err = snd_pcm_prepare(pcm);
        ALSERT("preparing device after resume");

        recover_resumes++;
        return 0;
    }

    return err;
}

/* Replace audio flow tree */
void synth_replace(msynth_modifier tree)
{
    /* prevent audio synthesis during replacement process */
    pthread_mutex_lock(&mutex);

    if (root != &msynth_null_signal)
        synth_free_recursive(root);

    root = tree;
    pthread_mutex_unlock(&mutex);

    return;
}

/* Evaluate sound flow graph.
 *
 * This is where the actual synthesis takes place.
 * mod: A graph structure indicating oscillators, transformers and
 *      filters.
 * sc: The sample clock, used to make the synthesis discrete.
 */
float synth_eval(msynth_modifier mod, struct sampleclock sc)
{
    switch(mod->type) {
        case MSMT_CONSTANT:
            return mod->data.constant;

        case MSMT_NODE:
            return mod->data.node.func(sc, &mod->storage,
                synth_eval(mod->data.node.in, sc));

        case MSMT_NODE2:
            return mod->data.node2.func(sc, &mod->storage,
                synth_eval(mod->data.node2.a, sc),
                synth_eval(mod->data.node2.b, sc));

        default:;
    }

    return 0.0f;
}

/* Recursively free synth modifier graph */
void synth_free_recursive(msynth_modifier mod)
{
    switch(mod->type) {
        case MSMT_NODE:
            synth_free_recursive(mod->data.node.in);
            break;

        case MSMT_NODE2:
            synth_free_recursive(mod->data.node2.a);
            synth_free_recursive(mod->data.node2.b);
            break;

        default:;
    }

    if (mod->storage)
        free(mod->storage);
    free(mod);
    return;
}

