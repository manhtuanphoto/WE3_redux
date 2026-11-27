/*
 * func_8001A19C
 * Purpose: Builds an actor shadow/ground quad transform via PsyQ matrix functions and GTE
 *          (coprocessor 2), perspective-transforms 4 quad vertices (SCRATCH+0x248..0x264) into
 *          a primitive in D_800E8208, and inserts it into the ordering table D_800F3560.
 * Arguments:
 *   actor - pointer to 1000-byte actor struct
 * Return value:
 *   void
 * Scratchpad reads/writes:
 *   R: 0x00 (u32 OT slot index), 0x1A8..0x1C4 (view MATRIX), 0x248..0x264 (4 SVEC vertices),
 *      0x29D (u8 active double-buffer index)
 *   W: 0x104..0x120 (local/combined MATRIX), 0x124..0x128 (s16 rotation = 0,0,0),
 *      0x13C..0x144 (s32 translation = x, 0, z), 0x14C..0x154 (s32 scale = 0x1000,0x1000,0x1000),
 *      0x0EC (s32 OTZ >> 2)
 * Actor reads/writes:
 *   R: +0x02 (s16 x), +0x06 (s16 z)
 *   W: none
 * Constants:
 *   0x1000 = 1.0 in PS1 1.12 fixed point;
 *   0x3AC0 (15040 bytes) per-buffer primitive block stride in D_800E8208;
 *   0x4000 (16384 bytes = 4096 entries) per-buffer OT stride in D_800F3560.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCH  ((unsigned char *)0x1F800000)

extern unsigned char D_800E8208[];
extern unsigned char D_800F3560[];

extern void func_800B2178(unsigned char *rot, unsigned char *mat, unsigned char *actor);
extern void func_800B1348(unsigned char *mat, unsigned char *scale);
extern void func_800B1318(unsigned char *mat, unsigned char *trans);

void func_8001A19C(unsigned char *actor) {
    unsigned char *prim;
    unsigned char *ot_entry;
    unsigned int otz;
    unsigned int ot_off;

    S(SCRATCH, 0x124) = 0;
    S(SCRATCH, 0x126) = 0;
    S(SCRATCH, 0x128) = 0;

    SW(SCRATCH, 0x140) = 0;
    SW(SCRATCH, 0x13C) = S(actor, 2);
    SW(SCRATCH, 0x144) = S(actor, 6);

    SW(SCRATCH, 0x14C) = 0x1000;
    SW(SCRATCH, 0x150) = 0x1000;
    SW(SCRATCH, 0x154) = 0x1000;

    prim = D_800E8208 + B(SCRATCH, 0x29D) * 0x3AC0;

    /* Note: $a2 holds actor on entry to func_800B2178 */
    func_800B2178(SCRATCH + 0x124, SCRATCH + 0x104, actor);
    func_800B1348(SCRATCH + 0x104, SCRATCH + 0x14C);
    func_800B1318(SCRATCH + 0x104, SCRATCH + 0x13C);

    /*
     * GTE matrix multiply: combine view matrix at SCRATCH+0x1A8 with local matrix at SCRATCH+0x104
     *   gte_SetRotMatrix(SCRATCH + 0x1A8)
     *   rotate columns (0x104+0/6/C, 0x104+2/8/E, 0x104+4/A/10) via mvmva(1,0,3,3,0)
     *   gte_SetTransMatrix(SCRATCH + 0x1A8)
     *   transform translation at SCRATCH+0x118 via mvmva(1,0,0,0,0) -> store MAC1..3 back to 0x118
     *   gte_SetRotMatrix & gte_SetTransMatrix from SCRATCH+0x104
     *   gte_ldv3(SCRATCH+0x248, SCRATCH+0x250, SCRATCH+0x258); gte_rtpt()
     *   store SXY0, SXY1, SXY2 -> prim+0x08, prim+0x10, prim+0x18
     *   gte_ldv0(SCRATCH+0x260); gte_rtps()
     *   store SXY2 -> prim+0x20
     *   otz = gte_stSZ3() >> 2
     */
    /* UNCLEAR: literal GTE coprocessor 2 inline operations omitted in portable C */
    SW(SCRATCH, 0xEC) = 0; /* placeholder for (s32)SZ3 >> 2 */

    if (SW(SCRATCH, 0xEC) > 0) {
        ot_off = (B(SCRATCH, 0x29D) << 14) + (4u << W(SCRATCH, 0x00));
        ot_entry = D_800F3560 + ot_off;
        W(prim, 0) = (W(prim, 0) & 0xFF000000u) | (W(ot_entry, 0) & 0x00FFFFFFu);

        ot_off = (B(SCRATCH, 0x29D) << 14) + (4u << W(SCRATCH, 0x00));
        ot_entry = D_800F3560 + ot_off;
        W(ot_entry, 0) = (W(ot_entry, 0) & 0xFF000000u) | ((unsigned int)prim & 0x00FFFFFFu);
    }
}
