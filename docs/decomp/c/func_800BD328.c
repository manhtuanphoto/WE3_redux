/*
 * func_800BD328
 *
 * Purpose:
 *   Resets all active voice slots `0..D_800FB578-1` (where bit `slot` of
 *   D_800D4F0C is 0) in the 54-byte-stride voice table, initializes a local
 *   SpuVoiceAttr-like structure on the stack (mask 0x00060093, zero volume,
 *   pitch 0x1000, addr 0x1000, ADSR 0x80FF / 0x4000) and commits it via
 *   func_800C44A8, then calls func_800BFD18(1) with D_8010A3E2 = slot.
 *
 * Arguments:
 *   void
 *
 * Return value:
 *   void
 *
 * Global fields read / written:
 *   D_800FB578 (s8): total voice slot count
 *   D_800D4F0C (u32): bitmask of free/inactive slots (bit == 1 skips slot)
 *   D_800E78DA, D_800E78DE, D_800E78E8, D_800E78EA, D_800E78EC, D_800E78EE
 *     (s16[], stride 54 bytes): per-slot voice parameters reset to defaults
 *   D_8010A3E2 (s16): current slot index passed implicitly to func_800BFD18
 */

extern signed char D_800FB578;
extern unsigned int D_800D4F0C;
extern unsigned char D_800E78DA[];
extern unsigned char D_800E78DE[];
extern unsigned char D_800E78E8[];
extern unsigned char D_800E78EA[];
extern unsigned char D_800E78EC[];
extern unsigned char D_800E78EE[];
extern signed short D_8010A3E2;

extern void func_800C44A8(void *voiceAttr);
extern void func_800BFD18(signed int arg0);

void func_800BD328(void)
{
    unsigned char voiceAttr[0x50];
    signed int slot = 0;

    *(unsigned int *)(voiceAttr + 0x04) = 0x00060093U;
    *(signed short *)(voiceAttr + 0x08) = 0;
    *(signed short *)(voiceAttr + 0x0A) = 0;
    *(signed short *)(voiceAttr + 0x14) = 0x1000;
    *(unsigned int *)(voiceAttr + 0x1C) = 0x1000U;
    *(unsigned short *)(voiceAttr + 0x3A) = 0x80FFU;
    *(signed short *)(voiceAttr + 0x3C) = 0x4000;

    while ((signed int)(signed short)slot < (signed int)D_800FB578) {
        signed int idx = (signed int)(signed short)slot;
        unsigned int voiceMask = 1U << idx;

        if ((D_800D4F0C & voiceMask) == 0) {
            unsigned int off = (unsigned int)idx * 54U;
            *(signed short *)(D_800E78DA + off) = 0x18;
            *(signed short *)(D_800E78DE + off) = 0;
            *(signed short *)(D_800E78E8 + off) = 0xFF;
            *(signed short *)(D_800E78EA + off) = 0;
            *(signed short *)(D_800E78EC + off) = 0;
            *(signed short *)(D_800E78EE + off) = 0xFF;

            *(unsigned int *)(voiceAttr + 0x00) = voiceMask;
            func_800C44A8(voiceAttr);

            D_8010A3E2 = (signed short)slot;
            func_800BFD18(1);
        }

        slot++;
    }
}
