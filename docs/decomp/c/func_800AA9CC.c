/*
 * func_800AA9CC
 *
 * Purpose:
 *   Initializes CD-ROM / XA audio playback globals: stores `arg0 & 0xFFFF` into
 *   D_800E6BC8, sets D_800E6B58 to 0x90 (144), and clears D_800E6B38 to 0.
 *
 * Arguments:
 *   arg0 (unsigned int): 16-bit value masked with 0xFFFF and stored in D_800E6BC8
 *
 * Return value:
 *   None (void)
 *
 * Global fields written:
 *   D_800E6BC8 (unsigned int): initialized to (arg0 & 0xFFFF)
 *   D_800E6B58 (signed short): initialized to 0x90
 *   D_800E6B38 (unsigned int): cleared to 0
 */

extern unsigned int D_800E6BC8;
extern signed short D_800E6B58;
extern unsigned int D_800E6B38;

void func_800AA9CC(unsigned int arg0)
{
    /* UNCLEAR: exact semantic roles of D_800E6BC8, D_800E6B58, and D_800E6B38 */
    D_800E6BC8 = arg0 & 0xFFFFU;
    D_800E6B58 = 0x90;
    D_800E6B38 = 0U;
}
