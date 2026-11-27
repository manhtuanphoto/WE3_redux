/*
 * func_8001B894
 *
 * Purpose:
 *   Computes a camera/screen vertical projection offset for a distant angle
 *   target (e.g., radar/camera elevation/horizon clamp), updating D_800E6C10
 *   periodically during open play, building a world-to-view matrix in
 *   Scratchpad+0x104 via GTE matrix composition, and returning the projected Y
 *   clamped to [-288, 160].
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   int (s16): projected vertical coordinate clamped to [-0x120, 0xA0] (-288..160).
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x00E (s16) base height Y
 *   +0x0F8 (s16), +0x0FA (s16), +0x0FC (s16) scratch target xyz
 *   +0x104..+0x123 (32 bytes) GTE matrix (R + t)
 *   +0x124..+0x129 (3x s16) Euler rotation (zeroed)
 *   +0x13C, +0x140, +0x144 (3x s32) translation vector
 *   +0x14C, +0x150, +0x154 (3x s32) scale vector (0x1000 = 1.0)
 *   +0x1A8..+0x1C7 (32 bytes) camera/view matrix (R + t)
 *   +0x1F0 (s32), +0x1F8 (s32) reference X/Z coordinates
 *   +0x290 (u32) frame counter
 *   +0x29A (u8)  match phase (1 = open play; updates D_800E6C10 every 10 frames)
 *   +0x29C (u8)  camera/replay sub-flag (< 5 forces D_800E6C10 refresh)
 *   +0x32C (s32) GTE projection distance H
 *
 * Scratchpad fields written:
 *   +0x0F8, +0x0FA, +0x0FC (s16) target xyz
 *   +0x104..+0x123 composed GTE matrix
 *   +0x124..+0x128 (s16) cleared to 0
 *   +0x13C, +0x140, +0x144 (s32) translation
 *   +0x14C, +0x150, +0x154 (s32) set to 0x1000
 *
 * Global fields read/written:
 *   D_800E6C10 (u8) heading angle byte updated by func_8009F348()
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   10 frames = 0.333 s refresh interval in open play;
 *   -0x7FFF (-32767) ray radius = 131.068 m;
 *   0x1000 (4096) fixed-point 1.0 / distance threshold (16.384 m);
 *   0x200 (512) minimum Z depth clamp;
 *   [-0x120, 0xA0] = [-288, 160] screen Y clamp.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800E6C10;

extern unsigned int func_8009F348(void);
extern int func_8001E694(unsigned int angle, int scale);
extern int func_8001E5A4(unsigned int angle, int scale);
extern unsigned int func_800B1128(unsigned int val);
extern void func_800B2178(unsigned char *rot, unsigned char *mat);
extern void func_800B1348(unsigned char *mat, unsigned char *scale);
extern void func_800B1318(unsigned char *mat, unsigned char *trans);

int func_8001B894(void)
{
    int dx;
    int dz;
    unsigned int dist;
    int tz;
    int ty;
    int h;
    signed short projY;
    int col;

    if (B(Scratchpad, 0x29C) >= 5) {
        switch (B(Scratchpad, 0x29A)) {
        case 1:
            if ((SW(Scratchpad, 0x290) % 10) != 0) {
                goto skip_angle_refresh;
            }
            break;
        default:
            break;
        }
    }
    D_800E6C10 = (unsigned char)func_8009F348();

skip_angle_refresh:
    S(Scratchpad, 0x0F8) = (signed short)func_8001E694(D_800E6C10, -0x7FFF);
    S(Scratchpad, 0x0FC) = (signed short)func_8001E5A4(D_800E6C10, -0x7FFF);

    dx = SW(Scratchpad, 0x1F0) - S(Scratchpad, 0x0F8);
    dz = SW(Scratchpad, 0x1F8) - S(Scratchpad, 0x0FC);
    S(Scratchpad, 0x0FA) = S(Scratchpad, 0x00E);

    dist = func_800B1128((unsigned int)(dx * dx + dz * dz));
    if (dist < 0x1000U) {
        S(Scratchpad, 0x0FA) = (signed short)(S(Scratchpad, 0x0FA) + ((0x1000U - dist) >> 4));
    }

    SW(Scratchpad, 0x14C) = 0x1000;
    SW(Scratchpad, 0x150) = 0x1000;
    SW(Scratchpad, 0x154) = 0x1000;
    S(Scratchpad, 0x124) = 0;
    S(Scratchpad, 0x126) = 0;
    S(Scratchpad, 0x128) = 0;
    SW(Scratchpad, 0x13C) = S(Scratchpad, 0x0F8);
    SW(Scratchpad, 0x140) = S(Scratchpad, 0x0FA);
    SW(Scratchpad, 0x144) = S(Scratchpad, 0x0FC);

    func_800B2178(Scratchpad + 0x124, Scratchpad + 0x104);
    func_800B1348(Scratchpad + 0x104, Scratchpad + 0x14C);
    func_800B1318(Scratchpad + 0x104, Scratchpad + 0x13C);

    /* Inline GTE matrix-matrix multiply & vector transform:
     * M_104 = RT_1A8 * M_104, t_118 = TR_1BC + RT_1A8 * t_118 (>> 12 fixed-point) */
    for (col = 0; col < 3; col++) {
        signed short m0 = S(Scratchpad, 0x104 + col * 2);
        signed short m1 = S(Scratchpad, 0x10A + col * 2);
        signed short m2 = S(Scratchpad, 0x110 + col * 2);

        S(Scratchpad, 0x104 + col * 2) = (signed short)((
            S(Scratchpad, 0x1A8) * m0 + S(Scratchpad, 0x1AA) * m1 + S(Scratchpad, 0x1AC) * m2) >> 12);
        S(Scratchpad, 0x10A + col * 2) = (signed short)((
            S(Scratchpad, 0x1AE) * m0 + S(Scratchpad, 0x1B0) * m1 + S(Scratchpad, 0x1B2) * m2) >> 12);
        S(Scratchpad, 0x110 + col * 2) = (signed short)((
            S(Scratchpad, 0x1B4) * m0 + S(Scratchpad, 0x1B6) * m1 + S(Scratchpad, 0x1B8) * m2) >> 12);
    }

    {
        signed short tx = (signed short)SW(Scratchpad, 0x118);
        signed short ty0 = (signed short)SW(Scratchpad, 0x11C);
        signed short tz0 = (signed short)SW(Scratchpad, 0x120);

        SW(Scratchpad, 0x118) = SW(Scratchpad, 0x1BC) +
            ((S(Scratchpad, 0x1A8) * tx + S(Scratchpad, 0x1AA) * ty0 + S(Scratchpad, 0x1AC) * tz0) >> 12);
        SW(Scratchpad, 0x11C) = SW(Scratchpad, 0x1C0) +
            ((S(Scratchpad, 0x1AE) * tx + S(Scratchpad, 0x1B0) * ty0 + S(Scratchpad, 0x1B2) * tz0) >> 12);
        SW(Scratchpad, 0x120) = SW(Scratchpad, 0x1C4) +
            ((S(Scratchpad, 0x1B4) * tx + S(Scratchpad, 0x1B6) * ty0 + S(Scratchpad, 0x1B8) * tz0) >> 12);
    }

    tz = SW(Scratchpad, 0x120);
    if (tz < 0x201) {
        tz = 0x200;
    }
    ty = SW(Scratchpad, 0x11C);
    h = SW(Scratchpad, 0x32C);
    projY = (signed short)((ty * h) / tz);

    if (projY >= 0xA0) {
        projY = 0xA0;
    }
    if (projY < -0x120) {
        projY = -0x120;
    }
    return (int)projY;
}
