/*
 * func_800AD9E8
 *
 * Purpose:
 *   Initializes an extended 28-byte display environment struct (DISPENV +
 *   additional display fields at +0x14..+0x1B). Sets disp (x, y, w, h) at
 *   +0x00..+0x06, screen (x, y, 0, 0) at +0x08..+0x0E, clears flags at
 *   +0x10..+0x13 and +0x18..+0x1B, sets +0x16 = 1, +0x14 = 10 (0xA), and sets
 *   +0x17 to whether `h` fits within standard display height (< 0x121 for PAL
 *   or < 0x101 for NTSC via func_800BB22C()).
 *
 * Arguments:
 *   env (u8*) - pointer to 28-byte display environment struct
 *   x   (s16) - VRAM display X
 *   y   (s16) - VRAM display Y
 *   w   (s16) - display width
 *   h   (s16) - display height (passed on stack at sp+0x10 of caller)
 *
 * Return value:
 *   u8* - returns `env`
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define S(p, o) (*(signed short *)((p) + (o)))

extern int func_800BB22C(void);

unsigned char *func_800AD9E8(unsigned char *env, int x, int y, int w, int h)
{
    int isPal = func_800BB22C();

    S(env, 0x00) = (signed short)x;
    S(env, 0x02) = (signed short)y;
    S(env, 0x04) = (signed short)w;
    S(env, 0x06) = (signed short)h;

    S(env, 0x0C) = 0;
    S(env, 0x0E) = 0;
    S(env, 0x10) = 0;
    S(env, 0x12) = 0;

    B(env, 0x19) = 0;
    B(env, 0x1A) = 0;
    B(env, 0x1B) = 0;
    B(env, 0x16) = 1;

    if (isPal != 0) {
        B(env, 0x17) = (unsigned char)(h < 0x121);
    } else {
        B(env, 0x17) = (unsigned char)(h < 0x101);
    }

    S(env, 0x08) = (signed short)x;
    S(env, 0x0A) = (signed short)y;
    S(env, 0x14) = 0xA;
    B(env, 0x18) = 0;

    return env;
}
