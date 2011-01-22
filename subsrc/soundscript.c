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

/* Initialize soundscript subsystem */
void soundscript_init()
{
    gc_ht = g_hash_table_new(NULL, NULL);
    symtab = g_hash_table_new(g_str_hash, g_str_equal);

    /* Setup built-ins */
    g_hash_table_insert(symtab, "sin", (gpointer)gen_sin);
    g_hash_table_insert(symtab, "cos", (gpointer)gen_cos);
    g_hash_table_insert(symtab, "saw", (gpointer)gen_saw);
    g_hash_table_insert(symtab, "rsaw", (gpointer)gen_rsaw);
    g_hash_table_insert(symtab, "triangle", (gpointer)gen_triangle);
    g_hash_table_insert(symtab, "pulse", (gpointer)gen_pulse);
    g_hash_table_insert(symtab, "square", (gpointer)gen_square);
    g_hash_table_insert(symtab, "chipify", (gpointer)tf_chipify);

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

/* ----- Function calls ------ */

/* Check for single input function */
int ssb_can_func1(char *func_name)
{
    return g_hash_table_lookup(symtab, func_name) != NULL;
}

/* Function call with a single input signal */
msynth_modifier ssb_func1(char *func_name, msynth_modifier in)
{
    msynth_modifier newmod = malloc(sizeof(struct _msynth_modifier));
    assert(newmod);

    newmod->type = MSMT_NODE;
    newmod->data.node.in = in;
    newmod->data.node.func =
        (msynth_modfunc)g_hash_table_lookup(symtab, func_name);
    newmod->storage = NULL;

    /* Update GC state */
    soundscript_mark_use(in);
    soundscript_mark_no_use(newmod);

    return newmod;
}

