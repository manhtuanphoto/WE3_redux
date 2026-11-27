/*
 * func_80090EE0
 *
 * Purpose:
 *   Evaluates whether a player (typically a goalkeeper) can intercept/dive for
 *   the ball at the predicted ball target (Scratchpad +0x48/+0x60/+0x78) based
 *   on player stats, ball height, time since last touch, squared distance, and
 *   ball travel direction, and transitions the actor into state 0x90, 0x9A, or 0x8C.
 *
 * Arguments:
 *   actor      (u8 *)  : pointer to 1000-byte actor struct
 *   distSq     (u32)   : squared distance from actor to target point (raw^2)
 *   otherDistSq(u32)   : squared distance from comparison player/point to target (raw^2)
 *   angle1     (u8)    : angle byte (0..255 = 360 deg) from actor to target
 *   angle2     (u8)    : reference angle byte (0..255 = 360 deg)
 *
 * Return value:
 *   1 if an interception/dive state (0x90, 0x9A, or 0x8C) was triggered;
 *   0 if the ball is out of vertical/horizontal reach or touch cooldown has not elapsed.
 *
 * Actor fields read:
 *   +0x02 (s16) x position, +0x06 (s16) z position,
 *   +0x392 (u8) team slot (0..10), +0x398 (u8) side (0/1),
 *   +0x3B9 (u8) stat*10 (used as cooldown divisor / 10)
 *
 * Actor fields written:
 *   +0x396 (u8) state (0x90, 0x9A, or 0x8C), +0x397 (u8) substate (0),
 *   +0x3AB (u8) target heading byte, +0x3AC (u8) mirror/foot side (0 or 1),
 *   +0x3C2/+0x3C4/+0x3C6 (u16) target x/y/z coordinates,
 *   +0x3D1 (u8) cleared to 0
 *
 * Global / Scratchpad (0x1F800000) fields read:
 *   Scratchpad +0x48/+0x60/+0x78 (s16) predicted ball x/y/z,
 *   Scratchpad +0x2B0 (u32) ball event code (checked == 4),
 *   Scratchpad +0x2F4 (u8*) pointer to ball actor (reads +0x3A4 move heading),
 *   Scratchpad +0x31C (s16) ball timer (frames since last touch),
 *   D_8010C330 / D_8010C32F (player stat table, stride 0x108 per side, 12 per slot)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   stat = ((D_8010C330[offset] >> 11) & 0x7F) + 100 (range 100..227).
 *   Vertical bounds: -(stat * 608) / 180 (-1.35..-3.06 m, negative = up) and
 *   -(stat * 736) / 180 + 6 - reach_mod (-1.63..-3.71 m).
 *   Cooldown: 31 - stat_3B9 / 10 frames (at 30 Hz).
 *   Horizontal reach thresholds: (stat * {768, 576, 512, 240} / 180)^2 raw^2.
 *   0x40000 raw^2 = 262144 (= (512 raw)^2 = (2.048 m)^2).
 *   Angular window: 0x21..0x5F (33..95 / 256 = ~46..134 deg).
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_8010C32F[];
extern unsigned int D_8010C330[];

extern unsigned int func_8001E6B8(int n);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);

int func_80090EE0(unsigned char *actor, unsigned int distSq, unsigned int otherDistSq, unsigned char angle1, unsigned char angle2)
{
    int tblOff;
    int stat;
    int minY;
    int maxY;
    int cooldown;
    int reachScaled;
    unsigned int reachSq;
    unsigned char *ball;
    int ballDir;
    int angleDiff;

    tblOff = (int)B(actor, 0x392) * 12 + (int)B(actor, 0x398) * 0x108;
    stat = (int)((W((unsigned char *)D_8010C330, tblOff) >> 11) & 0x7F) + 100;

    /* Vertical reach checks (negative y = up, 250 raw = 1 m) */
    minY = -(stat * 608) / 180;
    if ((int)S(Scratchpad, 0x60) > minY) {
        return 0;
    }

    maxY = -(stat * 736) / 180 + 6 - (int)(B(D_8010C32F, tblOff) & 0xF);
    if ((int)S(Scratchpad, 0x60) < maxY) {
        return 0;
    }

    /* Minimum frames since last touch (at 30 Hz) */
    cooldown = 31 - (int)((B(actor, 0x3B9) / 10U) & 0xFF);
    if ((int)S(Scratchpad, 0x31C) < cooldown) {
        return 0;
    }

    /* Outer horizontal reach limit */
    reachScaled = stat * 768;
    reachSq = (unsigned int)(((reachScaled / 180) * reachScaled) / 180);
    if (reachSq < distSq) {
        return 0;
    }

    if (otherDistSq < distSq + 0x40000U) {
        reachScaled = stat * 576;
        reachSq = (unsigned int)(((reachScaled / 180) * reachScaled) / 180);
        if (distSq < reachSq) {
            goto check_closer;
        }
    }

    reachScaled = stat * 512;
    reachSq = (unsigned int)(((reachScaled / 180) * reachScaled) / 180);
    if (reachSq < distSq || W(Scratchpad, 0x2B0) == 4 || func_8001E6B8(3) == 0) {
        if (otherDistSq < distSq) {
            B(actor, 0x396) = 0x90;
            B(actor, 0x397) = 0;
            H(actor, 0x3C2) = H(Scratchpad, 0x48);
            H(actor, 0x3C4) = H(Scratchpad, 0x60);
            H(actor, 0x3C6) = H(Scratchpad, 0x78);
            B(actor, 0x3D1) = 0;
            return 1;
        }
        goto check_closer_post;
    }

    B(actor, 0x396) = 0x90;
    B(actor, 0x397) = 0;
    H(actor, 0x3C2) = H(Scratchpad, 0x48);
    H(actor, 0x3C4) = H(Scratchpad, 0x60);
    H(actor, 0x3C6) = H(Scratchpad, 0x78);
    B(actor, 0x3D1) = 0;
    return 1;

