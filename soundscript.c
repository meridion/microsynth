/* microsynth - Sound scripting */
#include <assert.h>
#include <glib.h>

#include "sampleclock.h"
#include "synth.h"
#include "gen.h"
#include "transform.h"
#include "soundscript_lex.h"
#include "soundscript_parse.h"
#include "soundscript.h"

/* Soundscript function definition */
struct ss_func_def {
    void *func;
    int args;
};

/* Soundscript GC */
static GHashTable *gc_ht = NULL;

/* Symbol table */
static GHashTable *symtab;

/* Parse a command line */
void soundscript_parse(char *line)
{

    char *mod_str;
    int len;
    YY_BUFFER_STATE x;

    /* Build string with appended newline */
    len = strlen(line);
    mod_str = malloc(sizeof(char) * len + 3);
    memcpy(mod_str, line, sizeof(char) * len);
    mod_str[len] = '\n';
    mod_str[len + 1] = '\0';
    mod_str[len + 2] = '\0';

    /* Parse string */
    x = yy_scan_buffer(mod_str, len + 3);
    yyparse();
    yy_delete_buffer(x);

    /* Clean up */
    soundscript_run_gc();
    free(mod_str);

    return;
}

/* Generate function definition */
gpointer ssi_def_func(void *func, int args)
{
    struct ss_func_def *def;
    def = malloc(sizeof(struct ss_func_def));
    if (!def) {
        perror("ssi_def_func");
        exit(1);
    }

    def->args = args;
    def->func = func;

    return def;
}

/* Initialize soundscript subsystem */
void soundscript_init()
{
    gc_ht = g_hash_table_new(NULL, NULL);
    symtab = g_hash_table_new(g_str_hash, g_str_equal);

    /* Setup built-in functions */

    /* Oscillators */
    g_hash_table_insert(symtab, "sin", ssi_def_func(gen_sin, 1));
    g_hash_table_insert(symtab, "cos", ssi_def_func(gen_cos, 1));
    g_hash_table_insert(symtab, "saw", ssi_def_func(gen_saw, 1));
    g_hash_table_insert(symtab, "rsaw", ssi_def_func(gen_rsaw, 1));
    g_hash_table_insert(symtab, "triangle", ssi_def_func(gen_triangle, 1));
    g_hash_table_insert(symtab, "pulse", ssi_def_func(gen_pulse, 1));
    g_hash_table_insert(symtab, "square", ssi_def_func(gen_square, 1));
    g_hash_table_insert(symtab, "whitenoise", ssi_def_func(gen_whitenoise, 0));

    /* Transformers */
    g_hash_table_insert(symtab, "chipify", ssi_def_func(tf_chipify, 1));

    /* Mathematical operations */
    g_hash_table_insert(symtab, "add", ssi_def_func(tf_add, 2));
    g_hash_table_insert(symtab, "sub", ssi_def_func(tf_sub, 2));
    g_hash_table_insert(symtab, "mul", ssi_def_func(tf_mul, 2));
    g_hash_table_insert(symtab, "div", ssi_def_func(tf_div, 2));
    g_hash_table_insert(symtab, "min", ssi_def_func(tf_min, 2));
    g_hash_table_insert(symtab, "max", ssi_def_func(tf_max, 2));
    g_hash_table_insert(symtab, "abs", ssi_def_func(tf_abs, 1));
    g_hash_table_insert(symtab, "clamp", ssi_def_func(tf_clamp, 2));
    g_hash_table_insert(symtab, "floor", ssi_def_func(tf_floor, 1));
    g_hash_table_insert(symtab, "ceil", ssi_def_func(tf_ceil, 1));

    return;
}

/* Destroy soundscript subsystem */
void soundscript_shutdown()
{
    g_hash_table_destroy(gc_ht);
    g_hash_table_destroy(symtab);
    return;
}

/* Mark mod pointer as used */
void soundscript_mark_use(msynth_modifier mod)
{
     /*
     * A hashtable entry means the garbage collector
     * will free this pointer, therefore we remove it,
     * if the key was not there anyway, there is no problem.
     */
    g_hash_table_remove(gc_ht, mod);

    return;
}

/* Mark mod pointer as unused */
void soundscript_mark_no_use(msynth_modifier mod)
{
     /*
     * Insert the mod as key and value to mark it
     * for removal.
     */
     g_hash_table_insert(gc_ht, mod, mod);

    return;
}

/* Destroy all unused pointers and clear GC */
void soundscript_run_gc(void)
{
    GList *list;
    msynth_modifier mod;

    list = g_hash_table_get_values(gc_ht);

    /* While elements in list, free and update */
    while (list) {
        mod = g_list_nth_data(list, 0);

        /* Remove object */
        synth_free_recursive(mod);

        /* grab next item in list */
        list = g_list_next(list);
    }

    /* Clear GC table */
    g_hash_table_remove_all(gc_ht);

    return;
}

/* -------- Soundscript Build Interface ---------- */

/* Constant signal */
msynth_modifier ssb_number(float num)
{
    msynth_modifier newmod = malloc(sizeof(struct _msynth_modifier));
    assert(newmod);

    newmod->type = MSMT_CONSTANT;
    newmod->data.constant = num;
    newmod->storage = NULL;

    return newmod;
}

