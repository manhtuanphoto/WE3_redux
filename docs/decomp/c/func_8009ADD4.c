/*
 * func_8009ADD4
 *
 * Purpose:
 *   Initializes 28 stadium camera-flash / light-flare entries (D_800E6E20..D_800E6E2D,
 *   stride 0x10 bytes) with 3D positions from the stadium table D_800CB434 (mirrored
 *   across 4 quadrants) and random/cycling flash flags, then initializes 2 display
 *   buffers of 28 sprite primitives (0x28 bytes each at *(Scratchpad+0x190)+0x3660)
 *   with texture/tpage setup and stadium/time-of-day RGB tinting.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global / Scratchpad (0x1F800000) fields read:
 *   Scratchpad +0x190 (u32) base pointer to frame/OT primitive buffer (2 buffers, stride 0x3AC0),
 *   Scratchpad +0x2E3 (u8) stadium index (0..5),
 *   Scratchpad +0x2E5 (u8) time-of-day / lighting variant (1 checked),
 *   D_800CB434 / D_800CB436 / D_800CB438 (s16 x/y/z entries, 14 entries * 6 bytes = 84 per stadium),
 *   D_800CAA7B / D_800CAA7C (u8 sprite/anim sequence table)
 *
 * Global fields written:
 *   D_800E6E20 / D_800E6E22 / D_800E6E24 (s16 x/y/z, stride 0x10, 28 entries),
 *   D_800E6E28 (u32 pointer, stride 0x10),
 *   D_800E6E2C (u8 variant flag 0/1, stride 0x10),
 *   D_800E6E2D (u8 active/phase flag 0/1, stride 0x10),
 *   Primitive buffer entries (+0x04..+0x06 RGB, +0x0E u16 flag, +0x16 u16 tpage)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   28 entries (0x1C), 2 double-buffers (stride 0x3AC0, offset +0x3660, 0x28 bytes each).
 *   TPage bounds: (1, 0, 0x280, 0x100). Sprite IDs: 0x1FA (506) / 0x1FB (507).
 *   RGB tints: 0x70, 0x78, 0x80, 0x90, 0x98.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800CAA7B[];
extern unsigned char D_800CAA7C[];
extern unsigned char D_800CB434[];
extern unsigned char D_800CB436[];
extern unsigned char D_800CB438[];
extern unsigned char D_800E6E20[];
extern unsigned char D_800E6E22[];
extern unsigned char D_800E6E24[];
extern unsigned char D_800E6E28[];
extern unsigned char D_800E6E2C[];
extern unsigned char D_800E6E2D[];

extern unsigned int func_8001E6B8(int n);
extern void func_800ADD6C(unsigned char *prim);
extern void func_800ADCB8(unsigned char *prim, int flag);
extern unsigned int func_800ADAD8(int tp, int abr, int x, int y);
extern void func_800ADB14(int arg0, int texId);

void func_8009ADD4(void)
{
    int i;
    int entryOff;
    int off0;
    int off1;
    int off2;
    int buf;
    int bufOff;

    entryOff = 0;
    off0 = 0;
    off1 = -0x2A;
    off2 = -0x54;

    for (i = 0; i < 28; i++) {
        int stadiumOff = (int)(B(Scratchpad, 0x2E3) & 0xFF) * 84;
        if (i < 7) {
            int tbl = off0 + stadiumOff;
            B(D_800E6E2C, entryOff) = 0;
            H(D_800E6E20, entryOff) = H(D_800CB434, tbl);
            H(D_800E6E22, entryOff) = H(D_800CB436, tbl);
            H(D_800E6E24, entryOff) = H(D_800CB438, tbl);
        } else if (i < 14) {
            int tbl = off1 + stadiumOff;
            B(D_800E6E2C, entryOff) = 0;
            H(D_800E6E20, entryOff) = H(D_800CB434, tbl);
            H(D_800E6E22, entryOff) = H(D_800CB436, tbl);
            H(D_800E6E24, entryOff) = (unsigned short)(-(int)H(D_800CB438, tbl));
        } else if (i < 21) {
            int tbl = off1 + stadiumOff;
            B(D_800E6E2C, entryOff) = (unsigned char)func_8001E6B8(2);
            H(D_800E6E20, entryOff) = H(D_800CB434, tbl);
            H(D_800E6E22, entryOff) = H(D_800CB436, tbl);
            H(D_800E6E24, entryOff) = H(D_800CB438, tbl);
        } else {
            int tbl = off2 + stadiumOff;
            B(D_800E6E2C, entryOff) = (unsigned char)func_8001E6B8(2);
            H(D_800E6E20, entryOff) = (unsigned short)(-(int)H(D_800CB434, tbl));
            H(D_800E6E22, entryOff) = H(D_800CB436, tbl);
            H(D_800E6E24, entryOff) = H(D_800CB438, tbl);
        }

        switch (i % 4) {
        case 0:
            W(D_800E6E28, entryOff) = (unsigned int)D_800CAA7B;
            break;
        case 1:
            W(D_800E6E28, entryOff) = (unsigned int)D_800CAA7C;
            break;
        case 2:
            W(D_800E6E28, entryOff) = (unsigned int)(D_800CAA7C + 1);
            break;
        case 3:
            W(D_800E6E28, entryOff) = (unsigned int)(D_800CAA7C + 2);
            break;
        default:
            break;
        }

        B(D_800E6E2D, entryOff) = (func_8001E6B8(3) != 0) ? 1 : 0;

        entryOff += 0x10;
        off2 += 6;
        off1 += 6;
        off0 += 6;
    }

    bufOff = 0;
    for (buf = 0; buf < 2; buf++) {
        int primOff = 0x3660;
        entryOff = 0;
        for (i = 0; i < 28; i++) {
            unsigned char *prim = (unsigned char *)W(Scratchpad, 0x190) + bufOff + primOff;
            unsigned char flag;
            unsigned char tint;

            func_800ADD6C(prim);
            func_800ADCB8(prim, 0);
            H(prim, 0x16) = (unsigned short)func_800ADAD8(1, 0, 0x280, 0x100);
            flag = B(D_800E6E2C, entryOff);
            H(prim, 0xE) = (unsigned short)flag;
            func_800ADB14(0, (flag != 0) ? 0x1FB : 0x1FA);

            switch (B(Scratchpad, 0x2E3)) {
            case 0:
            case 4:
            case 5:
                tint = (B(Scratchpad, 0x2E5) == 1) ? 0x78 : 0x80;
                B(prim, 4) = tint;
                B(prim, 5) = tint;
                B(prim, 6) = tint;
                break;

            case 1:
                B(prim, 4) = 0x90;
                B(prim, 5) = 0x90;
                B(prim, 6) = 0x90;
                break;

            case 2:
                tint = (B(Scratchpad, 0x2E5) == 1) ? 0x78 : 0x70;
                B(prim, 4) = tint;
                B(prim, 5) = tint;
                B(prim, 6) = tint;
                break;

            case 3:
                tint = (B(Scratchpad, 0x2E5) == 1) ? 0x90 : 0x98;
                B(prim, 4) = tint;
                B(prim, 5) = tint;
                B(prim, 6) = tint;
                break;

            default:
                break;
            }

            entryOff += 0x10;
            primOff += 0x28;
        }
        bufOff += 0x3AC0;
    }
}
