/*
 * func_80068554
 *
 * Purpose:
 *   Evaluates whether an AI actor should sprint/dash toward its target
 *   (actor+0x3BC, actor+0x3BE) given the squared distance `dist_sq` to the
 *   target. Checks facing alignment (actor+0x3AA), pitch X bounds (< 0x25E3),
 *   team possession (Scratchpad+0x2CF[team]), and actor state (actor+0x391)
 *   to compare `dist_sq` against role-dependent squared distance thresholds.
 *
 * Arguments:
 *   actor:   (unsigned char*) pointer to player entity struct
 *   dist_sq: (unsigned int) squared distance to target (0x3BC, 0x3BE)
 *
 * Return value:
 *   1 if the actor meets the sprint/distance threshold, 0 otherwise.
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x005 (u8)  active/nearest player ID (R)
 *     +0x2CF (u8[2]) team ball-possession flags (== 1 means team has ball) (R)
 *     +0x2F4 (ptr) ball entity (+0x02 s16 x) (R)
 *     +0x30B (u8)  match/tactic state (== 4 forces 0x100000 threshold) (R)
 *   actor:
 *     +0x002 (s16) X position
 *     +0x006 (s16) Z position
 *     +0x38D (u8)  player ID
 *     +0x391 (u8)  actor state (1..0x18, jump tables jtbl_80012680 / jtbl_800126E0)
 *     +0x398 (u8)  team index (0 or 1)
 *     +0x399 (u8)  attacking side (0 or 1; goal direction = side << 7)
 *     +0x3AA (u8)  facing angle (0..255)
 *     +0x3BC (s16) target X
 *     +0x3BE (s16) target Z
 *     +0x3E1 (u8)  flag forcing 0x100000 threshold when team has possession
 *
 * Magic numbers:
 *   0x25E3 (9699): max |actor->x| to allow sprint
 *   0x40 / 0x20: max angle diff (with/without possession)
 *   0x100000..0x1000000: squared distance thresholds ((N*256)^2)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define P(p,o)  (*(unsigned char**)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);

int func_80068554(unsigned char *actor, unsigned int dist_sq) {
    unsigned char target_ang;
    unsigned char facing;
    unsigned char diff;
    signed short ax;

    target_ang = (unsigned char)func_8005B36C(S(actor, 0x02), S(actor, 0x06),
                                              S(actor, 0x3BC), S(actor, 0x3BE));
    facing = B(actor, 0x3AA);
    diff = (unsigned char)(target_ang - facing);
    if (diff >= 0x81) {
        diff = (unsigned char)(facing - target_ang);
    }
    if (diff >= 0x41) {
        return 0;
    }

    ax = S(actor, 0x02);
    if (((ax >= 0) ? ax : -ax) >= 0x25E3) {
        return 0;
    }

    if (B(Scratchpad, 0x2CF + B(actor, 0x398)) == 1) {
        unsigned int state_off;

        if (B(actor, 0x3E1) != 0) {
            return (0x100000U < dist_sq);
        }

        state_off = (unsigned int)(B(actor, 0x391) - 1);
        if (state_off >= 0x18) {
            return (0x510000U < dist_sq);
        }

        switch (state_off) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            /* .L80068728 */
            return (0x510000U < dist_sq);
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
            /* .L80068720 */
            return (0x400000U < dist_sq);
        case 13:
        case 14:
        case 15:
            /* .L800686B0 */
            if (0x190000U < dist_sq) {
                unsigned char *ball = P(Scratchpad, 0x2F4);
                if (B(actor, 0x399) != 0) {
                    if (S(actor, 0x02) < S(ball, 0x02) + 0x200) {
                        return 1;
                    }
                } else {
                    if (S(ball, 0x02) - 0x200 < S(actor, 0x02)) {
                        return 1;
                    }
                }
            }
            return (0x310000U < dist_sq);
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23: {
            /* .L80068638 */
            signed short dz;
            if (0x190000U < dist_sq) {
                int forward = (B(actor, 0x399) != 0)
                            ? (S(actor, 0x02) < S(actor, 0x3BC))
                            : (S(actor, 0x3BC) < S(actor, 0x02));
                if (forward) {
                    return 1;
                }
            }
            dz = (signed short)(S(actor, 0x06) - S(actor, 0x3BE));
            if (dz < 0) {
                dz = (signed short)-dz;
            }
            if (dz < 0x501) {
                return (0x310000U < dist_sq);
            }
            return 1;
        }
        default:
            return (0x510000U < dist_sq);
        }
    } else {
        unsigned char atk_ang = (unsigned char)(B(actor, 0x399) << 7);
        unsigned int state_off;

        diff = (unsigned char)(target_ang - atk_ang);
        if (diff >= 0x81) {
            diff = (unsigned char)(atk_ang - target_ang);
        }
        if (diff >= 0x21) {
            return 0;
        }

        if (B(actor, 0x38D) == B(Scratchpad, 0x005)) {
            return (0x240000U < dist_sq);
        }

        state_off = (unsigned int)(B(actor, 0x391) - 1);
        if (state_off >= 0x18) {
            return (0x1000000U < dist_sq);
        }

        switch (state_off) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4: {
            /* .L800687CC */
            unsigned char *ball = P(Scratchpad, 0x2F4);
            signed short actor_x = S(actor, 0x02);
            signed short ball_x = S(ball, 0x02);
            int dx = actor_x - ball_x;
            if (dx < 0) {
                dx = -dx;
            }
            if (dx < 0x600) {
                return (0x100000U < dist_sq);
            }
            if (B(actor, 0x399) != 0) {
                if (-actor_x < -ball_x) {
                    return (0x100000U < dist_sq);
                }
            } else {
                if (actor_x < ball_x) {
                    return (0x100000U < dist_sq);
                }
            }
            return (0x790000U < dist_sq);
        }
        case 5:
        case 6:
        case 7:
            /* .L80068834 */
            if (B(Scratchpad, 0x30B) == 4) {
                return (0x100000U < dist_sq);
            }
            if (0x310000U < dist_sq) {
                int moving = (B(actor, 0x399) != 0)
                           ? (S(actor, 0x3BC) < S(actor, 0x02))
                           : (S(actor, 0x02) < S(actor, 0x3BC));
                if (moving) {
                    return 1;
                }
            }
            return (0x640000U < dist_sq);
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
            /* .L800687C4 */
            return (0x900000U < dist_sq);
        case 13:
        case 14:
        case 15:
        case 18:
        case 19:
            /* .L800687BC */
            return (0xC40000U < dist_sq);
        case 16:
        case 17:
        case 20:
        case 21:
        case 22:
        case 23:
            /* .L800687AC */
            return 0;
        default:
            return (0x1000000U < dist_sq);
        }
    }
}
