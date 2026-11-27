/*
 * func_800AEB28
 *
 * Purpose:
 *   GPU driver dispatch wrapper (likely PsyQ PutDrawEnv / PutDispEnv-style helper).
 *   If debug verbosity D_800CEAC6 >= 2, logs format string D_80014FA8 with `arg0`
 *   via callback D_800CEAC0. Then invokes the GPU table function at
 *   *(D_800CEABC + 0x08) passing (*(D_800CEABC + 0x18), arg0, 0, 0).
 *
 * Arguments:
 *   arg0 (void *): environment/descriptor pointer forwarded to the GPU callback
 *
 * Return value:
 *   signed int: return value from the GPU dispatch callback
 *
 * Global fields read:
 *   D_800CEAC6 (unsigned char): debug/verbosity level
 *   D_80014FA8 (char[]): debug format string
 *   D_800CEAC0 (void (*)(void *, void *)): debug printf-style callback
 *   D_800CEABC (unsigned char *): GPU control/dispatch table pointer:
 *     +0x08: function pointer(ctx, arg0, 0, 0)
 *     +0x18: context/handle passed as first argument
 */

#define W(p, o) (*(unsigned int *)((unsigned char *)(p) + (o)))

extern unsigned char D_800CEAC6;
extern char D_80014FA8[];
extern void (*D_800CEAC0)(void *fmt, void *arg);
extern unsigned char *D_800CEABC;

typedef signed int (*gpu_env_fn_t)(unsigned int ctx, void *arg0, signed int arg2, signed int arg3);

signed int func_800AEB28(void *arg0)
{
    unsigned char *gpu_tbl;
    gpu_env_fn_t fn;

    if (D_800CEAC6 >= 2) {
        D_800CEAC0(D_80014FA8, arg0);
    }

    gpu_tbl = D_800CEABC;
    /* UNCLEAR: exact GPU operation performed by D_800CEABC+0x08 callback */
    fn = (gpu_env_fn_t)W(gpu_tbl, 0x08);
    return fn(W(gpu_tbl, 0x18), arg0, 0, 0);
}
