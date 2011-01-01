/* Soundscript interface */

int yyparse(void);
void soundscript_parse(char *line);

/* Global init/shutdown */
void soundscript_init();
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
int ssb_can_func1(char *func_name);
msynth_modifier ssb_func1(char *func_name, msynth_modifier in);

