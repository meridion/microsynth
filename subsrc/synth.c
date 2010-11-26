#include <asoundlib.h>
#include <pthread.h>
#include "synth.h"

static void *_msynth_thread_main(void *arg);

/* pthread globals */
static pthread_t synthread;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static snd_pcm_t *pcm;
static snd_pcm_hw_params_t *hw_p;
static snd_pcm_sw_params_t *sw_p;
static unsigned int srate = 44100;
static int dir = 0;
static snd_pcm_uframes_t
    buffer_size = 0,
    period_size = 0;

void msynth_init()
{
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

#define ALSERT(MSG) \
    if (err < 0) { \
        fprintf(stderr, "synthread: Error while " MSG ": %s\n", \
            snd_strerror(err)); \
        exit(EXIT_FAILURE); \
    }

static void *_msynth_thread_main(void *arg)
{
    int err;

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

    /* native-endian 16-bit signed sample format */
    err = snd_pcm_hw_params_set_format(pcm, hw_p, SND_PCM_FORMAT_S16);
    ALSERT("setting sample format");

    /* Switch to 2.0ch audio */
    err = snd_pcm_hw_params_set_channels(pcm, hw_p, 2);
    ALSERT("switching to 2.0ch audio");

    /* Since we want the interactive synthesizer to be very responsive
     * select a minimum buffer size.
     */
    err = snd_pcm_hw_params_set_buffer_size_first(pcm, hw_p, &buffer_size);
    ALSERT("setting buffer size");
    printf("synthread: Selected buffersize of %lu\n", buffer_size);
    printf("synthread: Response delay is approximately %.2f ms\n",
        (double)buffer_size / (double)srate * 1000.0);

    /* Select maximum period size since this improves processing performance */
    err = snd_pcm_hw_params_set_period_size_last(pcm, hw_p, &period_size, &dir);
    ALSERT("setting period size");
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
    err = snd_pcm_sw_params_set_stop_threshold(pcm, sw_p, 0);
    ALSERT("setting stop threshold");

    /* Block synthesizer when there is not at least period frames available */
    err = snd_pcm_sw_params_set_avail_min(pcm, sw_p, period_size);
    ALSERT("setting minimum free frames");

    /* Write software parameters */
    err = snd_pcm_sw_params(pcm, sw_p);
    ALSERT("writing sw params");
    printf("synthread: Audio system configured.\n");

    /* Signal successful completion of initialization */
    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    /* Main loop */
    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);

    puts("synthread: shutting down");
    err = snd_pcm_close(pcm);
    ALSERT("closing audio device");

    return NULL;
}

void msynth_shutdown()
{
    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    pthread_join(synthread, NULL);
    return;
}

