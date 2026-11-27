/*
 * func_800BB7C8
 *
 * Purpose:
 *   Updates a single sound/stream envelope or volume fade slot (bank a0, slot a1)
 *   in D_8010C2A8: decrements its frame timer (+0xA0), steps its level (+0x4A)
 *   according to step rate (+0x4C), queries/sets the hardware/driver level via
 *   func_800C083C / func_800C02B8, clears flag bit 0x20 in +0x98 on completion,
 *   and refreshes current levels into +0x5C / +0x5E.
 *
 * Arguments:
 *   a0 (s32) - bank/table index (sign-extended to s16)
 *   a1 (s32) - slot index within bank (sign-extended to s16, stride 0xB0 = 176 bytes)
 *
 * Return value:
 *   None (void; ends with tail-style call to func_800C083C).
 *
 * Global struct fields read/written (entry = D_8010C2A8[(s16)a0] + (s16)a1 * 0xB0):
 *   +0x48 (s16) fade threshold compared against elapsed * (-step)
 *   +0x4A (u16/s16) remaining fade counter (read/written)
 *   +0x4C (s16/u16) step rate (>0: decrement +0x4A by 1 every step frames; <0: add step per frame)
 *   +0x5C (u16) output level L written by func_800C083C
 *   +0x5E (u16) output level R written by func_800C083C
 *   +0x98 (u32) flags word (clears bit 0x20 when fade finishes)
 *   +0x9C (s32) initial frame count
 *   +0xA0 (s32) remaining frame countdown (decremented each call)
 *
 * Scratchpad fields read/written:
 *   None.
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   Audio/envelope driver only; no spatial constants.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char *D_8010C2A8[];
extern void func_800C083C(int id, unsigned short *out_l, unsigned short *out_r);
extern void func_800C02B8(int id, unsigned short l, unsigned short r, int flag);

void func_800BB7C8(int a0, int a1)
{
    unsigned short cur_l;
    unsigned short cur_r;
    unsigned char **bank_ptr = &D_8010C2A8[(short)a0];
    int slot_off = (int)((short)a1) * 0xB0;
    unsigned char *entry = *bank_ptr + slot_off;
    int timer = SW(entry, 0xA0) - 1;
    short step;
    unsigned short ustep;
    int packed_id;

    SW(entry, 0xA0) = timer;
    if (timer < 0) {
        W(D_8010C2A8[(short)a0] + slot_off, 0x98) &= ~0x20U;
        packed_id = (short)(a0 | (a1 << 8));
        func_800C083C(packed_id, (unsigned short *)(entry + 0x5C), (unsigned short *)(entry + 0x5E));
        return;
    }

    step = S(entry, 0x4C);
    ustep = H(entry, 0x4C);

    if (step > 0) {
        if ((timer % step) == 0) {
            unsigned short rem = H(entry, 0x4A) - 1;
            H(entry, 0x4A) = rem;
            if ((short)rem < 0) {
                packed_id = (short)(a0 | (a1 << 8));
                func_800C02B8(packed_id, 0, 0, 1);
                W(D_8010C2A8[(short)a0] + slot_off, 0x98) &= ~0x20U;
            } else {
                packed_id = (short)(a0 | (a1 << 8));
                func_800C083C(packed_id, &cur_l, &cur_r);
                {
                    short rem_s = S(entry, 0x4A);
                    if ((int)(cur_l - 1) >= (int)(cur_l - rem_s) ||
                        (int)(cur_r - 1) < (int)(cur_r - rem_s)) {
                        if (cur_l == 0 || cur_r == 0) {
                            W(D_8010C2A8[(short)a0] + slot_off, 0x98) &= ~0x20U;
                        } else {
                            func_800C02B8(packed_id, (unsigned short)(cur_l - 1), (unsigned short)(cur_r - 1), 1);
                        }
                    }
                }
            }
        }
    } else if (step < 0) {
        unsigned short rem = H(entry, 0x4A) + ustep;
        H(entry, 0x4A) = rem;
        if ((short)rem < 0) {
            packed_id = (short)(a0 | (a1 << 8));
            func_800C02B8(packed_id, 0, 0, 1);
            W(D_8010C2A8[(short)a0] + slot_off, 0x98) &= ~0x20U;
        } else {
            packed_id = (short)(a0 | (a1 << 8));
            func_800C083C(packed_id, &cur_l, &cur_r);
            if ((int)cur_l + S(entry, 0x4C) <= 0 &&
                (int)cur_r + S(entry, 0x4C) <= 0) {
                func_800C02B8(packed_id, 0, 0, 1);
                W(D_8010C2A8[(short)a0] + slot_off, 0x98) &= ~0x20U;
            }
            {
                int elapsed = SW(entry, 0x9C) - SW(entry, 0xA0);
                int scaled = elapsed * (-S(entry, 0x4C));
                if (scaled < S(entry, 0x48)) {
                    if (cur_l == 0 || cur_r == 0) {
                        W(D_8010C2A8[(short)a0] + slot_off, 0x98) &= ~0x20U;
                    } else {
                        func_800C02B8(
                            packed_id,
                            (unsigned short)(cur_l + ustep),
                            (unsigned short)(cur_r + ustep),
                            1
                        );
                    }
                }
            }
        }
    }

    if (SW(entry, 0xA0) == 0 || S(entry, 0x4A) <= 0) {
        W(D_8010C2A8[(short)a0] + slot_off, 0x98) &= ~0x20U;
    }

    packed_id = (short)(a0 | (a1 << 8));
    func_800C083C(packed_id, (unsigned short *)(entry + 0x5C), (unsigned short *)(entry + 0x5E));
}
