/*
 * func_800BD040
 *
 * Purpose:
 *   Updates left/right volume for a sequence track slot in D_8010C2A8.
 *   Looks up entry = D_8010C2A8[(s16)bankIdx] + (s16)trackIdx * 0xB0.
 *   If flags at entry+0x98 == 1, immediately applies the volume via
 *   func_800C02B8((s16)(bankIdx | (trackIdx << 8)), volL, volR, 1); otherwise
 *   stores `volL` and `volR` into entry+0x58 and entry+0x5A.
 *
 *   Note on asm_compact contents:
 *   Lines 39-50 of func_800BD040.s contain an unlabeled function immediately
 *   following func_800BD040's `jr $ra` that packs `(s16)(a0 | (a1 << 8))` and
 *   calls func_800C083C(packedId, a2, a3). That trailing block is preserved in
 *   a clearly marked comment below.
 *
 * Arguments:
 *   bankIdx  (s16 in int) - index into D_8010C2A8 track table array
 *   trackIdx (s16 in int) - track index within the table (0xB0-byte stride)
 *   volL     (u16 in int) - left volume
 *   volR     (u16 in int) - right volume
 *
 * Return value:
 *   None (void)
 *
 * Globals read/written:
 *   D_8010C2A8 (u8*[], R) - array of sequence track tables (stride 0xB0 per track)
 *     +0x58 (s16, W) stored left volume when flags != 1
 *     +0x5A (s16, W) stored right volume when flags != 1
 *     +0x98 (u32, R) track status flags
 */

#define S(p, o) (*(signed short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned char *D_8010C2A8[];

extern void func_800C02B8(int voiceId, unsigned int volL, unsigned int volR, int mode);

void func_800BD040(int bankIdx, int trackIdx, int volL, int volR)
{
    unsigned char *entry = D_8010C2A8[(signed short)bankIdx] + ((signed short)trackIdx * 0xB0);

    if (W(entry, 0x98) == 1U) {
        signed short packedId = (signed short)(bankIdx | (trackIdx << 8));
        func_800C02B8((int)packedId, (unsigned int)volL & 0xFFFFU, (unsigned int)volR & 0xFFFFU, 1);
    } else {
        S(entry, 0x58) = (signed short)volL;
        S(entry, 0x5A) = (signed short)volR;
    }
}

/*
 * UNCLEAR: Unlabeled function body in func_800BD040.s after `jr $ra` (lines 39-50):
 *
 * extern void func_800C083C(int voiceId, unsigned short *outVolL, unsigned short *outVolR);
 *
 * static void sub_trailing_in_func_800BD040(int bankIdx, int trackIdx,
 *                                           unsigned short *outVolL, unsigned short *outVolR)
 * {
 *     signed short packedId = (signed short)(bankIdx | (trackIdx << 8));
 *     func_800C083C((int)packedId, outVolL, outVolR);
 * }
 */
