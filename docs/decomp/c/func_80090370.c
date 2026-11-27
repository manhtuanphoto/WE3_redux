/*
 * func_80090370
 *
 * Purpose:
 *   Top-level goalkeeper/defender ball-interception dispatcher for a predicted
 *   ball trajectory point (Scratchpad +0x48/+0x60/+0x78 and next step +0x4A/+0x62/+0x7A).
 *   Checks penalty-box bounds, height ceiling, touch cooldown/ball approach angle,
 *   and whether the ball is moving away from the actor, then tries interception
 *   handlers func_80090EE0, func_80090968, and func_80091518 in order.
 *
 * Arguments:
 *   actor (u8 *) : pointer to 1000-byte actor struct
 *
 * Return value:
 *   1 if any interception handler (func_80090EE0, func_80090968, func_80091518) succeeded;
 *   0 otherwise.
 *
 * Actor fields read:
 *   +0x02 (s16) x position, +0x06 (s16) z position,
 *   +0x392 (u8) team slot (0..10), +0x398 (u8) side (0/1),
 *   +0x399 (u8) attack direction flag, +0x3B9 (u8) stat*10
 *
 * Actor fields written:
 *   None directly (delegated to func_80090EE0 / func_80090968 / func_80091518)
 *
 * Global / Scratchpad (0x1F800000) fields read:
 *   Scratchpad +0x36/+0x66 (s16) earlier predicted ball x/z,
 *   Scratchpad +0x48/+0x60/+0x78 (s16) predicted ball x/y/z,
 *   Scratchpad +0x4A/+0x62/+0x7A (s16) next predicted ball x/y/z,
 *   Scratchpad +0x2F4 (u8 *) pointer to ball actor (reads +0x3A4 move heading),
 *   Scratchpad +0x31C (s16) ball timer (frames since last touch),
 *   Scratchpad +0x31E[2] (u8, stride 2) per-side opponent actor index,
 *   D_800FF748 (1000-byte actor array, indexed by (id % 24) * 1000),
 *   D_8010C330 / D_8010C32F (player stat table, stride 0x108 per side, 12 per slot)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   Box X bounds: 0x2422 <= |x| <= 0x333F (9250..13119 raw = 37.000..52.476 m);
 *   Box Z bound: |z| < 0x12F5 (4853 raw = 19.412 m).
 *   Max height ceiling: -(stat * 864) / 180 (-1.92..-4.36 m, negative = up).
 *   Cooldown: 19 - stat_3B9 / 10 frames (at 30 Hz).
 *   0x32E0 = 13024 raw = 52.096 m; 0x40000 raw^2 = (2.048 m)^2; 0x9000 raw^2 = (0.768 m)^2.
 *   Angular windows: 0x25 (37/256 = ~52 deg), 0x38 (56/256 = ~78.75 deg).
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010C32F[];
extern unsigned int D_8010C330[];

extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_80090EE0(unsigned char *actor, int distSq, int otherDistSq, unsigned int angle1, unsigned int angle2);
extern unsigned int func_80090968(unsigned char *actor, int distSq, unsigned int angle1, unsigned int angle2);
extern unsigned int func_80091518(unsigned char *actor, int distSq, unsigned int angle1, unsigned int angle2);

int func_80090370(unsigned char *actor)
{
    signed short x48;
    signed short z78;
    int absVal;
    int tblOff;
    int stat;
    int minY;
    int actorX;
    int actorZ;
    int dx;
    int dz;
    int distSq;
    int otherDistSq;
    int nextDistSq;
    signed short x4A;
    signed short z7A;
    unsigned char otherId;
    unsigned char *otherActor;
    int cooldown;
    int cond;
    unsigned int ang0;
    unsigned int ang1;

    x48 = S(Scratchpad, 0x48);
    z78 = S(Scratchpad, 0x78);

    absVal = (x48 >= 0) ? (int)x48 : -(int)x48;
    if ((unsigned int)(absVal - 0x2422) >= 0xF1EU) {
        return 0;
    }

    absVal = (z78 >= 0) ? (int)z78 : -(int)z78;
    if (absVal >= 0x12F5) {
        return 0;
    }

    tblOff = (int)B(actor, 0x392) * 12 + (int)B(actor, 0x398) * 0x108;
    stat = (int)((W((unsigned char *)D_8010C330, tblOff) >> 11) & 0x7F) + 100;
    minY = -(stat * 864) / 180;
    if ((int)S(Scratchpad, 0x60) < minY) {
        return 0;
    }

    actorX = (int)S(actor, 2);
    actorZ = (int)S(actor, 6);
    dx = actorX - (int)x48;
    dz = actorZ - (int)z78;
    distSq = dx * dx + dz * dz;

    otherId = B(Scratchpad + ((B(actor, 0x399) ^ 1U) << 1), 0x31E);
    otherActor = D_800FF748 + (int)((otherId % 24U) & 0xFFU) * 1000;
    dx = (int)S(otherActor, 2) - (int)x48;
    dz = (int)S(otherActor, 6) - (int)z78;
    otherDistSq = dx * dx + dz * dz;

    x4A = S(Scratchpad, 0x4A);
    z7A = S(Scratchpad, 0x7A);
    dx = actorX - (int)x4A;
    dz = actorZ - (int)z7A;
    nextDistSq = dx * dx + dz * dz;

    cooldown = 19 - (int)((B(actor, 0x3B9) / 10U) & 0xFF);
    if ((int)S(Scratchpad, 0x31C) < cooldown) {
        absVal = ((int)x4A >= 0) ? (int)x4A : -(int)x4A;
        if (absVal < 0x32E0) {
            unsigned int angA = func_8005B36C(actorX, actorZ, (int)x4A, (int)z7A);
            unsigned char ballDir = B((unsigned char *)W(Scratchpad, 0x2F4), 0x3A4);
            unsigned int angB;
            int diff;
            if (((ballDir - angA) & 0xFFU) >= 0x81U) {
                angB = func_8005B36C((int)S(actor, 2), (int)S(actor, 6), (int)x4A, (int)z7A);
                ballDir = B((unsigned char *)W(Scratchpad, 0x2F4), 0x3A4);
                diff = (int)((angB - ballDir) & 0xFFU);
            } else {
                angB = func_8005B36C((int)S(actor, 2), (int)S(actor, 6), (int)x4A, (int)z7A);
                ballDir = B((unsigned char *)W(Scratchpad, 0x2F4), 0x3A4);
                diff = (int)((ballDir - angB) & 0xFFU);
            }
            if (diff < 0x25) {
                cond = (nextDistSq < distSq);
                goto check_away;
            } else {
                int reachScaled;
                int reachSq;
                tblOff = (int)B(actor, 0x392) * 12 + (int)B(actor, 0x398) * 0x108;
                stat = (int)((W((unsigned char *)D_8010C330, tblOff) >> 11) & 0x7F) + 100;
                reachScaled = stat * 1280;
                reachSq = ((reachScaled / 180) * reachScaled) / 180;
                if (nextDistSq < reachSq) {
                    return 0;
                }
            }
        }
    }

    cond = (nextDistSq < distSq);
check_away:
    if (cond && (distSq + 0x40000) < otherDistSq && 0x9000 < distSq) {
        unsigned int angNext = func_8005B36C((int)S(actor, 2), (int)S(actor, 6), (int)x4A, (int)z7A);
        int maxY;
        tblOff = (int)B(actor, 0x392) * 12 + (int)B(actor, 0x398) * 0x108;
        stat = (int)((W((unsigned char *)D_8010C330, tblOff) >> 11) & 0x7F) + 100;
        maxY = -(stat * 736) / 180 + 6 - (int)(B(D_8010C32F, tblOff) & 0xF);
        if (maxY < (int)S(Scratchpad, 0x62)) {
            int absNextX = ((int)x4A >= 0) ? (int)x4A : -(int)x4A;
            if (absNextX < 0x32E0) {
                int actorXOff = (int)S(actor, 2) + 0x80;
                int absActorX = (actorXOff >= 0) ? actorXOff : -actorXOff;
                if (absNextX < absActorX) {
                    unsigned char ballDir = B((unsigned char *)W(Scratchpad, 0x2F4), 0x3A4);
                    unsigned int diff1 = (angNext - (unsigned int)ballDir) & 0xFFU;
                    if (diff1 < 0x81U) {
                        if (diff1 >= 0x38U) {
                            return 0;
                        }
                    } else {
                        unsigned int diff2 = ((unsigned int)ballDir - angNext) & 0xFFU;
                        if (diff2 >= 0x38U) {
                            return 0;
                        }
                    }
                }
            }
        }
    }

    ang0 = func_8005B36C((int)S(actor, 2), (int)S(actor, 6), (int)S(Scratchpad, 0x36), (int)S(Scratchpad, 0x66)) & 0xFFU;
    ang1 = func_8005B36C((int)S(actor, 2), (int)S(actor, 6), (int)x48, (int)z78) & 0xFFU;

    if (func_80090EE0(actor, distSq, otherDistSq, ang1, ang0) & 0xFFU) {
        return 1;
    }
    if (func_80090968(actor, distSq, ang1, ang0) & 0xFFU) {
        return 1;
    }
    if (func_80091518(actor, distSq, ang1, ang0) & 0xFFU) {
        return 1;
    }
    return 0;
}
