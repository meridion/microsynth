/* Soundscript interface */

int yyparse(void);
void soundscript_parse(char *line);

/* Global init/shutdown */
void soundscript_init();
gpointer ssi_def_func(void *func, int args);
void soundscript_shutdown();

/* Soundscript GC */
void soundscript_mark_use(msynth_modifier mod);
void soundscript_mark_no_use(msynth_modifier mod);
void soundscript_run_gc(void);

/* Soundscript lexer/string GC */
void soundscript_clear_dups();

/* Soundscript build interface */
msynth_modifier ssb_number(float num);
msynth_modifier ssb_add(msynth_modifier a, msynth_modifier b);
msynth_modifier ssb_sub(msynth_modifier a, msynth_modifier b);
msynth_modifier ssb_mul(msynth_modifier a, msynth_modifier b);
msynth_modifier ssb_div(msynth_modifier a, msynth_modifier b);
msynth_modifier ssb_delay(msynth_modifier in, int delay);
int ssb_can_func0(char *func_name);
int ssb_can_func1(char *func_name);
int ssb_can_func2(char *func_name);
msynth_modifier ssb_func0(char *func_name);
msynth_modifier ssb_func1(char *func_name, msynth_modifier in);
msynth_modifier ssb_func2(char *func_name, msynth_modifier a,
    msynth_modifier b);

