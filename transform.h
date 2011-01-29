/* Basic synth transforms */

/* transform storage structures */
typedef struct _tf_delay_info {
    int delay, pos;
} *tf_delay_info;

/* transform functions */
float tf_mul(struct sampleclock sc, void **storage, float a, float b);
float tf_add(struct sampleclock sc, void **storage, float a, float b);
float tf_div(struct sampleclock sc, void **storage, float a, float b);
float tf_sub(struct sampleclock sc, void **storage, float a, float b);
float tf_delay(struct sampleclock sc, void **storage, float in);
float tf_chipify(struct sampleclock sc, void **storage, float in);

