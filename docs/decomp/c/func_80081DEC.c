/*
 * func_80081DEC
 *
 * Purpose:
 *   Resets the ball near the left goal/penalty area (-10480 - rand(21), -32, 0)
 *   and arranges all 22 players in a scattered cluster facing (-13088, 0) with
 *   celebration/cutscene animation (0x46 or 0x47) and state 0x36.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800FF748 (actor table base, 1000-byte / 0x3E8 stride, 22 actors starting at +0x3E8)
 *   D_8010A400 (u8) if >= 100 (0x64), sets actor+0x39A = 1
 *
 * Ball fields written:
 *   +0x02 (s16) X, +0x04 (s16) Y = -0x20, +0x06 (s16) Z = 0
 *   +0x24/+0x26/+0x28 (s16) velocity = 0
 *   +0x396 (u8) = 0, +0x3A4 (u8) = 0x80, +0x3B0 (u8) = 0
 *
 * Actor fields read/written (for i = 0..21):
 *   +0x002 (s16) X, +0x006 (s16) Z, +0x026 (s16) velocity/offset
 *   +0x396 (u8) = 0x36, +0x397 (u8) = 0, +0x398 (u8) team (R)
 *   +0x39A (u8) = 1 if D_8010A400 >= 100
 *   +0x3A5 (u8) = 0x0C, +0x3A6 (u8) = 0x18, +0x3AA (u8) facing angle
 *   +0x3C4 (u16) = Z, +0x3D3 (u8) = 0, +0x3E2 (u8) = 5, +0x3E3 (u8) = 0
 *
 * Scratchpad (0x1F800000) fields read/written:
 *   +0x2A8 (u8) = 0, +0x2A9 (u8) = 0, +0x2F4 (u32) ball pointer (R)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010A400;

extern int func_8001E6B8(int max);
extern signed short func_8005C7A0(int range);
extern void func_8005BA30(unsigned char *actor, unsigned int anim, unsigned int param2);
extern unsigned int func_8005B36C(signed short x1, signed short z1, signed short x2, signed short z2);

void func_80081DEC(void)
{
    unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
    unsigned char *actor = D_800FF748 + 0x3E8;
    int i;

    S(ball, 0x24) = 0;
    S(ball, 0x26) = 0;
    S(ball, 0x28) = 0;

    S(ball, 2) = (signed short)(-0x28F0 - func_8001E6B8(0x15));
    S(ball, 4) = -0x20;
    S(ball, 6) = 0;
    B(ball, 0x3A4) = 0x80;

    B(Scratchpad, 0x2A9) = 0;
    B(Scratchpad, 0x2A8) = 0;
    B(ball, 0x396) = 0;
    B(ball, 0x3B0) = 0;

    for (i = 0; i < 0x16; i++, actor += 0x3E8) {
        unsigned int anim;
        unsigned int ang;
        signed int diff;
        signed int rem;

        S(actor, 2) = func_8005C7A0(0x480);
        if (B(actor, 0x398) == 0) {
            S(actor, 6) = (signed short)(func_8005C7A0(0x480) + 0x400);
        } else {
            S(actor, 6) = (signed short)(func_8005C7A0(0x480) - 0x400);
        }

        H(actor, 0x3C4) = H(actor, 6);
        anim = (func_8001E6B8(2) != 0) ? 0x46U : 0x47U;
        func_8005BA30(actor, anim, 0);

        ang = func_8005B36C(S(actor, 2), S(actor, 6), -0x3320, 0);
        ang += (unsigned int)func_8005C7A0(8);
        B(actor, 0x3AA) = (unsigned char)ang;

        diff = 0x1C0 - (signed int)(ang & 0xFFU);
        rem = diff - (((diff >= 0 ? diff : diff + 0xFF) >> 8) << 8);

        B(actor, 0x3A5) = 0xC;
        B(actor, 0x3A6) = 0x18;
        B(actor, 0x396) = 0x36;
        B(actor, 0x3E2) = 5;
        B(actor, 0x3D3) = 0;
        B(actor, 0x397) = 0;
        B(actor, 0x3E3) = 0;
        S(actor, 0x26) = (signed short)(rem << 4);

        if (D_8010A400 >= 0x64) {
            B(actor, 0x39A) = 1;
        }
    }
}
