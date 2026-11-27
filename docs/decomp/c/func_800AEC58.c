/*
 * func_800AEC58 - Initialize context block, update 24-bit tag, and invoke callback
 *
 * Purpose:
 *   Optionally logs via function pointer D_800CEAC0 when D_800CEAC6 >= 2,
 *   initializes `ctx + 0x1C` via func_800AF590(ctx + 0x1C, ctx), inserts the
 *   low 24 bits of `arg0` into `*(u32 *)(ctx + 0x1C)` while preserving the top
 *   byte, invokes callback `D_800CEABC->fn_08(D_800CEABC->arg_18, ctx + 0x1C, 0x40, 0)`,
 *   and calls func_800B1118(&D_800CEAC6 + 0xE, ctx, 0x5C).
 *
 * Arguments:
 *   arg0 (unsigned int): 24-bit value merged into *(u32 *)(ctx + 0x1C)
 *   ctx (unsigned char *): context/primitive buffer (at least 0x20+ bytes)
 *
 * Return value:
 *   None (void)
 *
 * Globals read:
 *   D_800CEAC6 (u8): debug/verbosity level; +0x0E passed to func_800B1118
 *   D_80014FD4 (const char *): format/log string passed to D_800CEAC0
 *   D_800CEAC0 (void (*)(const char *, u32, u8 *)): optional log callback
 *   D_800CEABC (u8 *): object with callback at +0x08 and first argument at +0x18
 */

#define W(p, o) (*(unsigned int *)((p) + (o)))

typedef void (*log_fn_t)(const char *, unsigned int, unsigned char *);
typedef void (*obj_fn_t)(unsigned int, unsigned char *, int, int);

extern unsigned char D_800CEAC6;
extern const char D_80014FD4[];
extern log_fn_t D_800CEAC0;
extern unsigned char *D_800CEABC;

extern void func_800AF590(unsigned char *dst, unsigned char *ctx);
extern void func_800B1118(unsigned char *a0, unsigned char *ctx, int size);

void func_800AEC58(unsigned int arg0, unsigned char *ctx)
{
    unsigned char *field_1C;
    unsigned char *obj;
    obj_fn_t fn;

    if (D_800CEAC6 >= 2u) {
        D_800CEAC0(D_80014FD4, arg0, ctx);
    }

    field_1C = ctx + 0x1C;
    func_800AF590(field_1C, ctx);

    W(ctx, 0x1C) = (W(ctx, 0x1C) & 0xFF000000u) | (arg0 & 0x00FFFFFFu);

    obj = D_800CEABC;
    fn = (obj_fn_t)W(obj, 0x08);
    fn(W(obj, 0x18), field_1C, 0x40, 0);

    func_800B1118(&D_800CEAC6 + 0xE, ctx, 0x5C);
}
