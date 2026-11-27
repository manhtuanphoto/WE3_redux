/*
 * func_80043018
 *
 * Purpose:
 *   Evaluates whether a player actor should attempt a challenge/interception/shot-
 *   pressure decision based on match mode, ball ownership, ball position/heading,
 *   distance, and offside/defensive line thresholds.
 *
 * Arguments:
 *   actor (u8*) - pointer to the 1000-byte player actor struct
 *   angle (u32) - candidate heading angle byte
 *   dist  (u32) - distance metric to ball/target
 *
 * Return value:
 *   int - 1 if condition/action is triggered, 0 otherwise
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x04A (s16) reference x position (R)
 *     +0x2A9 (u8)  ball owner ID (0 = free) (R)
 *     +0x2F4 (u8*) pointer to ball actor (R)
 *     +0x314 (s32[2]) per-attack-direction x line thresholds (R)
 *     +0x31C (s16) ball timer (frames since last touch) (R)
 *     +0x31E (u16[2]) per-side nearest/marked actor ID (R)
 *     +0x328 (u8)  tactical/pressure mode (0 = inactive, 1 = mode 1, >1 = mode 2+) (R)
 *     +0x3D0 (u8)  per-attack-direction bitmask override (R)
 *   Actor:
 *     +0x38D (u8)  actor ID (R)
 *     +0x396 (u8)  state (0xF checked) (R)
 *     +0x399 (u8)  attack direction flag (R)
 *     +0x3B9 (u8)  player stat / attribute byte (divided by 10) (R)
 *     +0x3D0 (u8)  cooldown flag (R)
 *   Opponent actor (D_800FF748 + (id % 24) * 1000):
 *     +0x3A0 (u32) speed (R)
 *   Ball (Scratchpad+0x2F4):
 *     +0x002 (s16) ball x position (R)
 *     +0x006 (s16) ball z position (R)
 *     +0x3A8 (u8)  ball heading/direction byte (R)
 *   Globals:
 *     D_800C6D60 (u8[]) table indexed by Scratchpad+0x328 (R)
 *     D_800FF748 (u8[]) actor array (24 x 1000 bytes) (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x12F4 = 4852 raw = 19.408 m (penalty area half-width z)
 *   0x11F5 = 4597 raw = 18.388 m (penalty area z threshold)
 *   -0x1BE2 = -7138 raw = -28.552 m (x threshold in mode 1)
 *   -0x21E2 = -8674 raw = -34.696 m (x threshold in mode != 1)
 *   -0x27E2 = -10210 raw = -40.840 m (reference x threshold)
 *   0x3320 = 13088 raw = 52.352 m (goal line x)
 *   0x280  = 640 raw = 2.56 m; 0x180 = 384 raw = 1.536 m
 *   0x48 - (abs(x - 0x3320) * 32) / 26176 = 72 - abs(x - 13088) / 818 (angle cone byte)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800C6D60[];
extern unsigned char D_800FF748[];

extern unsigned int func_8005CE30(unsigned int);

int func_80043018(unsigned char* actor, unsigned int angle, unsigned int dist) {
    unsigned char mode = B(Scratchpad, 0x328);
    unsigned char owner;
    unsigned char atkDir;
    unsigned char* oppActor;
    unsigned char* ball;
    unsigned int maxAngleDiff;
    int absZ;
    int signedX;
    int refX;
    unsigned int ballDir;
    unsigned int diff;

    if (mode == 0) {
        return 0;
    }

    owner = B(Scratchpad, 0x2A9);
    if (owner == 0) {
        int threshold;
        if (B(actor, 0x3D0) != 0) {
            return 0;
        }
        threshold = (int)B(D_800C6D60, mode) - (int)((B(actor, 0x3B9) / 10U) & 0xFF);
        return (threshold + 2 < S(Scratchpad, 0x31C)) ? 1 : 0;
    }

    if (owner == (func_8005CE30(B(actor, 0x38D)) & 0xFF)) {
        return 1;
    }

    atkDir = B(actor, 0x399);
    oppActor = D_800FF748 + (B(Scratchpad, 0x31E + ((atkDir ^ 1) << 1)) % 24) * 0x3E8;

    if ((B(Scratchpad, 0x3D0) >> atkDir) & 1) {
        return 1;
    }

    if (B(Scratchpad, 0x328) == 1) {
        if (B(actor, 0x396) == 0xF) {
            ball = (unsigned char*)W(Scratchpad, 0x2F4);
            absZ = S(ball, 6);
            if (absZ < 0) {
                absZ = -absZ;
            }
            if (absZ < 0x12F4) {
                signedX = S(ball, 2);
                if (atkDir != 0) {
                    signedX = -signedX;
                }
                if (signedX < -0x1BE2) {
                    return 1;
                }
            }
        } else {
            ballDir = B(W(Scratchpad, 0x2F4), 0x3A8);
            diff = (ballDir - angle) & 0xFF;
            if (diff < 0x81) {
                if (diff >= 0x50) {
                    return 0;
                }
            } else if (((angle - ballDir) & 0xFF) >= 0x50) {
                return 0;
            }
        }

        maxAngleDiff = 0x60;
        ball = (unsigned char*)W(Scratchpad, 0x2F4);
        absZ = S(ball, 6);
        if (absZ < 0) {
            absZ = -absZ;
        }
        if (absZ < 0x11F5) {
            refX = S(Scratchpad, 0x4A);
            if (B(actor, 0x399) != 0) {
                refX = -refX;
            }
            if (refX >= -0x27E2) {
                return 0;
            }
        }
    } else {
        int distFail = 0;

        if (B(actor, 0x396) == 0xF) {
            ball = (unsigned char*)W(Scratchpad, 0x2F4);
            absZ = S(ball, 6);
            if (absZ < 0) {
                absZ = -absZ;
            }
            if (absZ < 0x12F4) {
                signedX = S(ball, 2);
                if (atkDir != 0) {
                    signedX = -signedX;
                }
                if (signedX < -0x21E2) {
                    return 1;
                }
            }
            distFail = (dist < 0x280U);
        } else {
            ballDir = B(W(Scratchpad, 0x2F4), 0x3A8);
            diff = (ballDir - angle) & 0xFF;
            if (diff < 0x81) {
                if (diff >= 0x74) {
                    return 0;
                }
            } else if (((angle - ballDir) & 0xFF) >= 0x74) {
                return 0;
            }
            distFail = (dist < 0x280U);
        }

        if (!distFail) {
            return 1;
        }
        if (W(oppActor, 0x3A0) >= 0x31U && dist >= 0x180U) {
            return 1;
        }

        maxAngleDiff = 0x70;
        ball = (unsigned char*)W(Scratchpad, 0x2F4);
        absZ = S(ball, 6);
        if (absZ < 0) {
            absZ = -absZ;
        }
        if (absZ < 0x11F5) {
            unsigned int dir = B(actor, 0x399);
            refX = S(Scratchpad, 0x4A);
            if (dir != 0) {
                refX = -refX;
            }
            if (refX >= -0x27E2) {
                int lineX = SW(Scratchpad, 0x314 + dir * 4);
                int curRefX = S(Scratchpad, 0x4A);
                if (dir != 0) {
                    if (-curRefX < -lineX) {
                        goto check_angle_cone;
                    }
                } else {
                    if (curRefX < lineX) {
                        goto check_angle_cone;
                    }
                }

                signedX = S(W(Scratchpad, 0x2F4), 2);
                if (B(actor, 0x399) != 0) {
                    signedX = -signedX;
                }
                signedX -= 0x3320;
                if (signedX < 0) {
                    signedX = -signedX;
                }
                /* 72 - abs(x - 13088) / 818 */
                maxAngleDiff = (unsigned int)(0x48 - ((signedX << 5) / 26176));
            }
        }
    }

check_angle_cone:
    ballDir = B(W(Scratchpad, 0x2F4), 0x3A8);
    maxAngleDiff &= 0xFF;
    diff = (angle - ballDir) & 0xFF;
    if (diff < 0x81) {
        return ((int)maxAngleDiff < (int)diff) ? 0 : 1;
    }
    return ((int)maxAngleDiff < (int)((ballDir - angle) & 0xFF)) ? 0 : 1;
}
