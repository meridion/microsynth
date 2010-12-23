/* Basic synth transforms */

float tf_mul(struct sampleclock sc, float a, float b);
float tf_add(struct sampleclock sc, float a, float b);
float tf_div(struct sampleclock sc, float a, float b);
float tf_sub(struct sampleclock sc, float a, float b);
float tf_chipify(struct sampleclock sc, float in);

