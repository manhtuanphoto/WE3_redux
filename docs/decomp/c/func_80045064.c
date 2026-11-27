/*
 * func_80045064
 *
 * Purpose:
 *   Evaluates goalkeeper / defender reaction action state based on controller/AI
 *   input bitmasks (Scratchpad+0x12..0x2A), ball position/height/velocity, and
 *   distance to the ball. Returns an action/state ID (or 0xFF if none triggered).
 *
 * Arguments:
 *   actor (u8 *): pointer to 1000-byte actor struct
 *   arg1 (u32):   mode/phase flag tested via (arg1 & 0xFF) != 0
 *
 * Return value:
 *   int: selected action/state code (3, 4, 5, 6, 8, 9, 0xA, 0xD, 0xF, 0x24,
 *        0x2A, 0xFF, or result of func_80039A5C(actor) & 0xFF).
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x12[2] (u16) per-side input/action mask (checked against actor+0x3D4)
 *   +0x16[2] (u16), +0x1A[2] (u16), +0x1E[2] (u16), +0x2A[2] (u16)
 *            per-side action masks (checked against actor+0x3D6)
 *   +0x22[2] (u16), +0x26[2] (u16) per-side masks (checked against actor+0x3D4)
 *   +0x4E    (s16) predicted ball height / vertical threshold (-0x7F = -127 = 0.508 m up)
 *   +0x2A9   (u8)  ball owner actor ID (0 = free)
 *   +0x2CF[2](u8)  per-side possession flags
 *   +0x2F4   (u32) pointer to ball actor
 *   +0x31C   (s16) ball timer (frames since last touch; < 16 frames = 0.53 s)
 *
 * Actor fields read:
 *   +0x02 (s16) x, +0x06 (s16) z
 *   +0x396 (u8) state, +0x398 (u8) side (0/1), +0x399 (u8) attack direction flag
 *   +0x39A (u8) control/AI flag, +0x3C2 (s16) target/contact flag
 *   +0x3D4 (u16) action mask 1, +0x3D6 (u8) action mask 2
 *
 * Actor fields written:
 *   +0x3C2 (s16) cleared to 0 when returning 4 on ball proximity check
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   -0x7F (-127) = 0.508 m up; -0x50 (-80) = 0.32 m up;
 *   0x12F4 (4852) = 19.408 m; 0x20E3 (8419) = 33.676 m;
 *   0x21E3 (8675) = 34.700 m; -0x21E2 (-8674) = -34.696 m;
 *   0x23FFFF (distSq <= 1536^2) ~ 6.144 m; 0x3FFFFF (distSq <= 2048^2) ~ 8.192 m;
 *   0x10 (16 frames) = 0.533 s.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern int func_8005575C(unsigned char *actor);
extern unsigned int func_80039A5C(unsigned char *actor);

int func_80045064(unsigned char *actor, unsigned int arg1)
{
    unsigned char side;
    unsigned short mask3D4;
    unsigned char flags3D6;

    if ((H(Scratchpad, 0x12 + B(actor, 0x398) * 2) & H(actor, 0x3D4)) != 0 ||
        func_8005575C(actor) != 0) {
        if ((arg1 & 0xFF) != 0) {
            unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
            signed short ballX = S(ball, 2);

            if (B(actor, 0x399) != 0) {
                return (ballX > 0) ? 0xA : 4;
            }
            return (ballX < 0) ? 0xA : 4;
        }

        if (S(Scratchpad, 0x4E) >= -0x7F) {
            unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
            int ballVy = SW(ball, 0xC);
            int absVy = (ballVy < 0) ? -ballVy : ballVy;

            if (absVy < 0x1700 && B(Scratchpad, 0x2A9) == 0) {
                int ballZ = S(ball, 6);
                int absZ = (ballZ < 0) ? -ballZ : ballZ;

                if (absZ < 0x12F4) {
                    int ballX = S(ball, 2);
                    int dirBallX = (B(actor, 0x399) != 0) ? -ballX : ballX;

                    if (dirBallX >= 0x21E3) {
                        unsigned char *ballPtr = (unsigned char *)W(Scratchpad, 0x2F4);
                        int dx = S(actor, 2) - S(ballPtr, 2);
                        int dz = S(actor, 6) - S(ballPtr, 6);

                        if (dx * dx + dz * dz <= 0x23FFFF) {
                            S(actor, 0x3C2) = 0;
                            return 4;
                        }
                    }
                }
            }
        }

        {
            unsigned int nextState = func_80039A5C(actor) & 0xFF;
            if (nextState != B(actor, 0x396)) {
                return nextState;
            }
        }
    }

    flags3D6 = B(actor, 0x3D6);
    if (flags3D6 != 0) {
        if ((arg1 & 0xFF) != 0) {
            unsigned int sideOff = B(actor, 0x398) * 2;

            if ((H(Scratchpad, 0x16 + sideOff) & flags3D6) != 0) {
                return 5;
            }
            if ((H(Scratchpad, 0x1E + sideOff) & flags3D6) != 0) {
                return 6;
            }
            if ((H(Scratchpad, 0x1A + sideOff) & flags3D6) != 0) {
                int ballX;
                int dirBallX;
                int actorX;
                int dirActorX;

                if (B(actor, 0x39A) != 0 && S(actor, 0x3C2) == 0) {
                    return 9;
                }
                ballX = S((unsigned char *)W(Scratchpad, 0x2F4), 2);
                dirBallX = (B(actor, 0x399) != 0) ? -ballX : ballX;
                if (dirBallX >= 0x20E3) {
                    return 9;
                }
                actorX = S(actor, 2);
                dirActorX = (B(actor, 0x399) != 0) ? -actorX : actorX;
                return (dirActorX < -0x21E2) ? 0xA : 8;
            }
        } else {
            unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
            int ballVy = SW(ball, 0xC);
            int absVy = (ballVy < 0) ? -ballVy : ballVy;

            if (S(ball, 4) < -0x50 || absVy >= 0x1701) {
                if ((H(Scratchpad, 0x1A + B(actor, 0x398) * 2) & flags3D6) != 0) {
                    return 0x2A;
                }
            } else {
                unsigned char curSide = B(actor, 0x398);

                if ((H(Scratchpad, 0x2A + curSide * 2) & flags3D6) != 0) {
                    if (B(Scratchpad, 0x2A9) == 0 || B(Scratchpad, 0x2CF + curSide) == 1) {
                        int dx = S(actor, 2) - S(ball, 2);
                        int dz = S(actor, 6) - S(ball, 6);

                        if (dx * dx + dz * dz <= 0x3FFFFF) {
                            return 0xD;
                        }
                    }
                }
            }

            if ((H(Scratchpad, 0x16 + B(actor, 0x398) * 2) & flags3D6) != 0) {
                if (S((unsigned char *)W(Scratchpad, 0x2F4), 4) < -0x50) {
                    return 0x24;
                }
            }
        }
    }

    side = B(actor, 0x398);
    mask3D4 = H(actor, 0x3D4);
    if ((H(Scratchpad, 0x26 + side * 2) & mask3D4) != 0) {
        if (B(Scratchpad, 0x2A9) == 0 || B(Scratchpad, 0x2CF + side) == 1) {
            return 0xF;
        }
        mask3D4 = H(actor, 0x3D4);
    }

    if ((H(Scratchpad, 0x22 + side * 2) & mask3D4) == 0) {
        return 0xFF;
    }
    if (B(Scratchpad, 0x2A9) != 0) {
        return 3;
    }
    if (B(Scratchpad, 0x2CF + side) == 0) {
        return 3;
    }
    if (S(Scratchpad, 0x31C) < 0x10) {
        return 0xFF;
    }
    return 3;
}
