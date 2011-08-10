/* microsynth - Sound scripting */
#include <assert.h>
#include <glib.h>
#include <pthread.h>

#include "sampleclock.h"
#include "synth.h"
#include "gen.h"
#include "transform.h"
#include "soundscript_lex.h"
#include "soundscript_parse.h"
#include "soundscript.h"

/* Local function definitions */
void _ssv_recursively_mark_graphs(msynth_modifier mod);

/* Cast override functions (work around for warnings) */
#define __DEF_FORCE_CAST(INTYPE, OUTTYPE, NAME) \
static OUTTYPE __force_cast_ ## NAME(INTYPE pin) \
{ \
    union _force_cast { \
        INTYPE pin; \
        OUTTYPE pout; \
    } fc; \
    fc.pin = pin; \
    return fc.pout; \
}

__DEF_FORCE_CAST(msynth_modfunc0, void*, from_func0)
__DEF_FORCE_CAST(msynth_modfunc, void*, from_func1)
__DEF_FORCE_CAST(msynth_modfunc2, void*, from_func2)
__DEF_FORCE_CAST(void*, msynth_modfunc0, to_func0)
__DEF_FORCE_CAST(void*, msynth_modfunc, to_func1)
__DEF_FORCE_CAST(void*, msynth_modfunc2, to_func2)

/* Soundscript function definition */
struct ss_func_def {
    void *func;
    int args;
};

/* Soundscript GC */
static GHashTable *gc_ht = NULL;

/* Symbol table */
static GHashTable *symtab; /* Function table */
static GHashTable *vartab; /* Variable table */

/* Variable evaluation array */
static soundscript_var *eval_list = NULL;

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

    /* Ensure no synthesis will take place while we modify
     * the sound graphs
     */
    synth_lock_graphs();

    /* Parse string */
    x = yy_scan_buffer(mod_str, len + 3);
    yyparse();
    yy_delete_buffer(x);

    /* Clean up */
    soundscript_run_gc();
    free(mod_str);

    /* Update variable evaluation order */
    ssv_regroup();

    /* Finally unlock synthesizer */
    synth_unlock_graphs();

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

