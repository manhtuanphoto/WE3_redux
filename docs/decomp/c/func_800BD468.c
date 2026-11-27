/*
 * func_800BD468
 *
 * Purpose:
 *   SPU / sound driver voice allocation and playback start routines.
 *   Note: func_800BD468.s contains three consecutive routines before `endlabel func_800BD468`:
 *     1. func_800BD468: allocates a free voice via func_800BE028(), configures the 54-byte
 *        voice slot, and triggers key-on / pitch / pan.
 *     2. func_800BD7CC_unlabeled: stops or updates an existing voice slot [0..23] and clears
 *        its active flags.
 *     3. func_800BD918_unlabeled: starts playback on an explicitly specified voice slot [0..23].
 *
 * Arguments:
 *   func_800BD468(s16 p0, s16 p1, s16 p2, s16 p3, u16 p4, s16 p5, s16 p6)
 *   func_800BD7CC_unlabeled(s16 voice, s16 p1, s16 p2, s16 p3, s16 p4)
 *   func_800BD918_unlabeled(s16 voice, s16 p1, s16 p2, s16 p3, u16 p4, u16 p5, s16 p6, s16 p7)
 *
 * Return value:
 *   int - allocated/target voice index on success (or 0 in func_800BD7CC_unlabeled), -1 on
 *         re-entrancy lock (D_800F1014 == 1), invalid voice, or allocation failure.
 *
 * Reads / Writes:
 *   Globals:
 *     D_800F1014 (s32) re-entrancy lock (R/W)
 *     D_800F1098 (u8*) sound/program table pointer (R)
 *     D_800F20B0 (u8*) tone/ADSR table pointer (R)
 *     D_800FB578 (s8)  invalid/max voice sentinel (R)
 *     D_8010A3D2 (u8[]) / D_8010A3DE (u16) active parameter staging block (R/W)
 *     D_8010A3E2 (s16) voice id staging field (W)
 *     D_800E78D8..D_800E7902 (54-byte stride voice slots, 24 voices) (R/W)
 *
 * Magic numbers:
 *   0x18 = 24 SPU hardware voices; 54-byte voice slot stride; 0x21 = 33; 0x40 = 64 (center pan);
 *   0x7F = 127 (max pan/volume)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SB(p,o) (*(signed char*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern int D_800F1014;
extern unsigned char *D_800F1098;
extern unsigned char *D_800F20B0;
extern signed char D_800FB578;
extern unsigned char D_8010A3D2[];
extern unsigned char D_8010A3DE[];
extern short D_8010A3E2;

extern unsigned char D_800E78D8[];
extern unsigned char D_800E78DA[];
extern unsigned char D_800E78E6[];
extern unsigned char D_800E78E8[];
extern unsigned char D_800E78EA[];
extern unsigned char D_800E78EC[];
extern unsigned char D_800E78EE[];
extern unsigned char D_800E78F0[];
extern unsigned char D_800E78F5[];
extern unsigned char D_800E78F6[];
extern unsigned char D_800E7902[];

extern int func_800C0AB8(int, int);
extern int func_800BE028(void);
extern void func_800BE2B8(int);
extern void func_800BF758(int);
extern int func_800BF56C(unsigned int, unsigned int);
extern void func_800BFDE8(int, unsigned int);
extern void func_800BFCD8(unsigned int);
extern void func_800BFD18(int);

int func_800BD468(short arg0, short arg1, int arg2, int arg3, unsigned short arg4, short arg5, short arg6)
{
    unsigned char *prog_entry;
    unsigned char *tone_entry;
    int voice;
    int off;
    int ret;

    if (D_800F1014 == 1) {
        return -1;
    }
    D_800F1014 = 1;

    if (func_800C0AB8(arg0, arg1) != 0) {
        D_800F1014 = 0;
        return -1;
    }

    S(D_8010A3DE, 0) = 0x21;
    B(D_8010A3DE, -0x14) = (unsigned char)arg3;
    B(D_8010A3DE, -0x13) = (unsigned char)arg4;
    B(D_8010A3DE, -0xA) = (unsigned char)arg2;

    if (arg5 == arg6) {
        B(D_8010A3DE, -0x12) = (unsigned char)arg5;
        B(D_8010A3DE, -0x11) = 0x40;
    } else if (arg6 < arg5) {
        B(D_8010A3DE, -0x12) = (unsigned char)arg5;
        /* Explicit zero-divisor / INT_MIN checks in MIPS div preserved by C / */
        B(D_8010A3DE, -0x11) = (unsigned char)((arg6 << 6) / arg5);
    } else {
        B(D_8010A3DE, -0x12) = (unsigned char)arg6;
        B(D_8010A3DE, -0x11) = (unsigned char)(0x7F - ((arg5 << 6) / arg6));
    }

    prog_entry = D_800F1098 + (arg1 << 4);
    B(D_8010A3D2, 0) = B(prog_entry, 1);
    B(D_8010A3D2, 1) = B(prog_entry, 4);
    B(D_8010A3D2, -0xA) = B(prog_entry, 0);

    tone_entry = D_800F20B0 + ((SB(D_8010A3D2, 2) + (SB(D_8010A3D2, -3) << 4)) << 5);
    B(D_8010A3D2, 5) = B(tone_entry, 0);
    H(D_8010A3D2, 0xE) = H(tone_entry, 0x16);
    B(D_8010A3D2, 3) = B(tone_entry, 2);
    B(D_8010A3D2, 4) = B(tone_entry, 3);
    B(D_8010A3D2, 6) = B(tone_entry, 4);
    B(D_8010A3D2, 7) = B(tone_entry, 5);
    B(D_8010A3D2, 0xA) = B(tone_entry, 1);
    B(D_8010A3D2, 8) = B(tone_entry, 6);
    B(D_8010A3D2, 9) = B(tone_entry, 7);

    if (S(D_8010A3D2, 0xE) == 0) {
        D_800F1014 = 0;
        return -1;
    }

    voice = func_800BE028() & 0xFF;
    if (voice == (int)D_800FB578) {
        D_800F1014 = 0;
        return -1;
    }

    off = voice * 54;
    S(D_8010A3D2, 0x10) = (short)voice;
    S(D_800E78E8, off) = 0x21;
    S(D_800E78F0, off) = arg0;
    S(D_800E78EC, off) = arg1;
    S(D_800E78EA, off) = (short)SB(D_8010A3D2, -3);
    H(D_800E78D8, off) = H(D_8010A3D2, 0xE);
    S(D_800E78E6, off) = (short)arg3;
    B(D_800E78F5, off) = 1;
    S(D_800E78DA, off) = 0;
    S(D_800E78EE, off) = (short)SB(D_8010A3D2, 2);

    func_800BE2B8((int)SB(D_8010A3D2, 2));

    if (S(D_8010A3D2, 0xE) == 0xFF) {
        func_800BF758(voice);
    } else {
        ret = func_800BF56C((unsigned int)arg3 & 0xFFFFU, arg4);
        func_800BFDE8(1, (unsigned int)ret & 0xFFFFU);
    }

    D_800F1014 = 0;
    return voice;
}

