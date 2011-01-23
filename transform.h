/* Basic synth transforms */

float tf_mul(struct sampleclock sc, void **storage, float a, float b);
float tf_add(struct sampleclock sc, void **storage, float a, float b);
float tf_div(struct sampleclock sc, void **storage, float a, float b);
float tf_sub(struct sampleclock sc, void **storage, float a, float b);
float tf_chipify(struct sampleclock sc, void **storage, float in);

