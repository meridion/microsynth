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

    /* Allocate HW params */
    err = snd_pcm_hw_params_malloc(&hw_p);
    ALSERT("allocating hw params");

    /* Get default HW parameters */
    err = snd_pcm_hw_params_any(pcm, hw_p);
    ALSERT("requesting hw default params");

    /* Set sample rate 
     * snd_pcm_hw_params_set_rate_near(pcn, hw_p, *rate,
     *      0 (== set exact rate))
     */
    err = snd_pcm_hw_params_set_rate_near(pcm, hw_p, &srate, 0);
    ALSERT("setting samplerate");
    printf("synthread: Detected samplerate of %u\n", srate);

    /* write hw parameters to device */
    err = snd_pcm_hw_params(pcm, hw_p);
    ALSERT("writing hw params");

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

