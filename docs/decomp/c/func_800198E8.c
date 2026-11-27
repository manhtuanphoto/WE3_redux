/*
 * func_800198E8
 *
 * Purpose:
 *   Dispatches commentary / audio playback for an 8-bit clip index using
 *   jump table jtbl_800101C0 (for indices 0..9) and clip ID lookup table
 *   D_800C6A40. Initializes an unused 11-word local table copy on the stack
 *   from D_80014D30..D_80014D58 before dispatching.
 *
 * Arguments:
 *   idx (unsigned char): clip selector index (masked to 0xFF)
 *
 * Return value:
 *   func_80017684(D_800C6A40[idx]) & 0xFF on the .L800199F0 path;
 *   1 (after calling func_80017564(D_800C6A40[idx])) on the .L80019A14 path.
 *
 * Global fields read:
 *   D_80014D30..D_80014D58 (11 x u32): copied into local stack arrays (unused)
 *   D_800C6A40 (u16[]): clip ID table indexed by (idx & 0xFF)
 *   jtbl_800101C0 (void *[]): jump table for idx < 10
 */

extern unsigned int D_80014D30;
extern unsigned int D_80014D34;
extern unsigned int D_80014D38;
extern unsigned int D_80014D3C;
extern unsigned int D_80014D40;
extern unsigned int D_80014D44;
extern unsigned int D_80014D48;
extern unsigned int D_80014D4C;
extern unsigned int D_80014D50;
extern unsigned int D_80014D54;
extern unsigned int D_80014D58;

extern unsigned short D_800C6A40[];

extern unsigned int func_80017684(unsigned short clip_id);
extern void func_80017564(unsigned short clip_id);

unsigned int func_800198E8(unsigned int idx)
{
    unsigned int tmpSrc[11];
    unsigned int tmpDst[11];
    unsigned int i;
    unsigned char index = (unsigned char)(idx & 0xFF);

    tmpSrc[0]  = D_80014D30;
    tmpSrc[1]  = D_80014D44;
    tmpSrc[2]  = D_80014D54;
    tmpSrc[3]  = D_80014D3C;
    tmpSrc[4]  = D_80014D40;
    tmpSrc[5]  = D_80014D34;
    tmpSrc[6]  = D_80014D48;
    tmpSrc[7]  = D_80014D38;
    tmpSrc[8]  = D_80014D4C;
    tmpSrc[9]  = D_80014D50;
    tmpSrc[10] = D_80014D58;

    for (i = 0; i < 11; i++) {
        tmpDst[i] = tmpSrc[i];
    }
    (void)tmpDst;

    if (index < 10) {
        switch (index) {
        case 0:
        case 1:
        case 3:
        case 4:
        case 5:
        case 7:
        case 8:
        case 9:
            /* .L800199F0 */
            return func_80017684(D_800C6A40[index]) & 0xFF;
        case 2:
        case 6:
            break;
        }
    }

    /* .L80019A14 */
    func_80017564(D_800C6A40[index]);
    return 1;
}
