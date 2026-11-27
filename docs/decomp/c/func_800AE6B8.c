/*
 * func_800AE6B8
 *
 * Purpose:
 *   Calls func_800AE59C with D_80014F3C and arg0, packs the low 8 bits of
 *   (arg1, arg2, arg3) into a 24-bit word (arg1 | (arg2 << 8) | (arg3 << 16)),
 *   and invokes the callback stored at D_800CEABC->fn (+0x08) with
 *   (D_800CEABC->ctx (+0x0C), arg0, 8, packed).
 *
 * Arguments:
 *   arg0 (unsigned int) - passed to func_800AE59C and callback as 2nd arg
 *   arg1 (unsigned int) - low byte packed into bits [7:0]
 *   arg2 (unsigned int) - low byte packed into bits [15:8]
 *   arg3 (unsigned int) - low byte packed into bits [23:16]
 *
 * Return value:
 *   UNCLEAR: returns whatever the indirect call D_800CEABC->fn leaves in $v0 (or void).
 *
 * Global fields read:
 *   D_80014F3C (unknown data/string)
 *   D_800CEABC (struct pointer with callback at +0x08 and context at +0x0C)
 */

struct D_800CEABC_Ctx {
    unsigned int unk00;
    unsigned int unk04;
    int (*fn)(unsigned int ctx, unsigned int arg0, int cmd, unsigned int packed);
    unsigned int ctx;
};

extern unsigned char D_80014F3C[];
extern struct D_800CEABC_Ctx *D_800CEABC;

extern void func_800AE59C(unsigned char *arg0, unsigned int arg1);

int func_800AE6B8(unsigned int arg0, unsigned int arg1, unsigned int arg2, unsigned int arg3)
{
    unsigned int packed;
    struct D_800CEABC_Ctx *obj;

    func_800AE59C(D_80014F3C, arg0);

    packed = ((arg3 & 0xFFU) << 16) | ((arg2 & 0xFFU) << 8) | (arg1 & 0xFFU);
    obj = D_800CEABC;

    /* UNCLEAR: exact return type/semantics of D_800CEABC->fn (+0x08) */
    return obj->fn(obj->ctx, arg0, 8, packed);
}
