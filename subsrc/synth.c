#include <asoundlib.h>
#include <pthread.h>
#include "synth.h"

static void *_msynth_thread_main(void *arg);

/* pthread globals */
static pthread_t synthread;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static snd_pcm_t pcm;

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

static void *_msynth_thread_main(void *arg)
{
    puts("synthread: started");

    /* Begin initialization of ALSA */

    /* Signal successful completion of initialization */
    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    /* Main loop */
    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);

    puts("synthread: shutting down");

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

