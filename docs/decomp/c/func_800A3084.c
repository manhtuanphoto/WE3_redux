/*
 * func_800A3084 - AI team-shape / off-ball target (X, Z) refinement
 *
 * Purpose:
 *   In open play (Scratchpad+0x29A == 1), adjusts an AI player's target (outX, outZ)
 *   coordinates smoothly via func_8005A76C according to the team's tactical state
 *   byte D_8010C1D8[0xD + side] (cases 0..6 via jtbl_8001466C) and the player's
 *   formation slot (+0x391):
 *     - Centering Z toward 0 based on ball depth/possession
 *     - Pushing Z wide toward +/-0x2100 (+/-33.792 m)
 *     - Tracking ball X/Z (with +/-0x300 offset) via jtbl_8001468C
 *     - Pulling X back toward defensive/offside line Scratchpad+0x314[!dir] - dir_sign*0x200
 *       via jtbl_800146EC.
 *
 * Arguments:
 *   actor (unsigned char *): pointer to 1000-byte player actor struct.
 *   outX  (signed short *) : pointer to target X coordinate (R/W).
 *   outZ  (signed short *) : pointer to target Z coordinate (R/W).
 *
 * Return value:
 *   void.
 *
 * Scratchpad (0x1F800000) reads:
 *   - 0x29A (u8): match phase (1 = open play; otherwise early return).
 *   - 0x2CF (u8[2]): per-side possession flags (indexed by actor+0x398).
 *   - 0x2F4 (u8 *): pointer to ball actor (+0x02 s16 x, +0x06 s16 z).
 *   - 0x314 (s32[2]): defensive/offside X line per direction (indexed by dir ^ 1).
 *
 * Actor fields read:
 *   - +0x391 (u8): formation slot index.
 *   - +0x398 (u8): side (0/1).
 *   - +0x399 (u8): attack direction flag (0/1).
 *
 * Globals read:
 *   - D_8010C1D8 (+0x0D + side): team AI tactical mode byte (0..6).
 *   - jtbl_8001466C (7 entries), jtbl_8001468C (24 entries), jtbl_800146EC (8 entries).
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_8010C1D8[];
extern void *jtbl_8001466C[];
extern void *jtbl_8001468C[];
extern void *jtbl_800146EC[];

extern signed short func_8005A76C(int current, int target, int step);

void func_800A3084(unsigned char *actor, signed short *outX, signed short *outZ)
{
    signed short dir_sign;
    unsigned int tac_mode;
    int step = 0;

    if (B(Scratchpad, 0x29A) != 1u) {
        return;
    }

    dir_sign = (B(actor, 0x399) == 0) ? 1 : -1;
    tac_mode = B(D_8010C1D8, B(actor, 0x398) + 0xD);
    if (tac_mode >= 7u) {
        return;
    }

    /*
     * UNCLEAR: exact mapping of jtbl_8001466C[0..6] to the 5 jlabel arms below
     * (rodata jump table jtbl_8001466C is not present in asm_compact).
     */
    goto *jtbl_8001466C[tac_mode];

arm_L800A3110:
    {
        unsigned int poss = B(Scratchpad, 0x2CF + B(actor, 0x398));
        unsigned int slot;

        step = 2;
        if (poss != 1u) {
            unsigned int atk_dir = B(actor, 0x399);
            signed short bx = S((unsigned char *)W(Scratchpad, 0x2F4), 0x2);

            if ((atk_dir == 0u && bx < 0) || (atk_dir == 1u && bx > 0)) {
                step = 2;
            } else {
                int abs_bx = (bx >= 0) ? bx : -bx;
                if (abs_bx < 0x110A) {
                    step = 2;
                } else if (abs_bx < 0x1990) {
                    step = 3;
                } else {
                    step = 4;
                }
            }
        }

        slot = B(actor, 0x391);
        if (slot < 17u || slot == 19u || slot == 20u || slot >= 25u) {
            *outZ = func_8005A76C(*outZ, 0, step);
        }
        return;
    }

arm_L800A31E8:
    {
        unsigned int slot = B(actor, 0x391);
        if (slot != 0u && slot < 6u) {
            return;
        }
        step = (B(Scratchpad, 0x2CF + B(actor, 0x398)) == 1u) ? 2 : 3;
        if (B(actor, 0x399) != 0) {
            if (*outZ < 0) {
                *outZ = func_8005A76C(*outZ, -0x2100, step);
            }
        } else {
            if (*outZ > 0) {
                *outZ = func_8005A76C(*outZ, 0x2100, step);
            }
        }
        return;
    }

arm_L800A3264:
    {
        unsigned int slot = B(actor, 0x391);
        if (slot != 0u && slot < 6u) {
            return;
        }
        step = (B(Scratchpad, 0x2CF + B(actor, 0x398)) == 1u) ? 2 : 3;
        if (B(actor, 0x399) != 0) {
            if (*outZ > 0) {
                *outZ = func_8005A76C(*outZ, 0x2100, step);
            }
        } else {
            if (*outZ < 0) {
                *outZ = func_8005A76C(*outZ, -0x2100, step);
            }
        }
        return;
    }

arm_L800A32E0:
    {
        unsigned int slot_m1;
        unsigned char *ball;

        step = (B(Scratchpad, 0x2CF + B(actor, 0x398)) == 1u) ? 3 : 1;
        slot_m1 = B(actor, 0x391) - 1u;
        if (slot_m1 >= 0x18u) {
            goto arm_L800A3370;
        }

        /*
         * UNCLEAR: exact mapping of jtbl_8001468C[0..23] to the 3 jlabel arms below
         * (rodata jump table jtbl_8001468C is not present in asm_compact).
         */
        goto *jtbl_8001468C[slot_m1];

    arm_L800A3330:
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        *outZ = func_8005A76C(*outZ, S(ball, 0x6), step - 1);
        return;

    arm_L800A3344:
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        *outX = func_8005A76C(*outX, S(ball, 0x2), 1);
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        *outZ = func_8005A76C(*outZ, S(ball, 0x6), step - 1);
        return;

    arm_L800A3370:
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        *outX = (signed short)(func_8005A76C(*outX, S(ball, 0x2), (step & 0xFF) - 1) - ((int)dir_sign * 0x300));
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        *outZ = func_8005A76C(*outZ, S(ball, 0x6), step & 0xFF);
        return;
    }

arm_L800A33C4:
    {
        unsigned int slot_m1 = B(actor, 0x391) - 1u;
        int line_x;

        if (slot_m1 >= 8u) {
            return;
        }

        /*
         * UNCLEAR: exact mapping of jtbl_800146EC[0..7] to the 2 jlabel arms below
         * (rodata jump table jtbl_800146EC is not present in asm_compact).
         */
        goto *jtbl_800146EC[slot_m1];

    arm_L800A33F4:
        step = 3;
        goto pull_line;

    arm_L800A33FC:
        step = 5;

    pull_line:
        line_x = SW(Scratchpad, 0x314 + ((B(actor, 0x399) ^ 1u) << 2));
        *outX = func_8005A76C(*outX, line_x - ((int)dir_sign << 9), step);
        return;
    }
}
