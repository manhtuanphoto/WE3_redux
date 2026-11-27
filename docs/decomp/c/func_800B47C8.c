/*
 * func_800B47C8
 *
 * Purpose:
 *   Updates the active DRAWENV clip rectangle in D_800F0F78 from the base
 *   rectangle D_800FF738 plus the current double-buffer VRAM offset
 *   (D_800E7800[buf], D_800E789C[buf]) indexed by D_8010CD54, then submits
 *   D_800F0F78 to func_800AEB98 (PutDrawEnv).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   unsigned char * - returns result of func_800AEB98(D_800F0F78)
 *
 * Global fields read:
 *   D_8010CD54 (s16):    active buffer index (0 or 1)
 *   D_800FF738 (s16[4]): base clip rect (x, y, w, h)
 *   D_800E7800 (s16[]):  per-buffer VRAM X offsets
 *   D_800E789C (s16[]):  per-buffer VRAM Y offsets
 *
 * Global fields written:
 *   D_800F0F78 (DRAWENV): clip x, y, w, h at +0x00..+0x06
 */

#define H(p, o)  (*(unsigned short *)((unsigned char *)(p) + (o)))
#define SH(p, o) (*(signed short *)((unsigned char *)(p) + (o)))

extern signed short D_8010CD54;
extern unsigned char D_800FF738[];
extern signed short D_800E7800[];
extern signed short D_800E789C[];
extern unsigned char D_800F0F78[];

extern unsigned char *func_800AEB98(unsigned char *drawEnv);

unsigned char *func_800B47C8(void)
{
    signed short buf = D_8010CD54;
    signed short off_x = D_800E7800[buf];
    signed short off_y = D_800E789C[buf];
    unsigned short w = H(D_800FF738, 0x04);
    unsigned short h = H(D_800FF738, 0x06);
    signed short x = SH(D_800FF738, 0x00);
    signed short y = SH(D_800FF738, 0x02);

    H(D_800F0F78, 0x06) = h;
    H(D_800F0F78, 0x04) = w;
    SH(D_800F0F78, 0x00) = (signed short)(x + off_x);
    SH(D_800F0F78, 0x02) = (signed short)(y + off_y);

    return func_800AEB98(D_800F0F78);
}
