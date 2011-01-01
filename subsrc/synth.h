
/* synth types */
typedef struct _msynth_modifier *msynth_modifier;
typedef struct _msynth_frame *msynth_frame;

/* synth callbacks */
typedef float (*msynth_modfunc)(struct sampleclock sc, float in);
typedef float (*msynth_modfunc2)(struct sampleclock sc, float a, float b);

/* synth modifiers */
struct _msynth_modifier {
    int type;

    union _msynth_modifier_data {
        struct _mod_node {
            msynth_modifier in;
            msynth_modfunc func;
        } node;

        struct _mod_node2 {
            msynth_modifier a, b;
            msynth_modfunc2 func;
        } node2;

        float constant;
    } data;
};

struct _msynth_frame {
    short left;
    short right;
};

/* synth modification types */
#define MSMT_INVALID    -1
#define MSMT_CONSTANT   0
#define MSMT_NODE       1
#define MSMT_NODE2      2

/* NULL signal */
extern struct _msynth_modifier msynth_null_signal;

/* synth interface */
void msynth_init();
void msynth_shutdown();
int synth_recover(int err);
float synth_eval(msynth_modifier mod, struct sampleclock sc);
void synth_replace(msynth_modifier tree);
void synth_free_recursive(msynth_modifier mod);

