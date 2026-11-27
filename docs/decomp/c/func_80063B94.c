/*
 * func_80063B94 - AI ball-carrier decision / pass-or-dribble selector
 *
 * Purpose:
 *   Evaluates options for the AI player with the ball:
 *   1. If near either goal mouth (|x| >= 0x23E3 and |z| < 0x8B6) and
 *      func_800652EC(actor, 0x400) succeeds, triggers a shot/clear decision by
 *      recording Scratchpad+0x12[side] into actor+0x3D4 and clearing D_8010C1EC/ED/DB.
 *   2. Otherwise queries forward pass/option candidate via func_800655BC(dir<<7, 4, 0x20);
 *      if found (!= 0xFF), sets D_8010C1D8+0x14 = 2, +0x15 = 0x10, +0x02 = candidate.
 *   3. Otherwise scans up to 5 dribble directions via jtbl_80012498 and func_80065440
 *      (straight, +/-0x20, +/-0x40), clamping to straight if |z| >= 0x1D01.
 *   4. Optionally checks a wider pass via func_800655BC (0x40 and 0x38 ranges) or
 *      func_800652EC; otherwise, if deep in own half (dir==0 && ball_x < -0x1780, or
 *      dir==1 && ball_x >= 0x1781), initiates dribble/turn via func_8005E09C.
 *
 * Arguments:
 *   actor (unsigned char *): pointer to 1000-byte player actor struct.
 *
 * Return value:
 *   void.
 *
 * Scratchpad (0x1F800000) reads:
 *   - 0x012 (u16[2], R): per-side value copied to actor+0x3D4 on shot/clear trigger.
 *   - 0x2F4 (u8 *, R): pointer to ball actor (+0x02 s16 x).
 *
 * Actor fields read/written:
 *   - +0x02/+0x06 (s16, R): actor X/Z coordinates.
 *   - +0x398 (u8, R): side (0/1).
 *   - +0x399 (u8, R): attack direction flag (0/1).
 *   - +0x3D4 (u16, W): receives Scratchpad+0x012[side].
 *
 * Globals read/written:
 *   - D_8010C1D8 (+0x02, +0x03 [D_8010C1DB], +0x13, +0x14 [D_8010C1EC], +0x15 [D_8010C1ED]):
 *     AI ball-carrier decision state struct.
 *   - jtbl_80012498: jump table for 5 dribble direction probes (0..4).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x23E3 (9187 raw = 36.748 m): goal-area |X| threshold.
 *   - 0x8B6  (2230 raw = 8.92 m): goal-area |Z| threshold.
 *   - 0x1D01 (7425 raw = 29.70 m): touchline |Z| threshold forcing straight heading.
 *   - 0x1780 (6016 raw = 24.064 m): own-half ball X threshold for func_8005E09C dribble.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_8010C1D8[];
extern unsigned char D_8010C1EC; /* D_8010C1D8 + 0x14 */
extern unsigned char D_8010C1ED; /* D_8010C1D8 + 0x15 */
extern unsigned char D_8010C1DB; /* D_8010C1D8 + 0x03 */

extern unsigned int func_800652EC(unsigned char *actor, int arg1);
extern unsigned int func_800655BC(unsigned char *actor, unsigned int angle, int arg2, int range);
extern unsigned int func_80065440(unsigned char *actor, unsigned int angle, int arg2, int threshold);
extern int func_8001E6B8(int range);
extern void func_8005E09C(unsigned char *actor, unsigned int dir, int arg2);

