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

    switch (tac_mode) {
    case 0:
    case 4:
    default:
        /* .L800A342C */
        return;

    case 1:
        /* .L800A3110 */
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

    case 2:
        /* .L800A3264 */
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

    case 3:
        /* .L800A31E8 */
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

    case 5:
        /* .L800A32E0 */
        {
            unsigned int slot_m1;
            unsigned char *ball;

            step = (B(Scratchpad, 0x2CF + B(actor, 0x398)) == 1u) ? 3 : 1;
            slot_m1 = B(actor, 0x391) - 1u;

            switch (slot_m1) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
                /* .L800A3330 */
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                *outZ = func_8005A76C(*outZ, S(ball, 0x6), step - 1);
                return;

            case 5:
            case 6:
            case 7:
                /* .L800A3344 */
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                *outX = func_8005A76C(*outX, S(ball, 0x2), 1);
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                *outZ = func_8005A76C(*outZ, S(ball, 0x6), step - 1);
                return;

            case 16:
            case 17:
            case 18:
            case 19:
            case 20:
            case 21:
            case 22:
            case 23:
                /* .L800A342C */
                return;

            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            default:
                /* .L800A3370 */
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                *outX = (signed short)(func_8005A76C(*outX, S(ball, 0x2), (step & 0xFF) - 1) - ((int)dir_sign * 0x300));
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                *outZ = func_8005A76C(*outZ, S(ball, 0x6), step & 0xFF);
                return;
            }
        }

    case 6:
        /* .L800A33C4 */
        {
            unsigned int slot_m1 = B(actor, 0x391) - 1u;
            int line_x;

            switch (slot_m1) {
            case 0:
            case 1:
            case 4:
                /* .L800A33FC */
                step = 5;
                break;

            case 5:
            case 6:
            case 7:
                /* .L800A33F4 */
                step = 3;
                break;

            case 2:
            case 3:
            default:
                /* .L800A342C */
                return;
            }

            line_x = SW(Scratchpad, 0x314 + ((B(actor, 0x399) ^ 1u) << 2));
            *outX = func_8005A76C(*outX, line_x - ((int)dir_sign << 9), step);
            return;
        }
    }
}