/* Initialize soundscript subsystem - THIS FUNCTION MUST BE CALLED BEFORE msynth_init */
void soundscript_init()
{
    gc_ht = g_hash_table_new(NULL, NULL);
    symtab = g_hash_table_new(g_str_hash, g_str_equal);
    vartab = g_hash_table_new(g_str_hash, g_str_equal);

    /* Setup built-in functions */

    /* Oscillators */
    g_hash_table_insert(symtab, "sin", ssi_def_func(__force_cast_from_func1(gen_sin), 1));
    g_hash_table_insert(symtab, "cos", ssi_def_func(
        __force_cast_from_func1(gen_cos), 1));
    g_hash_table_insert(symtab, "saw", ssi_def_func(
        __force_cast_from_func1(gen_saw), 1));
    g_hash_table_insert(symtab, "rsaw", ssi_def_func(
        __force_cast_from_func1(gen_rsaw), 1));
    g_hash_table_insert(symtab, "triangle", ssi_def_func(
        __force_cast_from_func1(gen_triangle), 1));
    g_hash_table_insert(symtab, "pulse", ssi_def_func(
        __force_cast_from_func1(gen_pulse), 1));
    g_hash_table_insert(symtab, "square", ssi_def_func(
        __force_cast_from_func1(gen_square), 1));
    g_hash_table_insert(symtab, "whitenoise", ssi_def_func(
        __force_cast_from_func0(gen_whitenoise), 0));

    /* Transformers */
    g_hash_table_insert(symtab, "chipify", ssi_def_func(
        __force_cast_from_func1(tf_chipify), 1));

    /* Mathematical operations */
    g_hash_table_insert(symtab, "add", ssi_def_func(
        __force_cast_from_func2(tf_add), 2));
    g_hash_table_insert(symtab, "sub", ssi_def_func(
        __force_cast_from_func2(tf_sub), 2));
    g_hash_table_insert(symtab, "mul", ssi_def_func(
        __force_cast_from_func2(tf_mul), 2));
    g_hash_table_insert(symtab, "div", ssi_def_func(
        __force_cast_from_func2(tf_div), 2));
    g_hash_table_insert(symtab, "min", ssi_def_func(
        __force_cast_from_func2(tf_min), 2));
    g_hash_table_insert(symtab, "max", ssi_def_func(
        __force_cast_from_func2(tf_max), 2));
    g_hash_table_insert(symtab, "abs", ssi_def_func(
        __force_cast_from_func1(tf_abs), 1));
    g_hash_table_insert(symtab, "clamp", ssi_def_func(
        __force_cast_from_func2(tf_clamp), 2));
    g_hash_table_insert(symtab, "floor", ssi_def_func(
        __force_cast_from_func1(tf_floor), 1));
    g_hash_table_insert(symtab, "ceil", ssi_def_func(
        __force_cast_from_func1(tf_ceil), 1));

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
msynth_modifier soundscript_mark_use(msynth_modifier mod)
{
     /*
     * A hashtable entry means the garbage collector
     * will free this pointer, therefore we remove it,
     * if the key was not there anyway, there is no problem.
     */
    g_hash_table_remove(gc_ht, mod);

    return mod;
}

/* Mark mod pointer as unused */
msynth_modifier soundscript_mark_no_use(msynth_modifier mod)
{
     /*
     * Insert the mod as key and value to mark it
     * for removal.
     */
     g_hash_table_insert(gc_ht, mod, mod);

    return mod;
}

/* Destroy all unused pointers and clear GC */
void soundscript_run_gc(void)
{
    GList *list, *iter;
    msynth_modifier mod;

    iter = list = g_hash_table_get_values(gc_ht);

    /* While elements in iter, free and update */
    while (iter) {
        mod = g_list_nth_data(iter, 0);

        /* Remove object */
        synth_free_recursive(mod);

        /* grab next item in iter */
        iter = g_list_next(iter);
    }

    /* Clear GC table */
    g_hash_table_remove_all(gc_ht);
    g_list_free(list);

    return;
}

/* -------- Soundscript Build Interface ---------- */
/* The following functions are all garbage collected, use with caution */

/* Constant signal */
msynth_modifier ssb_number(float num)
{
    msynth_modifier newmod = malloc(sizeof(struct _msynth_modifier));
    assert(newmod);

    newmod->type = MSMT_CONSTANT;
    newmod->data.constant = num;
    newmod->storage = NULL;

    /* Update GC */
    soundscript_mark_no_use(newmod);

    return newmod;
}

/* Variable reference */
msynth_modifier ssb_variable(char *varname)
{
    msynth_modifier newmod = malloc(sizeof(struct _msynth_modifier));
    assert(newmod);

    newmod->type = MSMT_VARIABLE;
    newmod->data.varname = strdup(varname);
    assert(newmod->data.varname);
    newmod->storage = NULL;

    /* Update GC */
    soundscript_mark_no_use(newmod);

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
    msynth_modifier newmod = malloc(sizeof(struct _msynth_modifier));
    assert(newmod);

    newmod->type = MSMT_NODE1;
    newmod->data.node.func = tf_delay;
    newmod->data.node.in = in;
    newmod->storage = NULL;
    ssb_set_delay(newmod, delay);

    /* Update GC */
    soundscript_mark_no_use(newmod);
    soundscript_mark_use(in);

    return newmod;
}

/* ----- Function calls ------ */

/* Check for generator function */
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

/* Function generating signal (such as whitenoise) */
msynth_modifier ssb_func0(char *func_name)
{
    msynth_modifier newmod = malloc(sizeof(struct _msynth_modifier));
    assert(newmod);

    newmod->type = MSMT_NODE0;
    newmod->data.node0.func =
        __force_cast_to_func0(
        ((struct ss_func_def*)g_hash_table_lookup(symtab, func_name))->func);
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

    newmod->type = MSMT_NODE1;
    newmod->data.node.in = in;
    newmod->data.node.func =
        __force_cast_to_func1(
        ((struct ss_func_def*)g_hash_table_lookup(symtab, func_name))->func);
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
        __force_cast_to_func2(
        ((struct ss_func_def*)g_hash_table_lookup(symtab, func_name))->func);
    newmod->storage = NULL;

    /* Update GC state */
    soundscript_mark_use(a);
    soundscript_mark_use(b);
    soundscript_mark_no_use(newmod);

    return newmod;
}

/* Check if node is delay */
int ssb_is_delay(msynth_modifier mod)
{
    return mod->type == MSMT_NODE1 && mod->data.node.func == tf_delay;
}

/* Get sample delay
 *
 * this function will assume mod is a delay, use with caution
 */
int ssb_get_delay(msynth_modifier mod)
{
    tf_delay_info delay = (tf_delay_info)mod->storage;

    return delay->delay;
}

/* Change sample delay
 *
 * this function will assume mod is a valid delay, use with caution
 */
void ssb_set_delay(msynth_modifier mod, int delay)
{
    tf_delay_info di;
    float *history;
    int i;

    free(mod->storage);
    mod->storage = malloc(sizeof(struct _tf_delay_info) +
        sizeof(float) * delay);
    assert(mod->storage);

    di = (tf_delay_info)mod->storage;
    history = (float*)(di + 1);
    di->delay = delay;
    di->pos = 0;

    for (i = 0; i < delay; i++)
        history[i] = 0.0f;

    return;
}

/* -------- Soundscript variables -------- */

/* Set var <vname> to <mod> */
void ssv_set_var(char *vname, msynth_modifier mod)
{
    soundscript_var new; 

    new = g_hash_table_lookup(vartab, vname);

    /* Replace existing var or allocate if necessary */
    if (new) {
        synth_free_recursive(new->vargraph);
    } else {
        new = malloc(sizeof(struct _soundscript_var));
        assert(new);
        g_hash_table_insert(vartab, vname, new);
    }

    new->vargraph = mod;
    new->last_eval = 0.;

    return;
}

/* Return the evaluation of var <vname> */
float ssv_get_var_eval(char *vname)
{
    soundscript_var v = g_hash_table_lookup(vartab, vname);
    assert(v);
    return v->last_eval;
}

/* Return variable by name */
soundscript_var ssv_get_var(char *vname)
{
    soundscript_var v = g_hash_table_lookup(vartab, vname);
    return v;
}

/* Setup dummy variable
 *
 * This function is used to support recursive definitions.
 *
 * XXX: Changing this function to something else is complex
 *      since there is no reliable way to handle failed assignments
 *      and garbage collect useless dummies. Therefore, every
 *      variable assignment to a previously non-existent variable
 *      will currently result in that variable being initialized with 0
 *      when the actual assignment fails.
 */
void ssv_set_dummy(char *vname)
{
    if (g_hash_table_lookup(vartab, vname) == NULL)
        ssv_set_var(vname, soundscript_mark_use(ssb_number(0.f)));

    return;
}

/* Compare two graphs' variable usage
 *
 * Purpose: This function is a helper used to qsort()
 *          the variable evaluation order.
 *
 * Returns:
 *      -1 g1 is used by g2 and must be evaluted earlier than g1
 *       0 g1 and g2 form a circular dependency or don't reference each other
 *       1 g1 uses g2 and must be evaluated later.
 */
static int _compare_graphs(const void *g1, const void *g2)
{
    int r;
    soundscript_var
        mod1 = *(soundscript_var*)g1,
        mod2 = *(soundscript_var*)g2;

    r = ssv_makes_use_of(mod1, mod2);
    if (r) {
        /* Circular/Mutual use */
        if (r == SSV_USAGE_CIRCULAR)
            return 0;

        /* Non-circular usage, g1 only uses g2, evaluate later */
        else
            return 1;
    }

    /* Perhaps g2 uses g1, evaluate earlier */
    if (ssv_makes_use_of(mod2, mod1) == SSV_USAGE_ONEWAY)
        return -1;

    /* No usage at all */
    return 0;
}

/* This function checks the usage of a mod2 by mod1
 *
 * mod2 may be NULL to check purely check for cycles in the graph of mod1.
 *
 * returns:
 *      SSV_USAGE_NONE:     mod1 does not depend on mod2.
 *      SSV_USAGE_ONEWAY:   mod1 uses mod2.
 *      SSV_USAGE_CIRCULAR: mod1 uses mod2,
 *                          but mod2 eventually references back to mod1.
 */
int ssv_makes_use_of(soundscript_var mod1, soundscript_var mod2)
{
    int usage = 0x0;
    GList *list, *iter;
    soundscript_var v;

    /* Mark are variables used (directly or indirectly) by mod1 */
    ssv_recursively_mark_vars(mod1);

    iter = list = g_hash_table_get_values(vartab);

    /* Check all variables and unmark them */
    while (iter) {
        v = (soundscript_var)g_list_nth_data(iter, 0);

        if (v->mark & 0x2) {
            /* Non circular usage */
            if (v == mod2)
                usage |= 1;

            /* Circular usage */
            if (v == mod1)
                usage |= 2;
        }

        /* Unmark variable */
        v->mark &= ~0x3;

        /* grab next item in iter */
        iter = g_list_next(iter);
    }

    /* Free iterated list */
    g_list_free(list);

    if (usage > 1)
        return SSV_USAGE_CIRCULAR;

    if (usage)
        return SSV_USAGE_ONEWAY;

    return SSV_USAGE_NONE;
}

/* Check if assigning graph to var vname would cause a cycle
 *
 * A value of 1 will mean the assignment causes a cycle.
 */
int ssv_speculate_cycle(char *vname, msynth_modifier graph)
{
    soundscript_var var;
    int usage;

    var = g_hash_table_lookup(vartab, vname);
    
    /* The variable does not exist yet and can therefore not cause a cycle */
    if (!var) {
        printf("spec: var does not exists, no cycle\n");
        return 0;
    }

    /* Let's replace the vaiable with our speculation variable */
    g_hash_table_insert(vartab, vname, NULL);
    ssv_set_var(vname, graph);

    /* Compute cycle */
    usage = ssv_makes_use_of(ssv_get_var(vname), NULL);
    printf("spec: usage %i\n", usage);

    /* Restore old variable */
    free(ssv_get_var(vname));
    g_hash_table_insert(vartab, vname, var);

    if (usage == SSV_USAGE_CIRCULAR)
        printf("spec: Cycle detected returning 1.\n");
    return usage == SSV_USAGE_CIRCULAR;
}

/* Recursively mark variables used by variable
 *
 * NOTE: This function does not 0x2 mark variables, since doing so
 *       would cause the function to be useless for cycle detection.
 * NOTE2:
 *      This function should only modify the first 2 bits of var->mark;
 */
void ssv_recursively_mark_vars(soundscript_var var)
{
    /* This variable was processed already */
    if (var->mark & 0x1)
        return;

    /* Mark as touched (prevents infinite recursion) */
    var->mark |= 0x1;

    _ssv_recursively_mark_graphs(var->vargraph);
    return;
}

/* Recursively mark variables used in sound graph */
void _ssv_recursively_mark_graphs(msynth_modifier mod)
{
    soundscript_var var;

    switch(mod->type) {
        case MSMT_VARIABLE:
            var = ssv_get_var(mod->data.varname);
            assert(var);

            /* This is the actual usage mark used for cycle detection */
            var->mark |= 0x2;
            ssv_recursively_mark_vars(var);
            break;

        case MSMT_NODE1:
            _ssv_recursively_mark_graphs(mod->data.node.in);
            break;

        case MSMT_NODE2:
            _ssv_recursively_mark_graphs(mod->data.node2.a);
            _ssv_recursively_mark_graphs(mod->data.node2.b);
            break;

        default:;
    }

    return;
}

/* Mark all immediately reached vars */
void _ssv_recursively_mark_immediate_graphs(msynth_modifier mod)
{
    soundscript_var var;

    switch(mod->type) {
        case MSMT_VARIABLE:
            var = ssv_get_var(mod->data.varname);
            assert(var);

            /* This is the actual usage mark used for cycle detection */
            var->mark |= 0x4;
            break;

        case MSMT_NODE1:
            _ssv_recursively_mark_graphs(mod->data.node.in);
            break;

        case MSMT_NODE2:
            _ssv_recursively_mark_graphs(mod->data.node2.a);
            _ssv_recursively_mark_graphs(mod->data.node2.b);
            break;

        default:;
    }

    return;
}

/* Recursively find all variables in a cycle
 *
 * XXX: There are probably more efficient algorithms around to implement this,
 *      it might be worth investigating various cycle detection algorithms.
 *
 * This function first marks all variables reachable from a given variable
 * afterwards eliminating all variables not looping back to this variable.
 *
 * This function returns nothing, but leaves the variables mark members intact
 * after completion.
 *
 * NOTE: This function assumes there is an actual cycle.
 *
 * NOTE: This function requires all mark members to be 0 to return the correct
 *       results. Failure to do so will lead to incorrect results, although will
 *       not crash the program.
 */
void ssv_mark_cycle_vars(soundscript_var cvar)
{
    GList *list, *iter;
    soundscript_var v;

    /* First begin with marking all vars reachable from 'var' */
    _ssv_recursively_mark_immediate_graphs(cvar->vargraph);

    /* Now check all vars partaking in the cycle */
    iter = list = g_hash_table_get_values(vartab);

    /* Convert all touched marks to persistent test marks. */
    while (iter) {
        v = (soundscript_var)g_list_nth_data(iter, 0);

        /* Only check required vars */
        if (v->mark & 0x4) {
            /* Mark v to check if it links back to cvar */
            ssv_recursively_mark_vars(v);

            /* Did we reach cvar */
            if (cvar->mark & 0x2) {
                /* Set cycle member flag */
                v->mark |= 0x8;
            }

            ssv_clear_marks(0x3);
        }

        /* grab next item in iter */
        iter = g_list_next(iter);
    }

    /* Free iterated list */
    g_list_free(list);

    return;
}

/* Clear any active variable markings
 *
 * clear: should be a mask of bits to clear (0x1 means clear bit 1)
 */
void ssv_clear_marks(unsigned int clear)
{
    GList *list, *iter;
    soundscript_var v;

    iter = list = g_hash_table_get_values(vartab);

    /* Unmark all variables */
    while (iter) {
        v = (soundscript_var)g_list_nth_data(iter, 0);

        /* Unmark variable */
        v->mark &= ~clear;

        /* grab next item in iter */
        iter = g_list_next(iter);
    }

    /* Free iterated list */
    g_list_free(list);
}

/* Regroup variables */
void ssv_regroup()
{
    int i = 0;
    GList *iter, *list;

    free(eval_list);
    eval_list = calloc(g_hash_table_size(vartab),sizeof(soundscript_var));

    iter = list = g_hash_table_get_values(vartab);

    /* Fetch all variables and store them in evaluation array */
    while (iter) {
        /* Store var in array */
        eval_list[i++] = (soundscript_var)g_list_nth_data(iter, 0);

        /* grab next item in iter */
        iter = g_list_next(iter);
    }
    g_list_free(list);

    /* Now sort array using quicksort based on dependencies */
    qsort(eval_list, g_hash_table_size(vartab),
        sizeof(soundscript_var), _compare_graphs);

    return;
}

/* Evaluate variables */
void ssv_eval(struct sampleclock sc)
{
    int i = 0, size = g_hash_table_size(vartab);

    for (; i < size; i++) {
        soundscript_var v = eval_list[i];
        v->last_eval = synth_eval(v->vargraph, sc);
    }

    return;
}

