
/* synth types */
typedef struct _msynth_modifier *msynth_modifier;
typedef struct _msynth_frame *msynth_frame;

/* synth callbacks */
typedef float (*msynth_modfunc0)(struct sampleclock sc, void **storage);
typedef float (*msynth_modfunc)(struct sampleclock sc, void **storage,
    float in);
typedef float (*msynth_modfunc2)(struct sampleclock sc, void **storage,
    float a, float b);

/* synth modifiers */
struct _msynth_modifier {
    int type;
    void *storage;

    union _msynth_modifier_data {
        struct _mod_node {
            msynth_modifier in;
            msynth_modfunc func;
        } node;

        struct _mod_node2 {
            msynth_modifier a, b;
            msynth_modfunc2 func;
        } node2;

        struct _mod_node0 {
            msynth_modfunc0 func;
        } node0;

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
#define MSMT_NODE0      1
#define MSMT_NODE       2
#define MSMT_NODE2      3

/* NULL signal */
extern struct _msynth_modifier msynth_null_signal;

/* synth interface */
void msynth_init();
void msynth_shutdown();
int synth_recover(int err);
float synth_eval(msynth_modifier mod, struct sampleclock sc);
void synth_replace(msynth_modifier tree);
void synth_free_recursive(msynth_modifier mod);
void synth_set_volume(float new_volume);
float synth_get_volume();