void func_80063B94(unsigned char *actor)
{
    signed short ax = S(actor, 0x2);
    signed short az = S(actor, 0x6);
    int abs_x = (ax >= 0) ? ax : -ax;
    int abs_z = (az >= 0) ? az : -az;
    unsigned int cand;
    unsigned int best_dir;
    unsigned int probe_idx;
    unsigned int base_dir;

    if (abs_x >= 0x23E3 && abs_z < 0x8B6) {
        if ((func_800652EC(actor, 0x400) & 0xFFu) != 0u) {
            H(actor, 0x3D4) = H(Scratchpad, 0x012 + (B(actor, 0x398) << 1));
            D_8010C1EC = 0;
            D_8010C1ED = 0;
            D_8010C1DB = 0;
            return;
        }
    }

    cand = func_800655BC(actor, B(actor, 0x399) << 7, 4, 0x20);
    if ((cand & 0xFFu) != 0xFFu) {
        B(D_8010C1D8, 0x14) = 2;
        B(D_8010C1D8, 0x15) = 0x10;
        B(D_8010C1D8, 0x02) = (unsigned char)cand;
        return;
    }

    best_dir = 0xFFu;
    for (probe_idx = 0; (probe_idx & 0xFFu) < 5u; probe_idx++) {
        unsigned int test_dir = 0;

        switch (probe_idx & 0xFFu) {
        case 0: /* .L80063CB0 */
            test_dir = B(actor, 0x399) << 7;
            break;
        case 1: /* .L80063CBC */
            base_dir = B(actor, 0x399) << 7;
            test_dir = (S(actor, 0x6) < 0) ? (base_dir | 0x20u) : (base_dir - 0x20u);
            break;
        case 2: /* .L80063CE8 */
            base_dir = B(actor, 0x399) << 7;
            test_dir = (S(actor, 0x6) < 0) ? (base_dir - 0x20u) : (base_dir | 0x20u);
            break;
        case 3: /* .L80063D14 */
            base_dir = B(actor, 0x399) << 7;
            test_dir = (S(actor, 0x6) < 0) ? (base_dir | 0x40u) : (base_dir - 0x40u);
            break;
        case 4: /* .L80063D40 */
            base_dir = B(actor, 0x399) << 7;
            test_dir = (S(actor, 0x6) < 0) ? (base_dir - 0x40u) : (base_dir | 0x40u);
            break;
        }

        if (func_80065440(actor, test_dir & 0xFFu, 0x40, 0x240000) != 0u) {
            best_dir = test_dir;
        }
        if ((best_dir & 0xFFu) != 0xFFu) {
            break;
        }
    }

    az = S(actor, 0x6);
    abs_z = (az >= 0) ? az : -az;
    if (abs_z >= 0x1D01) {
        best_dir = B(actor, 0x399) << 7;
    }

    if (func_8001E6B8(5) == 0 && B(D_8010C1D8, 0x13) != 0) {
        unsigned int wide_cand = func_800655BC(actor, B(actor, 0x399) << 7, 4, 0x40);
        if ((wide_cand & 0xFFu) != 0xFFu) {
            B(D_8010C1D8, 0x14) = 2;
            B(D_8010C1D8, 0x15) = 0x10;
            B(D_8010C1D8, 0x02) = (unsigned char)wide_cand;
            return;
        }

        {
            unsigned int lat_ang = (S(actor, 0x6) < 0) ? 0x40u : 0xC0u;
            cand = func_800655BC(actor, lat_ang, 4, 0x38);
            if ((cand & 0xFFu) != 0xFFu) {
                B(D_8010C1D8, 0x14) = 2;
                B(D_8010C1D8, 0x15) = 0x10;
                B(D_8010C1D8, 0x02) = (unsigned char)cand;
                return;
            }
        }

        if ((func_800652EC(actor, 0x400) & 0xFFu) != 0u) {
            H(actor, 0x3D4) = H(Scratchpad, 0x012 + (B(actor, 0x398) << 1));
            B(D_8010C1D8, 0x14) = 0;
            B(D_8010C1D8, 0x15) = 0;
            B(D_8010C1D8, 0x03) = 0;
            return;
        }
    } else if (func_8001E6B8(5) == 0 && B(D_8010C1D8, 0x13) == 0) {
        /* Note: in asm, when func_8001E6B8(5) == 0 and D_8010C1D8[0x13] == 0, branches directly to .L80063E74 */
        H(actor, 0x3D4) = H(Scratchpad, 0x012 + (B(actor, 0x398) << 1));
        B(D_8010C1D8, 0x14) = 0;
        B(D_8010C1D8, 0x15) = 0;
        B(D_8010C1D8, 0x03) = 0;
        return;
    }

    {
        unsigned int atk_dir = B(actor, 0x399);
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        signed short ball_x = S(ball, 0x2);

        if ((atk_dir == 0u && ball_x < -0x1780) || (atk_dir == 1u && ball_x >= 0x1781)) {
            if ((best_dir & 0xFFu) != 0xFFu) {
                func_8005E09C(actor, best_dir & 0xFFu, 0);
            }
        } else {
            B(D_8010C1D8, 0x14) = 0;
            B(D_8010C1D8, 0x15) = 0;
        }
    }
}
