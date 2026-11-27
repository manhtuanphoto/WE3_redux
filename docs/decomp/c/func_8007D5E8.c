/*
 * func_8007D5E8
 *
 * Purpose:
 *   Match stoppage / post-whistle / set-piece transition tick.
 *   Updates all 22 player actors in D_800FF748[1..22] (1000-byte stride):
 *   triggers reactions via func_800537D0 / func_8005BA30 and handles the
 *   player matching Scratchpad+0x2A8 (calling func_8004B0F0 and SFX 0x509).
 *   Marks the 5 closest players to actor (Scratchpad+0x2AD % 24) via func_800477A0,
 *   updates ball/camera/animation helpers, and advances the state timer at
 *   Scratchpad+0x294 (transitioning via func_800171FC(3) after 30+ frames if
 *   func_800525A4() != 0, or func_800171FC(2) at 121 frames).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read / written:
 *   D_800FF748 (actor table base, 1000-byte stride)
 *     +0x38D (u8) player ID
 *     +0x38E (u16) state/action
 *     +0x396 (u8) = 0x33
 *     +0x397 (u8) = 0
 *     +0x3A0 (u32) anim/state value passed +8 to func_8004B0F0
 *     +0x3AA (u8) param passed to func_8004B0F0
 *
 * Scratchpad (0x1F800000) fields read / written:
 *   +0x294 (u16) frame timer (incremented each call; reset to 0 at 121)
 *   +0x2A8 (u8) target player ID
 *   +0x2AD (u8) focus actor index (mod 24)
 *   +0x2C0 (u8) = 0 when +0x294 >= 30
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];

extern int func_800537D0(unsigned char *actor);
extern unsigned int func_8001E6B8(unsigned int range);
extern void func_8005BA30(unsigned char *actor, unsigned int arg1, unsigned int arg2);
extern void func_8004B0F0(unsigned char *actor, unsigned int arg1, unsigned int arg2, unsigned int arg3);
extern void func_800AB620(unsigned int sfx);
extern void func_800477A0(const unsigned char *ref, unsigned int count);
extern void func_8002283C(void);
extern void func_8004A2D0(void);
extern void func_80051810(void);
extern int func_800525A4(void);
extern void func_8005C8D0(unsigned char *actor, unsigned int arg1, unsigned int arg2);
extern void func_800171FC(unsigned int state);

void func_8007D5E8(void)
{
    unsigned char *actor;
    unsigned int i;
    unsigned int focus_idx;
    unsigned short timer;

    actor = D_800FF748 + 0x3E8;
    for (i = 0; i < 22; i++, actor += 0x3E8) {
        if (func_800537D0(actor) != 0) {
            if (H(actor, 0x38E) != 1) {
                unsigned int rnd = func_8001E6B8(0x15);
                func_8005BA30(actor, 1, rnd & 0xFF);
            }
            B(actor, 0x396) = 0x33;
            B(actor, 0x397) = 0;
        }

        if (B(actor, 0x38D) == B(Scratchpad, 0x2A8)) {
            func_8004B0F0(actor, B(actor, 0x3AA), W(actor, 0x3A0) + 8, 0);
            func_800AB620(0x509);
        }
    }

    focus_idx = (unsigned int)B(Scratchpad, 0x2AD) % 24U;
    func_800477A0(D_800FF748 + focus_idx * 1000U, 5);

    func_8002283C();
    func_8004A2D0();
    func_80051810();

    timer = (unsigned short)(H(Scratchpad, 0x294) + 1);
    H(Scratchpad, 0x294) = timer;
    if (timer < 0x1E) {
        return;
    }

    B(Scratchpad, 0x2C0) = 0;
    if (func_800525A4() != 0) {
        func_800AB620(0x11);
        func_800171FC(3);
        return;
    }

    if (H(Scratchpad, 0x294) >= 0x79) {
        focus_idx = (unsigned int)B(Scratchpad, 0x2AD) % 24U;
        H(Scratchpad, 0x294) = 0;
        func_8005C8D0(D_800FF748 + focus_idx * 1000U, 0, 0x1A);
        func_800171FC(2);
    }
}