/* Add samples */
msynth_modifier ssb_add(msynth_modifier a, msynth_modifier b)
{
    msynth_modifier newmod = malloc(sizeof(struct _msynth_modifier));
    assert(newmod);

    newmod->type = MSMT_NODE2;
    newmod->data.node2.func = tf_add;
    newmod->data.node2.a = a;
    newmod->data.node2.b = b;
    newmod->storage = NULL;

    /* Update GC */
    soundscript_mark_no_use(newmod);
    soundscript_mark_use(a);
    soundscript_mark_use(b);

    return newmod;
}

/* Subtract samples */
msynth_modifier ssb_sub(msynth_modifier a, msynth_modifier b)
{
    msynth_modifier newmod = malloc(sizeof(struct _msynth_modifier));
    assert(newmod);

    newmod->type = MSMT_NODE2;
    newmod->data.node2.func = tf_sub;
    newmod->data.node2.a = a;
    newmod->data.node2.b = b;
    newmod->storage = NULL;

    /* Update GC */
    soundscript_mark_no_use(newmod);
    soundscript_mark_use(a);
    soundscript_mark_use(b);

    return newmod;
}

/* Multiply samples */
msynth_modifier ssb_mul(msynth_modifier a, msynth_modifier b)
{
    msynth_modifier newmod = malloc(sizeof(struct _msynth_modifier));
    assert(newmod);

    newmod->type = MSMT_NODE2;
    newmod->data.node2.func = tf_mul;
    newmod->data.node2.a = a;
    newmod->data.node2.b = b;
    newmod->storage = NULL;

    /* Update GC */
    soundscript_mark_no_use(newmod);
    soundscript_mark_use(a);
    soundscript_mark_use(b);

    return newmod;
}

/* Divide sample by sample */
msynth_modifier ssb_div(msynth_modifier a, msynth_modifier b)
{
    msynth_modifier newmod = malloc(sizeof(struct _msynth_modifier));
    assert(newmod);

    newmod->type = MSMT_NODE2;
    newmod->data.node2.func = tf_div;
    newmod->data.node2.a = a;
    newmod->data.node2.b = b;
    newmod->storage = NULL;

    /* Update GC */
    soundscript_mark_no_use(newmod);
    soundscript_mark_use(a);
    soundscript_mark_use(b);

    return newmod;
}

/* Delay sample by X */
msynth_modifier ssb_delay(msynth_modifier in, int delay)
{
    tf_delay_info di;
    float *history;
    int i;

    msynth_modifier newmod = malloc(sizeof(struct _msynth_modifier));
    assert(newmod);

    newmod->type = MSMT_NODE;
    newmod->data.node.func = tf_delay;
    newmod->data.node.in = in;
    newmod->storage = malloc(sizeof(struct _tf_delay_info) +
        sizeof(float) * delay);
    assert(newmod->storage);

    di = (tf_delay_info)newmod->storage;
    history = (float*)(di + 1);
    di->delay = delay;
    di->pos = 0;

    for (i = 0; i < delay; i++)
        history[i] = 0.0f;

    /* Update GC */
    soundscript_mark_no_use(newmod);
    soundscript_mark_use(in);

    return newmod;
}

/* ----- Function calls ------ */

/* Check for generator function (Disabled) */
int ssb_can_func0(char *func_name)
{
    struct ss_func_def *def = g_hash_table_lookup(symtab, func_name);
    if (!def)
        return 0;

    return def->args == 0;
}

/* Check for single input function */
int ssb_can_func1(char *func_name)
{
    struct ss_func_def *def = g_hash_table_lookup(symtab, func_name);
    if (!def)
        return 0;

    return def->args == 1;
}

/* Check for dual signal function */
int ssb_can_func2(char *func_name)
{
    struct ss_func_def *def = g_hash_table_lookup(symtab, func_name);
    if (!def)
        return 0;

    return def->args == 2;
}

/* Function generating signal (Disabled) */
msynth_modifier ssb_func0(char *func_name)
{
    msynth_modifier newmod = malloc(sizeof(struct _msynth_modifier));
    assert(newmod);

    newmod->type = MSMT_NODE0;
    newmod->data.node0.func =
        (msynth_modfunc)
        ((struct ss_func_def*)g_hash_table_lookup(symtab, func_name))->func;
    newmod->storage = NULL;

    /* Update GC state */
    soundscript_mark_no_use(newmod);

    return newmod;
}

/* Function call with a single input signal */
msynth_modifier ssb_func1(char *func_name, msynth_modifier in)
{
    msynth_modifier newmod = malloc(sizeof(struct _msynth_modifier));
    assert(newmod);

    newmod->type = MSMT_NODE;
    newmod->data.node.in = in;
    newmod->data.node.func =
        (msynth_modfunc)
        ((struct ss_func_def*)g_hash_table_lookup(symtab, func_name))->func;
    newmod->storage = NULL;

    /* Update GC state */
    soundscript_mark_use(in);
    soundscript_mark_no_use(newmod);

    return newmod;
}

/* Return modifier for function accepting 2 input signals */
msynth_modifier ssb_func2(char *func_name, msynth_modifier a,
    msynth_modifier b)
{
    msynth_modifier newmod = malloc(sizeof(struct _msynth_modifier));
    assert(newmod);

    newmod->type = MSMT_NODE2;
    newmod->data.node2.a = a;
    newmod->data.node2.b = b;
    newmod->data.node2.func =
        (msynth_modfunc)
        ((struct ss_func_def*)g_hash_table_lookup(symtab, func_name))->func;
    newmod->storage = NULL;

    /* Update GC state */
    soundscript_mark_use(a);
    soundscript_mark_use(b);
    soundscript_mark_no_use(newmod);

    return newmod;
}

