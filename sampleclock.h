/* Microsynth sample clock */

struct sampleclock {
    int samplerate;
    int samples;
    float seconds;
    float cycle;
};

struct sampleclock sc_from_samples(int samplerate, int samples);
struct sampleclock sc_rewind(struct sampleclock sc, int samples);

