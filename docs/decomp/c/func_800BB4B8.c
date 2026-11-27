/*
 * func_800BB4B8
 *
 * Purpose:
 *   Updates one voice/channel fade or volume-ramp state in the 176-byte (0xB0)
 *   sound channel table pointed to by D_8010C2A8[arg0] + arg1 * 0xB0, stepping
 *   volume toward the target via func_800C083C / func_800C02B8 and clearing flag
 *   bit 0x10 at +0x98 when the fade completes.
 *
 * Arguments:
 *   arg0: (signed short / int) table/bank index into D_8010C2A8
 *   arg1: (signed short / int) channel/slot index within that table (stride 0xB0)
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   D_8010C2A8[arg0]: array of pointers to 0xB0-byte sound channel structs:
 *     +0x48 (s16): fade threshold / target product (R)
 *     +0x4A (s16/u16): remaining fade step counter (R/W)
 *     +0x4C (s16/u16): step interval (>0) or negative volume increment (<0) (R)
 *     +0x5C (u16), +0x5E (u16): current left/right volume readback (W via func_800C083C)
 *     +0x98 (u32): status flags; bit 0x10 cleared on completion (R/W)
 *     +0x9C (s32): initial fade duration / tick count (R)
 *     +0xA0 (s32): remaining fade tick counter, decremented by 1 each call (R/W)
 *
 * Magic numbers:
 *   0xB0 (176): bytes per channel entry.
 *   0x7F (127): maximum PS1 SPU voice volume.
 *   ~0x10 (0xFFFFFFEF): active fade flag bit at +0x98.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char *D_8010C2A8[];
extern void func_800C083C(int voiceId, void *outVolL, void *outVolR);
extern void func_800C02B8(int voiceId, unsigned int volL, unsigned int volR, int mode);

void func_800BB4B8(int arg0, int arg1) {
    unsigned char **tableEntry;
    int offset;
    unsigned char *entry;
    signed int counter;
    signed short stepParam;
    unsigned short stepParamU;
    signed short voiceId;
    unsigned short volL;
    unsigned short volR;
    signed short remSteps;

    tableEntry = &D_8010C2A8[(signed short)arg0];
    offset = (signed short)arg1 * 0xB0;
    entry = *tableEntry + offset;

    counter = SW(entry, 0xA0) - 1;
    SW(entry, 0xA0) = counter;
    if (counter < 0) {
        unsigned char *reloaded = *tableEntry + offset;
        W(reloaded, 0x98) &= ~0x10u;
        func_800C083C((signed short)(arg0 | (arg1 << 8)), entry + 0x5C, entry + 0x5E);
        return;
    }

    stepParam = S(entry, 0x4C);
    stepParamU = H(entry, 0x4C);

    if (stepParam > 0) {
        if (counter % stepParam != 0) {
            func_800C083C((signed short)(arg0 | (arg1 << 8)), entry + 0x5C, entry + 0x5E);
            return;
        }
        H(entry, 0x4A) = (unsigned short)(H(entry, 0x4A) - 1);
        if (S(entry, 0x4A) < 0) {
            voiceId = (signed short)(arg0 | (arg1 << 8));
            func_800C02B8(voiceId, 0x7F, 0x7F, 1);
            W(*tableEntry + offset, 0x98) &= ~0x10u;
        } else {
            voiceId = (signed short)(arg0 | (arg1 << 8));
            func_800C083C(voiceId, &volL, &volR);
            remSteps = S(entry, 0x4A);
            if ((signed int)(volL + remSteps) >= (signed int)(volL + 1)) {
                func_800C02B8(voiceId, (volL + 1) & 0xFFFFu, (volR + 1) & 0xFFFFu, 1);
            }
        }
    } else if (stepParam < 0) {
        H(entry, 0x4A) = (unsigned short)(H(entry, 0x4A) + stepParamU);
        if (S(entry, 0x4A) < 0) {
            voiceId = (signed short)(arg0 | (arg1 << 8));
            func_800C02B8(voiceId, 0x7F, 0x7F, 1);
            W(*tableEntry + offset, 0x98) &= ~0x10u;
        } else {
            signed short negStep;
            signed int elapsed;
            voiceId = (signed short)(arg0 | (arg1 << 8));
            func_800C083C(voiceId, &volL, &volR);
            negStep = S(entry, 0x4C);
            if ((signed int)(volL - negStep) >= 0x7F && (signed int)(volR - negStep) >= 0x7F) {
                func_800C02B8(voiceId, 0x7F, 0x7F, 1);
                W(*tableEntry + offset, 0x98) &= ~0x10u;
            }
            elapsed = SW(entry, 0x9C) - SW(entry, 0xA0);
            if (elapsed * (-S(entry, 0x4C)) < S(entry, 0x48)) {
                unsigned short stepU = H(entry, 0x4C);
                func_800C02B8(
                    (signed short)(arg0 | (arg1 << 8)),
                    (volL - stepU) & 0xFFFFu,
                    (volR - stepU) & 0xFFFFu,
                    1
                );
            }
        }
    } else {
        func_800C083C((signed short)(arg0 | (arg1 << 8)), entry + 0x5C, entry + 0x5E);
        return;
    }

    if (SW(entry, 0xA0) == 0 || S(entry, 0x4A) <= 0) {
        unsigned char *reloaded = D_8010C2A8[(signed short)arg0] + (signed short)arg1 * 0xB0;
        W(reloaded, 0x98) &= ~0x10u;
    }

    func_800C083C((signed short)(arg0 | (arg1 << 8)), entry + 0x5C, entry + 0x5E);
}
