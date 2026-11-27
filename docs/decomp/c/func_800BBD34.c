/*
 * func_800BBD34
 *
 * Purpose:
 *   PsyQ SEQ/MIDI audio track loop/advance handler. Indexes the 176-byte
 *   (0xB0) track struct at D_8010C2A8[a0] + a1 * 0xB0, increments loop counter
 *   +0x21 against limit +0x20, resets or loops sequence pointers (+0x00/+0x04/
 *   +0x08/+0x0C) based on flags +0x98 (bit 0x400), and on final completion
 *   updates flags +0x98, triggers callback/note cleanup (func_800BC3D8,
 *   func_800C0930), and sets +0x90 = (s16)+0x54.
 *
 * Arguments:
 *   a0 (s16): sequence/slot index into D_8010C2A8
 *   a1 (s16): track index (stride 0xB0 = 176 bytes)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_8010C2A8 (u8*[]) array of sequence track struct tables (0xB0-byte stride)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char *D_8010C2A8[];

extern void func_800BC3D8(signed int a0, signed int a1);
extern void func_800C0930(signed short id);

void func_800BBD34(signed short a0, signed short a1)
{
    unsigned int offset = (unsigned int)((signed int)a1 * 0xB0);
    unsigned char *base = D_8010C2A8[a0];
    unsigned char *track = base + offset;
    unsigned char cnt = (unsigned char)(B(track, 0x21) + 1U);
    signed char limit = (signed char)B(track, 0x20);
    unsigned int val;

    B(track, 0x21) = cnt;

    if (limit == 0) {
        W(track, 0x88) = 0;
        B(track, 0x1C) = 0;
        W(track, 0x90) = 0;
        val = ((W(D_8010C2A8[a0] + offset, 0x98) & 0x400U) != 0)
            ? W(track, 0xC)
            : W(track, 4);
        W(track, 0) = val;
        return;
    }

    if ((signed char)cnt < limit) {
        W(track, 0x88) = 0;
        B(track, 0x1C) = 0;
        W(track, 0x90) = 0;
        val = ((W(D_8010C2A8[a0] + offset, 0x98) & 0x400U) != 0)
            ? W(track, 0xC)
            : W(track, 4);
        W(track, 0) = val;
        W(track, 8) = val;
        return;
    }

    {
        unsigned char *t2 = D_8010C2A8[a0] + offset;
        W(t2, 0x98) &= ~1U;
        t2 = D_8010C2A8[a0] + offset;
        W(t2, 0x98) &= ~8U;
        t2 = D_8010C2A8[a0] + offset;
        W(t2, 0x98) &= ~2U;
        t2 = D_8010C2A8[a0] + offset;
        W(t2, 0x98) |= 0x200U;
        t2 = D_8010C2A8[a0] + offset;
        W(t2, 0x98) |= 4U;
    }

    B(track, 0x14) = 0;
    W(track, 8) = ((W(D_8010C2A8[a0] + offset, 0x98) & 0x400U) != 0)
        ? W(track, 0xC)
        : W(track, 4);

    if ((signed char)B(track, 0x22) != -1) {
        func_800BC3D8((signed char)B(track, 0x22), (signed char)B(track, 0x23));
        B(track, 0x14) = 0;
        func_800C0930((signed short)((a1 << 8) | (a0 & 0xFFFF)));
    }

    func_800C0930((signed short)((a1 << 8) | (a0 & 0xFFFF)));
    SW(track, 0x90) = S(track, 0x54);
}
