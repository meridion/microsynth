/* Soundscript interface */

int yyparse(void);
void soundscript_parse(char *line);

/* Global init/shutdown */
void soundscript_init();
gpointer ssi_def_func(void *func, int args);
void soundscript_shutdown();

/* Soundscript GC */
msynth_modifier soundscript_gc_mark_use(msynth_modifier mod);
msynth_modifier soundscript_gc_mark_no_use(msynth_modifier mod);
void soundscript_gc_run(void);

/* Soundscript lexer/string GC */
void soundscript_gc_clear_dups();

/* Soundscript build interface */
msynth_modifier ssb_gc_number(float num);
msynth_modifier ssb_gc_variable(char *varname);
msynth_modifier ssb_gc_add(msynth_modifier a, msynth_modifier b);
msynth_modifier ssb_gc_sub(msynth_modifier a, msynth_modifier b);
msynth_modifier ssb_gc_mul(msynth_modifier a, msynth_modifier b);
msynth_modifier ssb_gc_div(msynth_modifier a, msynth_modifier b);
msynth_modifier ssb_gc_delay(msynth_modifier in, int delay);
int ssb_can_func0(char *func_name);
int ssb_can_func1(char *func_name);
int ssb_can_func2(char *func_name);
msynth_modifier ssb_gc_func0(char *func_name);
msynth_modifier ssb_gc_func1(char *func_name, msynth_modifier in);
msynth_modifier ssb_gc_func2(char *func_name, msynth_modifier a,
    msynth_modifier b);
int ssb_is_delay(msynth_modifier mod);
int ssb_get_delay(msynth_modifier mod);
void ssb_set_delay(msynth_modifier mod, int delay);

/* Soundscript variables */
typedef struct _soundscript_var {
    msynth_modifier vargraph;
    float
        last_eval,
        recursive_next;
    int recursive;
    int mark;
} *soundscript_var;

/* Sound graph usage dependencies */
#define SSV_USAGE_NONE 0
#define SSV_USAGE_ONEWAY 1
#define SSV_USAGE_CIRCULAR 2

/* Soundscript variables interface */
void ssv_set_var(char *vname, msynth_modifier mod);
void ssv_set_var_recursive(char *vname, msynth_modifier mod);
float ssv_get_var_eval(char *vname);
void ssv_set_dummy(char *vname);
soundscript_var ssv_get_var(char *vname);
int ssv_makes_use_of(soundscript_var var1, soundscript_var var2);
int ssv_speculate_cycle(char *vname, msynth_modifier graph);
int ssv_validate_recursion(msynth_modifier graph, char *vname);
void ssv_recursively_mark_vars(soundscript_var var);
void ssv_clear_marks(unsigned int clear);
void ssv_regroup(void);
void ssv_eval(struct sampleclock sc);