check_closer:
    if (distSq < otherDistSq) {
check_closer_post:
        if (distSq < otherDistSq) {
            tblOff = (int)B(actor, 0x392) * 12 + (int)B(actor, 0x398) * 0x108;
            stat = (int)((W((unsigned char *)D_8010C330, tblOff) >> 11) & 0x7F) + 100;
            reachScaled = stat * 512;
            reachSq = (unsigned int)(((reachScaled / 180) * reachScaled) / 180);
            if (reachSq < distSq) {
                return 0;
            }
        }
    }

    B(actor, 0x3D1) = 0;
    H(actor, 0x3C2) = H(Scratchpad, 0x48);
    H(actor, 0x3C4) = H(Scratchpad, 0x60);
    H(actor, 0x3C6) = H(Scratchpad, 0x78);

    tblOff = (int)B(actor, 0x392) * 12 + (int)B(actor, 0x398) * 0x108;
    stat = (int)((W((unsigned char *)D_8010C330, tblOff) >> 11) & 0x7F) + 100;
    reachScaled = stat * 512;
    reachSq = (unsigned int)(((reachScaled / 180) * reachScaled) / 180);
    if (distSq >= reachSq) {
        B(actor, 0x396) = 0x90;
        B(actor, 0x397) = 0;
        return 1;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    ballDir = (int)B(ball, 0x3A4);
    angleDiff = ballDir - (int)angle1;
    if ((unsigned int)(angleDiff & 0xFF) >= 0x81U) {
        angleDiff = (int)angle1 - ballDir;
    }

    tblOff = (int)B(actor, 0x392) * 12 + (int)B(actor, 0x398) * 0x108;
    stat = (int)((W((unsigned char *)D_8010C330, tblOff) >> 11) & 0x7F) + 100;
    reachScaled = stat * 240;
    reachSq = (unsigned int)(((reachScaled / 180) * reachScaled) / 180);
    if (reachSq < distSq) {
        reachScaled = stat * 512;
        reachSq = (unsigned int)(((reachScaled / 180) * reachScaled) / 180);
        if (distSq >= reachSq) {
            goto set_8c_from_pos;
        }
        if ((unsigned int)((angleDiff - 0x21) & 0xFF) < 0x3FU) {
            B(actor, 0x3AC) = (unsigned char)(((unsigned int)(((int)angle1 - (int)angle2) & 0xFF) < 0x81U) ? 1 : 0);
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            B(actor, 0x396) = 0x9A;
            B(actor, 0x397) = 0;
            B(actor, 0x3AB) = (unsigned char)(B(ball, 0x3A4) + 0x80);
            return 1;
        }
    }

    tblOff = (int)B(actor, 0x392) * 12 + (int)B(actor, 0x398) * 0x108;
    stat = (int)((W((unsigned char *)D_8010C330, tblOff) >> 11) & 0x7F) + 100;
    reachScaled = stat * 512;
    reachSq = (unsigned int)(((reachScaled / 180) * reachScaled) / 180);
    if (distSq < reachSq) {
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        B(actor, 0x3AB) = (unsigned char)(B(ball, 0x3A4) + 0x80);
    } else {
set_8c_from_pos:
        B(actor, 0x3AB) = (unsigned char)func_8005B36C((int)S(actor, 2), (int)S(actor, 6), (int)S(Scratchpad, 0x48), (int)S(Scratchpad, 0x78));
    }
    B(actor, 0x396) = 0x8C;
    B(actor, 0x397) = 0;
    return 1;
}
