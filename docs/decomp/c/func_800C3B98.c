/*
 * func_800C3B98
 *
 * Purpose:
 *   Sets SPU voice Key On (keyOnOff == 1) or Key Off (keyOnOff == 0) for up to
 *   24 voices (analogous to PsyQ SpuSetKey), either queuing into the shadow
 *   buffer D_800F0EA8 when (D_800D5578 & 1) is set or writing directly to SPU
 *   hardware KON/KOFF registers via D_800D558C (+0x188 / +0x18C).
 *
 * Arguments:
 *   keyOnOff:  (int) 0 = Key Off, 1 = Key On
 *   voiceMask: (unsigned int) 24-bit voice bitmask (masked with 0xFFFFFF)
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   D_800D5118: (u32) active voice bitmask (direct mode) (R/W)
 *   D_800D5140: (u32) active voice bitmask (queued mode) (R/W)
 *   D_800D5144: (u32) pending SPU update flags, bit 0 set in queued mode (R/W)
 *   D_800D5578: (u32) SPU driver mode flags (bit 0 = queued/deferred) (R)
 *   D_800D558C: (u8*) SPU hardware register base (KON at +0x188, KOFF at +0x18C) (R)
 *   D_800F0EA8: (u16[4]) queued KON (0, 2) and KOFF (4, 6) shadow registers (R/W)
 */

#define H(p,o) (*(unsigned short*)((p)+(o)))

extern unsigned int D_800D5118;
extern unsigned int D_800D5140;
extern unsigned int D_800D5144;
extern unsigned int D_800D5578;
extern unsigned char *D_800D558C;
extern unsigned char D_800F0EA8[];

void func_800C3B98(int keyOnOff, unsigned int voiceMask) {
    unsigned int lowMask = voiceMask & 0x00FFFFFFU;
    unsigned int highMask = lowMask >> 16;

    if (keyOnOff == 0) {
        if ((D_800D5578 & 1U) != 0) {
            H(D_800F0EA8, 4) = (unsigned short)lowMask;
            H(D_800F0EA8, 6) = (unsigned short)highMask;
            D_800D5144 |= 1U;
            D_800D5140 &= ~lowMask;
            if ((H(D_800F0EA8, 0) & lowMask) != 0) {
                H(D_800F0EA8, 0) &= (unsigned short)~lowMask;
            }
            if ((H(D_800F0EA8, 2) & highMask) != 0) {
                H(D_800F0EA8, 2) &= (unsigned short)~highMask;
            }
        } else {
            H(D_800D558C, 0x18C) = (unsigned short)lowMask;
            H(D_800D558C, 0x18E) = (unsigned short)highMask;
            D_800D5118 &= ~lowMask;
        }
    } else if (keyOnOff == 1) {
        if ((D_800D5578 & 1U) != 0) {
            H(D_800F0EA8, 0) = (unsigned short)lowMask;
            H(D_800F0EA8, 2) = (unsigned short)highMask;
            D_800D5144 |= 1U;
            D_800D5140 |= lowMask;
            if ((H(D_800F0EA8, 4) & lowMask) != 0) {
                H(D_800F0EA8, 4) &= (unsigned short)~lowMask;
            }
            if ((H(D_800F0EA8, 6) & highMask) != 0) {
                H(D_800F0EA8, 6) &= (unsigned short)~highMask;
            }
        } else {
            H(D_800D558C, 0x188) = (unsigned short)lowMask;
            H(D_800D558C, 0x18A) = (unsigned short)highMask;
            D_800D5118 |= lowMask;
        }
    }
}
