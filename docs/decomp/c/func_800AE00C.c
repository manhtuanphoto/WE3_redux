/*
 * func_800AE00C
 *
 * Purpose:
 *   Debug-dumps a 24-byte PsyQ DRAWENV structure via the printf callback
 *   D_800CEAC0 using format strings D_80014D84, D_80014D9C, D_80014DAC,
 *   D_80014DC4, D_80014DD0, and D_80014D5C.
 *
 * Arguments:
 *   env (unsigned char *): pointer to 24-byte DRAWENV-like struct:
 *                          +0x00..+0x07: s16 clip {x, y, w, h}
 *                          +0x08..+0x0B: s16 ofs {x, y}
 *                          +0x0C..+0x13: s16 tw {x, y, w, h}
 *                          +0x14: u16 tpage (TP, ABR, TX*64, TY*256 bitfields)
 *                          +0x16: u8 dtd
 *                          +0x17: u8 dfe
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800CEAC0 (int (*)(const char *, ...)): printf-style output function pointer
 *   D_80014D84, D_80014D9C, D_80014DAC, D_80014DC4, D_80014DD0, D_80014D5C: format strings
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define SH(p, o) (*(signed short *)((p) + (o)))

extern int (*D_800CEAC0)(const char *fmt, ...);
extern const char D_80014D84[];
extern const char D_80014D9C[];
extern const char D_80014DAC[];
extern const char D_80014DC4[];
extern const char D_80014DD0[];
extern const char D_80014D5C[];

void func_800AE00C(unsigned char *env)
{
    unsigned short tpage;
    unsigned int tp;
    unsigned int abr;
    unsigned int tx;
    unsigned int ty;

    D_800CEAC0(D_80014D84, SH(env, 0x00), SH(env, 0x02), SH(env, 0x04), SH(env, 0x06));
    D_800CEAC0(D_80014D9C, SH(env, 0x08), SH(env, 0x0A));
    D_800CEAC0(D_80014DAC, SH(env, 0x0C), SH(env, 0x0E), SH(env, 0x10), SH(env, 0x12));
    D_800CEAC0(D_80014DC4, B(env, 0x16));
    D_800CEAC0(D_80014DD0, B(env, 0x17));

    tpage = H(env, 0x14);
    tp = (tpage >> 7) & 0x3u;
    abr = (tpage >> 5) & 0x3u;
    tx = (tpage << 6) & 0x7C0u;
    ty = ((tpage << 4) & 0x100u) + ((tpage >> 2) & 0x200u);
    D_800CEAC0(D_80014D5C, tp, abr, tx, ty);
}
