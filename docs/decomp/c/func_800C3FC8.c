/*
 * func_800C3FC8
 *
 * Purpose:
 *   PsyQ SPU common attribute setter (SpuSetCommonAttr equivalent) plus an
 *   adjacent unlabeled 24-voice status reader (SpuGetAllKeysStatus-like)
 *   contained in the same assembly file.
 *   func_800C3FC8 updates SPU hardware registers via D_800D558C according to
 *   the bitmask at attr+0x00 (or updates all fields if mask == 0): master
 *   volume L/R and sweep modes (+0x180/+0x182), CD/external volumes
 *   (+0x1B0..+0x1B6), and CD/external mix & reverb control bits (+0x1AA).
 *
 * Arguments:
 *   func_800C3FC8(attr):
 *     attr - pointer to SpuCommonAttr-like struct:
 *       +0x00 (u32) mask (0 = apply all)
 *       +0x04 (s16) mvol.left, +0x06 (s16) mvol.right
 *       +0x08 (s16) mvolmode.left (0..7), +0x0A (s16) mvolmode.right (0..7)
 *       +0x10 (u16) cd.volume.left, +0x12 (u16) cd.volume.right
 *       +0x14 (u32) cd.mix, +0x18 (u32) cd.reverb
 *       +0x1C (u16) ext.volume.left, +0x1E (u16) ext.volume.right
 *       +0x20 (u32) ext.mix, +0x24 (u32) ext.reverb
 *
 * Return value:
 *   func_800C3FC8: None (void)
 *   (Adjacent unlabeled voice-status routine returns 0 on success or -3 on invalid range)
 *
 * Global fields read / written:
 *   D_800D558C (u8 *) pointer to SPU MMIO register base (0x1F801C00)
 *   D_800D5118 (u32) 24-bit voice key-on mask
 *
 * Scratchpad (0x1F800000) fields read / written:
 *   None
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   SPU volume mode bits 0x8000..0xE000, 7-bit sweep clamp [0, 0x7F]
 *   24 SPU voices (indices 0..23)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern volatile unsigned char *D_800D558C;
extern unsigned int D_800D5118;

void func_800C3FC8(unsigned char *attr)
{
    unsigned int mask = W(attr, 0);
    int all = (mask == 0);
    unsigned short modeBits;
    unsigned short volL = 0;
    unsigned short volR = 0;
    unsigned short outVal;

    if (all || (mask & 0x1)) {
        if (all || (mask & 0x4)) {
            switch ((unsigned int)S(attr, 8)) {
            case 1: modeBits = 0x8000; break;
            case 2: modeBits = 0x9000; break;
            case 3: modeBits = 0xA000; break;
            case 4: modeBits = 0xB000; break;
            case 5: modeBits = 0xC000; break;
            case 6: modeBits = 0xD000; break;
            case 7: modeBits = 0xE000; break;
            case 0:
            default:
                volL = H(attr, 4);
                modeBits = 0;
                break;
            }
        } else {
            volL = H(attr, 4);
            modeBits = 0;
        }

        if (modeBits == 0) {
            outVal = volL & 0x7FFF;
        } else {
            signed short s = S(attr, 4);
            unsigned short u = H(attr, 4);
            if (s >= 0x80) {
                volL = 0x7F;
            } else if (s < 0) {
                volL = 0;
            } else {
                volL = u;
            }
            outVal = volL & 0x7FFF;
        }
        H(D_800D558C, 0x180) = outVal | modeBits;
    }

    if (all || (mask & 0x2)) {
        if (all || (mask & 0x8)) {
            switch ((unsigned int)S(attr, 0xA)) {
            case 1: modeBits = 0x8000; break;
            case 2: modeBits = 0x9000; break;
            case 3: modeBits = 0xA000; break;
            case 4: modeBits = 0xB000; break;
            case 5: modeBits = 0xC000; break;
            case 6: modeBits = 0xD000; break;
            case 7: modeBits = 0xE000; break;
            case 0:
            default:
                volR = H(attr, 6);
                modeBits = 0;
                break;
            }
        } else {
            volR = H(attr, 6);
            modeBits = 0;
        }

        if (modeBits == 0) {
            outVal = volR & 0x7FFF;
        } else {
            signed short s = S(attr, 6);
            unsigned short u = H(attr, 6);
            if (s >= 0x80) {
                volR = 0x7F;
            } else if (s < 0) {
                volR = 0;
            } else {
                volR = u;
            }
            outVal = volR & 0x7FFF;
        }
        H(D_800D558C, 0x182) = outVal | modeBits;
    }

    if (all || (mask & 0x40)) {
        H(D_800D558C, 0x1B0) = H(attr, 0x10);
    }
    if (all || (mask & 0x80)) {
        H(D_800D558C, 0x1B2) = H(attr, 0x12);
    }
    if (all || (mask & 0x400)) {
        H(D_800D558C, 0x1B4) = H(attr, 0x1C);
    }
    if (all || (mask & 0x800)) {
        H(D_800D558C, 0x1B6) = H(attr, 0x1E);
    }

    if (all || (mask & 0x100)) {
        if (W(attr, 0x14) == 0) {
            H(D_800D558C, 0x1AA) &= 0xFFFB;
        } else {
            H(D_800D558C, 0x1AA) |= 0x4;
        }
    }
    if (all || (mask & 0x200)) {
        if (W(attr, 0x18) == 0) {
            H(D_800D558C, 0x1AA) &= 0xFFFE;
        } else {
            H(D_800D558C, 0x1AA) |= 0x1;
        }
    }
    if (all || (mask & 0x1000)) {
        if (W(attr, 0x20) == 0) {
            H(D_800D558C, 0x1AA) &= 0xFFF7;
        } else {
            H(D_800D558C, 0x1AA) |= 0x8;
        }
    }
    if (all || (mask & 0x2000)) {
        if (W(attr, 0x24) == 0) {
            H(D_800D558C, 0x1AA) &= 0xFFFD;
        } else {
            H(D_800D558C, 0x1AA) |= 0x2;
        }
    }
}

/* Adjacent unlabeled function in func_800C3FC8.s (24-voice key/envelope status query) */
static int func_800C3FC8_getKeysStatus(int start, int end, unsigned char *status)
{
    int i;

    if (start < 0) {
        start = 0;
    }
    if (start >= 0x18) {
        return -3;
    }
    if (end >= 0x18) {
        end = 0x17;
    }
    if (end < 0 || end < start) {
        return -3;
    }

    end += 1;
    status += start;
    for (i = start; i < end; i++, status++) {
        volatile unsigned char *voiceReg = D_800D558C + (i << 4);
        unsigned int keyOn = D_800D5118 & (1u << i);
        unsigned short envx = H(voiceReg, 0xC);

        if (keyOn != 0) {
            *status = (envx != 0) ? 1 : 3;
        } else {
            *status = (envx != 0) ? 2 : 0;
        }
    }
    return 0;
}
