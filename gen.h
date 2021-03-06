
/* Basic waveform generation */
float gen_sin(struct sampleclock sc, void **storage, float hertz);
float gen_cos(struct sampleclock sc, void **storage, float hertz);
float gen_triangle(struct sampleclock sc, void **storage, float hertz);
float gen_saw(struct sampleclock sc, void **storage, float hertz);
float gen_rsaw(struct sampleclock sc, void **storage, float hertz);
float gen_pulse(struct sampleclock sc, void **storage, float hertz);
float gen_square(struct sampleclock sc, void **storage, float hertz);
float gen_whitenoise(struct sampleclock sc, void **storage);

