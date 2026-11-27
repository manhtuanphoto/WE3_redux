/*
 * func_8009BF7C
 *
 * Purpose:
 *   Builds and GTE-projects an 8-segment ribbon/trail primitive strip (e.g.
 *   motion trail / ribbon / afterimage) around a world position (arg0, arg1, arg2),
 *   selecting frame offsets from a cyclic 110-entry pose/frame table D_800CADF0
 *   with repeat counts based on *arg4, and submitting visible segments to the
 *   ordering table D_800F3568.
 *
 * Arguments:
 *   x      (s16)          - world X coordinate
 *   y      (s16)          - world Y coordinate
 *   z      (s16)          - world Z coordinate
 *   mirror (unsigned char)- orientation/tilt sign flag (0 vs non-zero)
 *   framePtr (unsigned char *) - pointer to starting frame/pose index (0..109)
 *   phase  (unsigned char)- starting segment phase offset (mod 8)
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x000 (u32) OT shift base (0xE - W(0) used to shift screen Z) (R)
 *     +0x0EC (s32) GTE SZ3 >> 2 (W/R)
 *     +0x104..+0x120 (matrix/translation) built via matrix helpers & loaded into GTE (R/W)
 *     +0x124..+0x128 (s16[3]) local rotation angles from D_800CADF0[frame] (W)
 *     +0x12C..+0x130 (s16[3]) world xyz position (W)
 *     +0x13C..+0x144 (s32[3]) transformed translation output (R)
 *     +0x14C..+0x154 (s32[3]) scale vector set to 0x1000 (1.0 in 4.12 fixed) (W)
 *     +0x190 (u32) set to D_800E8208 (W)
 *     +0x1A8..+0x1C4 (camera/view matrix) (R/W)
 *     +0x29D (u8)  active display buffer index (0 or 1) (R)
 *   Globals:
 *     D_800CADF0 (u32[]) table of pointers to 3-halfword orientation entries (R)
 *     D_800CB30C (u8[])  vertex table (8 bytes per vertex for GTE lwc2) (R)
 *     D_800CB3DC/DD/DE/DF (u8[]) strip vertex index quads (stride 4) (R)
 *     D_800E8208 (u8[])  packet buffer (buffer stride 0x3AC0, base +0x2A30, poly stride 40) (W)
 *     D_800F3568 (u8[])  ordering table base (buffer stride 0x4000) (R/W via func_800ADC04)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x1000 = 4096 (1.0 in PS1 4.12 fixed-point)
 *   +-0x400 / +-0x300 = +-1024 / +-768 (1/4 and 3/16 rotation units)
 *   0x6D = 109 (max frame index before wraparound)
 *   0x3AC0 = 15040 bytes per display buffer packet block, 40 bytes per quad packet
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned int D_800CADF0[];
extern unsigned char D_800CB30C[];
extern unsigned char D_800CB3DC[];
extern unsigned char D_800CB3DD[];
extern unsigned char D_800CB3DE[];
extern unsigned char D_800CB3DF[];
extern unsigned char D_800E8208[];
extern unsigned char D_800F3568[];

extern void func_800B19A8(unsigned char*, unsigned char*);
extern void func_800B1C38(int, unsigned char*);
extern void func_800B1DD8(int, unsigned char*);
extern void func_800B1348(unsigned char*, unsigned char*);
extern void func_800B11B8(unsigned char*, unsigned char*);
extern void func_800B12C8(unsigned char*, unsigned char*, unsigned char*);
extern void func_800ADC04(unsigned char*, unsigned char*);

static int abs_val(int v) {
    return (v < 0) ? -v : v;
}

void func_8009BF7C(signed short x, signed short y, signed short z,
                   unsigned char mirror, unsigned char* framePtr, unsigned char phase) {
    int repeatType = -1;
    signed short repeatMode;
    int i;
    unsigned char curFrame;
    unsigned short prevX0 = 0, prevY0 = 0, prevX2 = 0, prevY2 = 0;

    curFrame = *framePtr;
    if (abs_val(0x24 - curFrame) < 12) {
        signed short d = (signed short)abs_val(curFrame - 0x24);
        if (d >= 11) {
            repeatType = 0;
        } else if (curFrame == 0x28) {
            repeatType = 6;
        } else if (d >= 10) {
            repeatType = 1;
        } else if (d >= 9) {
            repeatType = 2;
        } else if (d >= 8) {
            repeatType = 3;
        } else if (d >= 7) {
            repeatType = 4;
        } else {
            repeatType = 5;
        }
    } else if (abs_val(0x3F - curFrame) < 11) {
        signed short d = (signed short)abs_val(curFrame - 0x3F);
        if (d >= 9) {
            repeatType = 0;
        } else if (d >= 6) {
            repeatType = 1;
        } else if (d >= 4) {
            repeatType = 2;
        } else {
            repeatType = 3;
        }
    } else if (abs_val(0x5B - curFrame) < 12) {
        signed short d = (signed short)abs_val(curFrame - 0x5B);
        if (d >= 11) {
            repeatType = 0;
        } else if (curFrame == 0x5F) {
            repeatType = 6;
        } else if (d >= 10) {
            repeatType = 1;
        } else if (d >= 9) {
            repeatType = 2;
        } else if (d >= 8) {
            repeatType = 3;
        } else if (d >= 7) {
            repeatType = 4;
        } else {
            repeatType = 5;
        }
    } else if (abs_val(0x08 - curFrame) < 11) {
        signed short d = (signed short)abs_val(curFrame - 0x08);
        if (d >= 9) {
            repeatType = 0;
        } else if (d >= 6) {
            repeatType = 1;
        } else if (d >= 4) {
            repeatType = 2;
        } else {
            repeatType = 3;
        }
    } else if (abs_val(0x6D - curFrame) < 2) {
        repeatType = 0;
    }

    repeatMode = (signed short)repeatType;
    curFrame = *framePtr;

    for (i = 0; i < 8; i++) {
        signed short segIdx = (signed short)(i + phase);
        signed short segMod = segIdx % 8;
        unsigned char* entry;
        unsigned char* pkt;
        int sz3;

        entry = (unsigned char*)D_800CADF0[curFrame & 0xFF];
        if ((curFrame & 0xFF) == 0) {
            curFrame = 0x6D;
        } else {
            curFrame--;
        }

        if ((curFrame & 0xFF) != 0x6D && (unsigned int)repeatMode < 7) {
            switch (repeatMode) {
            case 0:
                if (segMod == 0) {
                    curFrame++;
                }
                break;
            case 1:
                if (segMod == 0 || segMod == 6) {
                    curFrame++;
                }
                break;
            case 2:
                if (segMod == 0 || (segMod >= 5 && segMod < 7)) {
                    curFrame++;
                }
                break;
            case 3:
                if ((segMod >= 0 && segMod < 2) || (segMod >= 5 && segMod < 7)) {
                    curFrame++;
                }
                break;
            case 4:
                if ((segMod >= 0 && segMod < 2) || (segMod >= 4 && segMod < 7)) {
                    curFrame++;
                }
                break;
            case 5:
                if ((segMod >= 0 && segMod < 3) || (segMod >= 4 && segMod < 7)) {
                    curFrame++;
                }
                break;
            case 6:
                if (segMod >= 0 && segMod < 7) {
                    curFrame++;
                }
                break;
            }
        }

        H(Scratchpad, 0x124) = H(entry, 0);
        H(Scratchpad, 0x126) = H(entry, 2);
        H(Scratchpad, 0x128) = H(entry, 4);
        H(Scratchpad, 0x12C) = (unsigned short)x;
        H(Scratchpad, 0x12E) = (unsigned short)y;
        H(Scratchpad, 0x130) = (unsigned short)z;
        W(Scratchpad, 0x14C) = 0x1000;
        W(Scratchpad, 0x150) = 0x1000;
        W(Scratchpad, 0x154) = 0x1000;

        func_800B19A8(Scratchpad + 0x124, Scratchpad + 0x104);
        if (mirror == 0) {
            func_800B1C38(-0x400, Scratchpad + 0x104);
            func_800B1DD8(0x300, Scratchpad + 0x104);
        } else {
            func_800B1C38(0x400, Scratchpad + 0x104);
            func_800B1DD8(-0x300, Scratchpad + 0x104);
        }
        func_800B1348(Scratchpad + 0x104, Scratchpad + 0x14C);
        func_800B11B8(Scratchpad + 0x1A8, Scratchpad + 0x104);
        func_800B12C8(Scratchpad + 0x1A8, Scratchpad + 0x12C, Scratchpad + 0x13C);

        W(Scratchpad, 0x118) = W(Scratchpad, 0x13C) + W(Scratchpad, 0x1BC);
        W(Scratchpad, 0x11C) = W(Scratchpad, 0x140) + W(Scratchpad, 0x1C0);
        W(Scratchpad, 0x120) = W(Scratchpad, 0x144) + W(Scratchpad, 0x1C4);

        /* GTE: ctc2 loads rotation matrix (Scratchpad+0x104..0x114) into $0..$4
         * and translation vector (Scratchpad+0x118..0x120) into $5..$7 */

        W(Scratchpad, 0x190) = (unsigned int)D_800E8208;
        pkt = D_800E8208 + B(Scratchpad, 0x29D) * 0x3AC0 + segIdx * 40 + 0x2A30;

        /* GTE rtps vertex 0: D_800CB30C[D_800CB3DC[segMod * 4] * 8] -> SXY0 ($14) stored at pkt+0x08 */
        {
            unsigned char idx0 = D_800CB3DC[segMod * 4];
            (void)D_800CB30C[idx0 * 8]; /* lwc2 $0/$1 + rtps -> swc2 $14, 0x08(pkt) */
        }
        if (segMod == 0) {
            unsigned char idx1 = D_800CB3DD[0];
            (void)D_800CB30C[idx1 * 8]; /* lwc2 $0/$1 + rtps -> swc2 $14, 0x10(pkt) */
        } else {
            H(pkt, 0x10) = prevX0;
            H(pkt, 0x12) = prevY0;
        }

        /* GTE rtps vertex 2: D_800CB30C[D_800CB3DE[segMod * 4] * 8] -> SXY0 ($14) stored at pkt+0x18 */
        {
            unsigned char idx2 = D_800CB3DE[segMod * 4];
            (void)D_800CB30C[idx2 * 8]; /* lwc2 $0/$1 + rtps -> swc2 $14, 0x18(pkt) */
        }
        if (segMod == 0) {
            unsigned char idx3 = D_800CB3DF[0];
            (void)D_800CB30C[idx3 * 8]; /* lwc2 $0/$1 + rtps -> swc2 $14, 0x20(pkt) */
        } else {
            H(pkt, 0x20) = prevX2;
            H(pkt, 0x22) = prevY2;
        }

        prevX0 = H(pkt, 0x08);
        prevY0 = H(pkt, 0x0A);
        prevX2 = H(pkt, 0x18);
        prevY2 = H(pkt, 0x1A);

        /* GTE mfc2 $19 (SZ3) >> 2 -> Scratchpad+0x0EC */
        sz3 = SW(Scratchpad, 0x0EC);
        if (sz3 > 0) {
            int zShifted = sz3 >> (0x0E - W(Scratchpad, 0x000));
            int otz = (zShifted * 19) / 20;
            func_800ADC04(D_800F3568 + (B(Scratchpad, 0x29D) << 14) + otz * 4, pkt);
        }
    }
}
