/*
 * func_8005346C - Check if goalkeeper / defender should initiate airborne or box challenge
 *
 * Purpose:
 *   Evaluates ball ownership, kick mode, ball trajectory/velocity, penalty-box
 *   bounds, and the opposing carrier's state (via jump table jtbl_80011DCC) to
 *   decide whether `actor` should transition to state 0x24 (substate 0) or
 *   state 0x1B (substate 0, Actor +0x3C2 = 0xFF), returning 0 or 1.
 *
 * Arguments:
 *   actor (u8*) - pointer to 1000-byte player actor struct
 *
 * Return value:
 *   unsigned int - 0 or 1.
 *
 * Scratchpad / Actor fields read & written:
 *   Scratch 0x04A (s16) - predicted/ball-path x coordinate
 *   Scratch 0x04E (s16) - predicted/ball-path y height
 *   Scratch 0x07A (s16) - predicted/ball-path z coordinate
 *   Scratch 0x29A (u8)  - active play / check enable flag (must be 1)
 *   Scratch 0x2A9 (u8)  - ball owner actor ID (must be 0)
 *   Scratch 0x2B0 (u32) - kick/play mode
 *   Scratch 0x2B4 (s16) - target/bounce x coordinate (0x7FFF = unset)
 *   Scratch 0x2B6 (s16) - target/bounce z coordinate
 *   Scratch 0x2F4 (u8*) - pointer to ball actor struct (reads Ball +0x0C s32 vy)
 *   Scratch 0x31C (s16) - ball timer (checked against 8 or 0x10)
 *   Scratch 0x31E[2] (u8) - designated actor index per attack direction
 *   Scratch 0x328 (u8)  - AI permission flag
 *   Actor +0x396  (u8)  - state (0x24 or 0x1B written)
 *   Actor +0x397  (u8)  - substate (cleared on state transition)
 *   Actor +0x399  (u8)  - attack direction flag
 *   Actor +0x39A  (u8)  - CPU control flag
 *   Actor +0x3C2  (u16) - set to 0xFF on state 0x1B transition
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];

extern unsigned int func_80052C6C(void);

unsigned int func_8005346C(unsigned char *actor)
{
    signed short target_timer;
    unsigned int mode;
    unsigned int opp_idx;
    unsigned char *opp;
    unsigned char opp_state;

    if (B(Scratchpad, 0x2A9) != 0) {
        return 0;
    }
    if (W(Scratchpad, 0x2B0) == 1) {
        return 0;
    }
    if ((func_80052C6C() & 0xFF) != 0) {
        return 0;
    }
    if (B(Scratchpad, 0x29A) != 1) {
        return 0;
    }

    target_timer = (S(Scratchpad, 0x2B4) == 0x7FFF) ? 8 : 0x10;
    if (S(Scratchpad, 0x31C) != target_timer) {
        return 0;
    }

    if (B(Scratchpad, 0x2A9) == 0) {
        int check_box = 0;

        if (S(Scratchpad, 0x04E) < -0xFF) {
            check_box = 1;
        } else {
            unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
            int vy = SW(ball, 0x0C);
            int abs_vy = (vy >= 0) ? vy : -vy;
            if (abs_vy >= 0x1700) {
                check_box = 1;
            }
        }

        if (check_box) {
            int x, z, abs_z;

            if (S(Scratchpad, 0x2B4) == 0x7FFF) {
                x = S(Scratchpad, 0x04A);
                z = S(Scratchpad, 0x07A);
            } else {
                x = S(Scratchpad, 0x2B4);
                z = S(Scratchpad, 0x2B6);
            }

            if (B(actor, 0x399) != 0) {
                x = -x;
            }

            if (x < -0x23E2) {
                abs_z = (z >= 0) ? z : -z;
                if (abs_z < 0x12F4) {
                    if (B(actor, 0x39A) != 0 && B(Scratchpad, 0x328) == 0) {
                        return 0;
                    }
                    B(actor, 0x396) = 0x24;
                    B(actor, 0x397) = 0;
                    return 1;
                }
            }
        }
    }

    if (B(actor, 0x396) == 0x1B) {
        return 0;
    }

    mode = W(Scratchpad, 0x2B0);
    if (mode >= 2 && (mode < 5 || mode == 8)) {
        return 0;
    }

    opp_idx = B(Scratchpad, 0x31E + (B(actor, 0x399) << 1)) % 24U;
    opp = D_800FF748 + opp_idx * 0x3E8;
    opp_state = B(opp, 0x396);

    if (opp_state >= 0x1C) {
        B(actor, 0x396) = 0x1B;
        B(actor, 0x397) = 0;
        H(actor, 0x3C2) = 0xFF;
        return 1;
    }

    switch (opp_state) {
    case 0:
    case 1:
    case 3:
    case 15: /* .L800536E8 */
        return 0;

    case 27: /* .L800536D4 */
        if (B(actor, 0x397) < 0x30) {
            return 0;
        }
        B(actor, 0x396) = 0x1B;
        B(actor, 0x397) = 0;
        H(actor, 0x3C2) = 0xFF;
        return 1;

    case 2:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26: /* .L800536F0 */
    default:
        B(actor, 0x396) = 0x1B;
        B(actor, 0x397) = 0;
        H(actor, 0x3C2) = 0xFF;
        return 1;
    }
}
