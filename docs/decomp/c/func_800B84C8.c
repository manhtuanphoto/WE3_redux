/*
 * func_800B84C8
 *
 * Purpose:
 *   Writes a 4-byte audio mixing/attenuation matrix (PsyQ CdMix pattern) to
 *   hardware registers mapped through pointer globals D_800D3C14..D_800D3C20:
 *     - Selects bank 2 (*D_800D3C14 = 2), writes params[0] to *D_800D3C1C
 *       and params[1] to *D_800D3C20.
 *     - Selects bank 3 (*D_800D3C14 = 3), writes params[2] to *D_800D3C18,
 *       params[3] to *D_800D3C1C, and 0x20 to *D_800D3C20 to latch/apply.
 *
 * Arguments:
 *   params (const unsigned char *) - 4-byte parameter array [0..3]
 *
 * Return value:
 *   0
 *
 * Global fields read (MMIO register pointers):
 *   D_800D3C14 (volatile u8 *) - bank/index register (e.g. CD REG0 0x1F801800)
 *   D_800D3C18 (volatile u8 *) - register 1 (e.g. CD REG1 0x1F801801)
 *   D_800D3C1C (volatile u8 *) - register 2 (e.g. CD REG2 0x1F801802)
 *   D_800D3C20 (volatile u8 *) - register 3 (e.g. CD REG3 0x1F801803)
 */

extern volatile unsigned char *D_800D3C14;
extern volatile unsigned char *D_800D3C18;
extern volatile unsigned char *D_800D3C1C;
extern volatile unsigned char *D_800D3C20;

int func_800B84C8(const unsigned char *params)
{
    *D_800D3C14 = 0x02U;
    *D_800D3C1C = params[0];
    *D_800D3C20 = params[1];

    *D_800D3C14 = 0x03U;
    *D_800D3C18 = params[2];
    *D_800D3C1C = params[3];
    *D_800D3C20 = 0x20U;

    return 0;
}
