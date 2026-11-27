/*
 * func_800AC160
 *
 * Purpose:
 *   Initializes CD stream / movie / data playback state for stream ID `id`
 *   (stored as `id - 0x300` in D_800E6AEE). Clears 16 halfwords backwards from
 *   D_8010CD7E (D_8010CD60..D_8010CD7E), copies metadata from the 12-byte-stride
 *   table at D_800CE9C8, converts the start sector (base + offset) to a CD
 *   location at D_800E6AE0 via func_800B7494, and sets stream active state
 *   D_800E6AB0 = 1, timeout D_800E6B4C = 0x1E (30), and byte/sector size
 *   D_800E6AB4 = len << 5.
 *
 * Arguments:
 *   id (signed int): stream/resource ID (0x300-based)
 *
 * Return value:
 *   void
 *
 * Global fields read / written:
 *   D_800E6AEE (s16): resolved index (id - 0x300)
 *   D_8010CD7E (s16 *): top of 16-halfword buffer zeroed backwards
 *   D_800CE9C8 (12-byte entries):
 *     +0x00 (D_800CE9C8, u32): base LBA sector
 *     +0x04 (D_800CE9CC, u16): length value (shifted << 5 into D_800E6AB4)
 *     +0x07 (D_800CE9CF, u8): copied to D_800E6AE8 (s16)
 *     +0x08 (D_800CE9D0, u8): sector offset added to base, copied to D_800E6BD4 (s16)
 *     +0x0A (D_800CE9D2, u8): copied to D_800E6AD6 (s16)
 *     +0x0B (D_800CE9D2+1, u8): copied to D_800E6ADC (s16)
 *   D_800E6AE0 (u8[]): CdlLOC output buffer for func_800B7494
 *   D_800E6AAC (u32), D_800E6AB0 (u32), D_800E6AB4 (u32),
 *   D_800E6B00 (u8), D_800E6B4C (s16): stream control state
 */

extern signed short D_800E6AEE;
extern signed short D_8010CD7E[];
extern unsigned char D_800CE9C8[];
extern unsigned char D_800CE9CC[];
extern unsigned char D_800CE9CF[];
extern unsigned char D_800CE9D0[];
extern unsigned char D_800CE9D2[];
extern signed short D_800E6AD6;
extern signed short D_800E6ADC;
extern unsigned char D_800E6AE0[];
extern signed short D_800E6AE8;
extern signed short D_800E6BD4;
extern unsigned char D_800E6B00;
extern unsigned int D_800E6AAC;
extern unsigned int D_800E6AB0;
extern unsigned int D_800E6AB4;
extern signed short D_800E6B4C;

extern void func_800B7494(unsigned int lba, unsigned char *outLoc);

void func_800AC160(signed int id)
{
    signed short *clrPtr = D_8010CD7E;
    signed int i;
    signed int idx;
    signed int entryOff;
    unsigned int lba;

    D_800E6AEE = (signed short)(id - 0x300);

    for (i = 0xF; i >= 0; i--, clrPtr--) {
        *clrPtr = 0;
    }

    idx = (signed int)D_800E6AEE;
    entryOff = idx * 12;

    D_800E6AD6 = (signed short)D_800CE9D2[entryOff];
    lba = *(unsigned int *)(D_800CE9C8 + entryOff);
    D_800E6ADC = (signed short)D_800CE9D2[entryOff + 1];
    D_800E6AE8 = (signed short)D_800CE9CF[entryOff];
    D_800E6B00 = 0;
    D_800E6BD4 = (signed short)D_800CE9D0[entryOff];

    func_800B7494(lba + (unsigned int)D_800CE9D0[entryOff], D_800E6AE0);

    idx = (signed int)D_800E6AEE;
    entryOff = idx * 12;
    D_800E6AAC = 0;
    D_800E6AB0 = 1;
    D_800E6B00 = 0;
    D_800E6B4C = 0x1E;
    D_800E6AB4 = (unsigned int)(*(unsigned short *)(D_800CE9CC + entryOff)) << 5;
}
