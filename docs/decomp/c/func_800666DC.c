/*
 * func_800666DC
 *
 * Purpose:
 *   Configures an actor's target/facing state relative to another actor in D_800FF748.
 *   Stores target_idx at actor+0x3C2, computes the angle/direction from actor (+0x02, +0x06)
 *   to target ((target_idx % 24) * 1000 in D_800FF748) via func_8005B36C and func_8005E0E8,
 *   writes the result to actor+0x3D4 and arg3 to actor+0x3D6. Updates actor+0x3C4 or halves
 *   D_8010C1DB (D_8010C1D8 + 3) if arg2 == 0xFF, then resets fields in D_8010C1D8 and
 *   increments D_8010C1DB.
 *
 * Arguments:
 *   actor (unsigned char *): pointer to actor struct (1000-byte stride in D_800FF748)
 *   target_idx (unsigned char): target actor index (wrapped mod 24)
 *   param2 (unsigned char): stored at actor+0x3C4 unless 0xFF (halves D_8010C1DB instead)
 *   param3 (signed short): stored at actor+0x3D6
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800FF748 (actor table base, 1000-byte stride)
 *   D_8010C1D8 (struct: +0x03 u8 == D_8010C1DB, +0x14 u8, +0x15 u8, +0x28 u32)
 *   D_8010C1DB (u8, alias for D_8010C1D8 + 3)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define SH(p, o) (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))

extern unsigned char D_800FF748[];
extern unsigned char D_8010C1D8[];
extern unsigned char D_8010C1DB;

extern unsigned int func_8005B36C(signed short x0, signed short z0, signed short x1, signed short z1);
extern signed short func_8005E0E8(unsigned int angle);

void func_800666DC(unsigned char *actor, unsigned char target_idx, unsigned char param2, signed short param3)
{
    unsigned int idx = target_idx & 0xFF;
    unsigned char *target = D_800FF748 + (idx % 24U) * 1000U;
    unsigned int dir;
    signed short mapped_dir;

    SH(actor, 0x3C2) = (signed short)idx;

    dir = func_8005B36C(SH(actor, 0x02), SH(actor, 0x06), SH(target, 0x02), SH(target, 0x06));
    mapped_dir = func_8005E0E8(dir & 0xFF);

    SH(actor, 0x3D6) = param3;
    SH(actor, 0x3D4) = mapped_dir;

    if ((param2 & 0xFF) == 0xFF) {
        D_8010C1DB = (unsigned char)(D_8010C1DB >> 1);
    } else {
        SH(actor, 0x3C4) = (signed short)(param2 & 0xFF);
    }

    W(D_8010C1D8, 0x28) = 0;
    B(D_8010C1D8, 0x14) = 0;
    B(D_8010C1D8, 0x15) = 0;
    B(D_8010C1D8, 0x03) = (unsigned char)(B(D_8010C1D8, 0x03) + 1);
}
