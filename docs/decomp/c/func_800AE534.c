/*
 * func_800AE534
 *
 * Purpose:
 *   GPU draw synchronization wrapper (PsyQ DrawSync-style).
 *   If debug verbosity D_800CEAC6 >= 2, logs format string D_80014F04 with
 *   `mode` via callback D_800CEAC0. Then invokes the GPU dispatch table
 *   callback at *(D_800CEABC + 0x3C)(mode) and returns its result.
 *
 * Arguments:
 *   mode (signed int): sync mode (0 = wait until drawing finishes, 1 = query)
 *
 * Return value:
 *   signed int: return value from (*(D_800CEABC + 0x3C))(mode)
 *
 * Global fields read:
 *   D_800CEAC6 (unsigned char): debug/verbosity level
 *   D_80014F04 (char[]): debug format string
 *   D_800CEAC0 (void (*)(void *, signed int)): debug printf-style callback
 *   D_800CEABC (unsigned char *): GPU control/dispatch table pointer (+0x3C function ptr)
 */

#define W(p, o) (*(unsigned int *)((unsigned char *)(p) + (o)))

extern unsigned char D_800CEAC6;
extern char D_80014F04[];
extern void (*D_800CEAC0)(void *fmt, signed int arg);
extern unsigned char *D_800CEABC;

typedef signed int (*gpu_sync_fn_t)(signed int mode);

signed int func_800AE534(signed int mode)
{
    gpu_sync_fn_t fn;

    if (D_800CEAC6 >= 2) {
        D_800CEAC0(D_80014F04, mode);
    }

    /* UNCLEAR: exact internal behavior of D_800CEABC+0x3C callback (DrawSync handler) */
    fn = (gpu_sync_fn_t)W(D_800CEABC, 0x3C);
    return fn(mode);
}
