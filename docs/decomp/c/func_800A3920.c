/*
 * func_800A3920 - Position all players into tactical formation targets
 *
 * Purpose:
 *   Iterates over all 22 player actors (skipping the player whose ID matches
 *   Scratchpad+0x2AD). Goalkeepers (IDs 1 and 12) are positioned on their goal
 *   line at x = +-12832, z = 0 with animation 0x5A (90). Outfield players fetch
 *   a base formation target via func_8005A85C, apply role- and possession-based
 *   horizontal (X) offsets and lateral (Z) compression/balancing relative to the
 *   ball, store the target in actor+0x3BC/+0x3BE, optionally blend toward the
 *   ball/center for roles 9..12 and 14..15 on the opposite flank, move via
 *   func_8009F554, and trigger run animation 1. Calls func_8009F574 at the end.
 *
 * Arguments:
 *   None (void).
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Actor fields read & written:
 *   Scratch 0x2AD      (u8)  - excluded actor ID (skip positioning)
 *   Scratch 0x2CF[2]   (u8)  - team possession flag per side (0/1)
 *   Scratch 0x2F4      (ptr) - pointer to ball actor (+0x02 s16 x, +0x06 s16 z)
 *   Scratch 0x2FD[2]   (u8)  - formation ID per side (0..16 checked)
 *   Actor +0x02        (s16) - player X position (read)
 *   Actor +0x06        (s16) - player Z position (read)
 *   Actor +0x38D       (u8)  - actor ID (1..22; 1 and 12 are goalkeepers)
 *   Actor +0x391       (u8)  - formation role ID (1..25)
 *   Actor +0x398       (u8)  - team side (0/1)
 *   Actor +0x399       (u8)  - attack direction flag (0 -> +X, 1 -> -X)
 *   Actor +0x3BC       (s16) - stored target X (written)
 *   Actor +0x3BE       (s16) - stored target Z (written)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   12832 (51.328 m) = goalkeeper goal-line X offset
 *   2816  (11.264 m) = defending X pullback for roles 1..8
 *   4096  (16.384 m) = defending X pullback for roles 17..25
 *   5120  (20.480 m) = defending X pullback for roles 14..16
 *   6144  (24.576 m) = defending X pullback for roles 9..13 / attacking push for 14..16
 *   5376  (21.504 m) = attacking X push for roles 17..25
 *   6656  (26.624 m) = attacking X push for roles 9..13
 *   6912  (27.648 m) = attacking X push for roles 6..8
 *   7168  (28.672 m) = attacking X push for roles 1..5
 *   768   (3.072 m)  = opposite-flank X blend target (+/-0x300)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];

extern void func_8009F554(unsigned char *actor, int x, int z);
extern void func_8009F574(void);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005A85C(unsigned char *actor, signed short *out_x, signed short *out_z);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8005A76C(int cur, int target, int divisor);

void func_800A3920(void)
{
    unsigned char *actor = D_800FF748 + 1000;
    int i;

    for (i = 0; i < 22; i++, actor += 1000) {
        unsigned char actor_id = B(actor, 0x38D);
        signed short dir;
        signed short tx;
        signed short tz;
        unsigned char role;
        unsigned char side;
        unsigned char *ball;

        if (actor_id == B(Scratchpad, 0x2AD)) {
            continue;
        }

        dir = (B(actor, 0x399) != 0) ? -1 : 1;

        /* Goalkeepers (actor IDs 1 and 12) go straight to their goal line */
        if (actor_id == 1 || actor_id == 12) {
            func_8009F554(actor, -(dir * 12832), 0);
            func_8005BA30(actor, 0x5A, 0);
            continue;
        }

        func_8005A85C(actor, &tx, &tz);
        side = B(actor, 0x398);
        role = B(actor, 0x391);

        if (B(Scratchpad, 0x2CF + side) == 0) {
            /* Defending / out of possession */
            switch (role) {
            case 1: case 2: case 3: case 4:
            case 5: case 6: case 7: case 8:
                tx = (signed short)((unsigned short)tx - (unsigned int)(dir * 2816));
                break;

            case 9: case 10: case 11: case 12: case 13:
                tx = (signed short)((unsigned short)tx - (unsigned int)(dir * 6144));
                switch (B(Scratchpad, 0x2FD + side)) {
                case 0: case 2: case 4: case 6:
                case 8: case 9: case 11: case 12:
                case 16: {
                    int tz_sign = (tz >= 0) ? 1 : -1;
                    ball = (unsigned char *)W(Scratchpad, 0x2F4);
                    if ((S(ball, 6) < 0 && tz_sign != -1) ||
                        (S(ball, 6) >= 0 && tz_sign != 1)) {
                        tz = (signed short)((tz * 7) / 10);
                    }
                    break;
                }
                default:
                    break;
                }
                break;

            case 14: case 15: case 16:
                tx = (signed short)((unsigned short)tx - (unsigned int)(dir * 5120));
                break;

            case 17: case 18: case 19: case 20:
                tz = (signed short)((tz * 8) / 10);
                /* fall through */
            case 21: case 22: case 23: case 24: case 25:
                tx = (signed short)((unsigned short)tx - (unsigned int)(dir * 4096));
                break;

            default:
                break;
            }
            tz = (signed short)((tz * 9) / 10);
        } else {
            /* Attacking / in possession */
            switch (role) {
            case 1: case 2: case 3: case 4: case 5:
                tx = (signed short)((unsigned short)tx + (unsigned int)(dir * 7168));
                break;

            case 6: case 7: case 8: {
                int tz_sign = (tz >= 0) ? 1 : -1;
                tx = (signed short)((unsigned short)tx + (unsigned int)(dir * 6912));
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                if ((S(ball, 6) < 0 && tz_sign != -1) ||
                    (S(ball, 6) >= 0 && tz_sign != 1)) {
                    tz = (signed short)(tz / 2);
                }
                break;
            }

            case 9: case 10: case 11: case 12: case 13: {
                int tz_sign = (tz >= 0) ? 1 : -1;
                tx = (signed short)((unsigned short)tx + (unsigned int)(dir * 6656));
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                if ((S(ball, 6) < 0 && tz_sign != -1) ||
                    (S(ball, 6) >= 0 && tz_sign != 1)) {
                    tz = (signed short)(tz / 2);
                }
                break;
            }

            case 14: case 15: {
                int tz_sign = (tz >= 0) ? 1 : -1;
                tx = (signed short)((unsigned short)tx + (unsigned int)(dir * 6144));
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                if ((S(ball, 6) < 0 && tz_sign != -1) ||
                    (S(ball, 6) >= 0 && tz_sign != 1)) {
                    tz = (signed short)((tz * 7) / 10);
                }
                break;
            }
            case 16: {
                int tz_sign = (tz >= 0) ? 1 : -1;
                tx = (signed short)((unsigned short)tx + (unsigned int)(dir * 6144));
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                if ((S(ball, 6) < 0 && tz_sign != -1) ||
                    (S(ball, 6) >= 0 && tz_sign != 1)) {
                    tz = (signed short)((tz * 7) / 10);
                }
                break;
            }

            case 17: case 18: case 19: case 20:
            case 21: case 22: case 23: case 24: case 25:
                tx = (signed short)((unsigned short)tx + (unsigned int)(dir * 5376));
                break;

            default:
                break;
            }
        }

        func_8005B36C(S(actor, 2), S(actor, 6), tx, tz);
        S(actor, 0x3BC) = tx;
        S(actor, 0x3BE) = tz;

        {
            int az_sign = (S(actor, 6) >= 0) ? 1 : -1;
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            if ((S(ball, 6) < 0 && az_sign != -1) ||
                (S(ball, 6) >= 0 && az_sign != 1)) {
                if ((role >= 9 && role < 13) || (role >= 14 && role < 16)) {
                    int blend_x = (S(ball, 2) > 0) ? -0x300 : 0x300;
                    tx = (signed short)func_8005A76C(tx, blend_x, 3);
                    tz = (signed short)func_8005A76C(tz, S(ball, 6), 2);
                }
            }
        }

        func_8009F554(actor, tx, tz);
        func_8005BA30(actor, 1, 0);
    }

    func_8009F574();
}
