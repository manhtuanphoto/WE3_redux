/*
 * func_800B4550
 *
 * Purpose:
 *   Fills a 16-byte GPU primitive (TILE-like struct at D_800E7198[D_8010CD54])
 *   with RGB color (r, g, b), screen coordinates from D_800E7800[idx] and
 *   D_800E789C[idx], width from D_800FB57C (scaled 1.5x if D_800F0FE9 != 0),
 *   and height from D_800FB5AC, then inserts it into the ordering table at
 *   *(otCtx + 0x10) via func_800ADC04.
 *
 * Arguments:
 *   r     (unsigned char): red color component
 *   g     (unsigned char): green color component
 *   b     (unsigned char): blue color component
 *   otCtx (unsigned char *): graphics context whose +0x10 word is the OT head
 *
 * Return value:
 *   void
 *
 * Global fields read / written:
 *   D_8010CD54 (s16): current primitive slot index
 *   D_800E7198 (u8[], stride 16): TILE primitive pool
 *     +0x04/+0x05/+0x06 (D_800E719C/D/E): r, g, b
 *     +0x08 (D_800E71A0): x (u16)
 *     +0x0A (D_800E71A2): y (u16)
 *     +0x0C (D_800E71A4): width (s16)
 *     +0x0E (D_800E71A6): height (u16)
 *   D_800E7800 (u16[]): X coordinate table
 *   D_800E789C (u16[]): Y coordinate table
 *   D_800FB57C (s32/u16): base width
 *   D_800FB5AC (u16): height
 *   D_800F0FE9 (u8): width 1.5x scale flag (e.g. wide/PAL/hi-res mode)
 */

extern signed short D_8010CD54;
extern unsigned char D_800E7198[];
extern unsigned char D_800E719C[];
extern unsigned char D_800E719D[];
extern unsigned char D_800E719E[];
extern unsigned char D_800E71A0[];
extern unsigned char D_800E71A2[];
extern unsigned char D_800E71A4[];
extern unsigned char D_800E71A6[];
extern unsigned short D_800E7800[];
extern unsigned short D_800E789C[];
extern signed int D_800FB57C;
extern unsigned short D_800FB5AC;
extern unsigned char D_800F0FE9;

extern void func_800ADC04(unsigned int otEntry, void *prim);

void func_800B4550(unsigned char r, unsigned char g, unsigned char b, unsigned char *otCtx)
{
    signed int idx;
    signed int off;

    idx = (signed int)D_8010CD54;
    D_800E719C[idx << 4] = r;

    idx = (signed int)D_8010CD54;
    D_800E719D[idx << 4] = g;

    idx = (signed int)D_8010CD54;
    D_800E719E[idx << 4] = b;

    idx = (signed int)D_8010CD54;
    off = idx << 4;
    *(unsigned short *)(D_800E71A0 + off) = D_800E7800[idx];
    *(unsigned short *)(D_800E71A6 + off) = D_800FB5AC;
    *(unsigned short *)(D_800E71A2 + off) = D_800E789C[idx];

    if (D_800F0FE9 != 0) {
        signed int w = D_800FB57C;
        *(signed short *)(D_800E71A4 + off) = (signed short)((w * 3) / 2);
    } else {
        *(unsigned short *)(D_800E71A4 + off) = *(unsigned short *)&D_800FB57C;
    }

    func_800ADC04(*(unsigned int *)(otCtx + 0x10), D_800E7198 + ((signed int)D_8010CD54 << 4));
}
