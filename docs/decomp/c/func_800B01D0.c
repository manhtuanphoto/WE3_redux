/*
 * func_800B01D0
 *
 * Purpose:
 *   Writes the 32-bit word `cmd` to the address stored in pointer D_800CEB9C
 *   (likely a PS1 GPU command register such as GP0), and caches the low byte
 *   of `cmd` in shadow table D_800E7090 indexed by the top byte (bits 24..31)
 *   of `cmd`.
 *
 * Arguments:
 *   cmd (unsigned int): 32-bit command/data word
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800CEB9C (volatile unsigned int *): target hardware/register pointer
 *
 * Global fields written:
 *   *D_800CEB9C (u32)
 *   D_800E7090[cmd >> 24] (u8): shadow byte indexed by top 8 bits of `cmd`
 */

extern volatile unsigned int *D_800CEB9C;
extern unsigned char D_800E7090[];

void func_800B01D0(unsigned int cmd)
{
    *D_800CEB9C = cmd;
    D_800E7090[cmd >> 24] = (unsigned char)cmd;
}