/* UNCLEAR: unlabeled function immediately following func_800BD468 in func_800BD468.s */
int func_800BD7CC_unlabeled(short voice, short p1, short p2, short p3, short p4)
{
    int off;

    if (D_800F1014 == 1) {
        return -1;
    }
    D_800F1014 = 1;

    if ((unsigned short)voice >= 0x18U) {
        D_800F1014 = 0;
        return -1;
    }

    off = voice * 54;
    if (S(D_800E78F0, off) == p1
        && S(D_800E78EC, off) == p2
        && S(D_800E78EE, off) == p3
        && S(D_800E78E6, off) == p4
        && S(D_800E78D8, off) == 0xFF) {
        func_800BFCD8((unsigned int)voice & 0xFFU);
    } else {
        D_8010A3E2 = voice;
        func_800BFD18(0);
    }

    S(D_800E7902, off) = 0;
    S(D_800E78F6, off) = 0;
    D_800F1014 = 0;
    return 0;
}

/* UNCLEAR: second unlabeled function before endlabel func_800BD468 in func_800BD468.s */
int func_800BD918_unlabeled(short voice, short p1, short p2, int p3, unsigned short p4, unsigned short p5, short p6, short p7)
{
    unsigned char *prog_entry;
    unsigned char *tone_entry;
    int off;
    int ret;

    if (D_800F1014 == 1) {
        return -1;
    }
    D_800F1014 = 1;

    if ((unsigned short)voice >= 0x18U) {
        D_800F1014 = 0;
        return -1;
    }

    if (func_800C0AB8(p1, p2) != 0) {
        D_800F1014 = 0;
        return -1;
    }

    S(D_8010A3DE, 0) = 0x21;
    B(D_8010A3DE, -0x14) = (unsigned char)p4;
    B(D_8010A3DE, -0x13) = (unsigned char)p5;
    B(D_8010A3DE, -0xA) = (unsigned char)p3;

    if (p6 == p7) {
        B(D_8010A3DE, -0x12) = (unsigned char)p6;
        B(D_8010A3DE, -0x11) = 0x40;
    } else if (p7 < p6) {
        B(D_8010A3DE, -0x12) = (unsigned char)p6;
        B(D_8010A3DE, -0x11) = (unsigned char)((p7 << 6) / p6);
    } else {
        B(D_8010A3DE, -0x12) = (unsigned char)p7;
        B(D_8010A3DE, -0x11) = (unsigned char)(0x7F - ((p6 << 6) / p7));
    }

    prog_entry = D_800F1098 + ((short)p2 << 4);
    B(D_8010A3D2, 0) = B(prog_entry, 1);
    B(D_8010A3D2, 1) = B(prog_entry, 4);
    B(D_8010A3D2, -0xA) = B(prog_entry, 0);

    tone_entry = D_800F20B0 + ((SB(D_8010A3D2, 2) + (SB(D_8010A3D2, -3) << 4)) << 5);
    B(D_8010A3D2, 5) = B(tone_entry, 0);
    H(D_8010A3D2, 0xE) = H(tone_entry, 0x16);
    B(D_8010A3D2, 3) = B(tone_entry, 2);
    B(D_8010A3D2, 4) = B(tone_entry, 3);
    B(D_8010A3D2, 6) = B(tone_entry, 4);
    B(D_8010A3D2, 7) = B(tone_entry, 5);
    B(D_8010A3D2, 0xA) = B(tone_entry, 1);
    B(D_8010A3D2, 8) = B(tone_entry, 6);
    B(D_8010A3D2, 9) = B(tone_entry, 7);

    if (S(D_8010A3D2, 0xE) == 0) {
        D_800F1014 = 0;
        return -1;
    }

    off = voice * 54;
    S(D_8010A3D2, 0x10) = voice;
    S(D_800E78E8, off) = 0x21;
    S(D_800E78F0, off) = p1;
    S(D_800E78EC, off) = p2;
    S(D_800E78EA, off) = (short)SB(D_8010A3D2, -3);
    H(D_800E78D8, off) = H(D_8010A3D2, 0xE);
    S(D_800E78E6, off) = (short)p4;
    B(D_800E78F5, off) = 1;
    S(D_800E78DA, off) = 0;
    S(D_800E78EE, off) = (short)SB(D_8010A3D2, 2);

    func_800BE2B8((int)SB(D_8010A3D2, 2));

    if (S(D_8010A3D2, 0xE) == 0xFF) {
        func_800BF758((unsigned int)voice & 0xFFU);
    } else {
        ret = func_800BF56C(p4, p5);
        func_800BFDE8(1, (unsigned int)ret & 0xFFFFU);
    }

    D_800F1014 = 0;
    return voice;
}
