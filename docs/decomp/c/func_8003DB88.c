/*
 * func_8003DB88
 *
 * Purpose:
 *   Evaluates and triggers a ball-steal / challenge attempt (state 0x0C) for a
 *   defending player against the current ball owner. Selects either a
 *   standing/front block-steal (substate 0x20, anim 0x0B) or a side/pursuit
 *   challenge (substate 0x10, anim 0x18) based on ball height, relative
 *   owner/actor distances, movement directions, and body facing angles.
 *
 * Arguments:
 *   actor - pointer to 1000-byte defending player actor struct
 *
 * Return value:
 *   1 if a challenge/steal was initiated (actor state set to 0x0C), 0 otherwise
 *
 * Actor / global table fields read:
 *   actor+0x02/+0x06 (s16) x/z, actor+0x392 (u8) team slot (0..10),
 *   actor+0x398 (u8) side, actor+0x3A0 (u32) speed,
 *   actor+0x3A4 (u8) move heading, actor+0x3AA (u8) body heading,
 *   actor+0x3B4 (u8) max speed,
 *   owner+0x02/+0x06 (s16) x/z, owner+0x396 (u8) state, owner+0x3A0 (u32) speed,
 *   ball+0x02/+0x04/+0x06 (s16) xyz, ball+0x3A4 (u8) heading, ball+0x3A8 (u8) facing/spin angle,
 *   D_8010C330[side * 264 + slot * 12] (u32) player flags (bit 4 -> preferred foot),
 *   D_8010C328[side * 264 + slot * 12] (u32) player stats (top nibble >> 28),
 *   D_800C6D00[] (u8) stat-to-delay lookup table
 *
 * Actor fields written:
 *   actor+0x396 (u8) state = 0x0C, actor+0x397 (u8) substate = 0x20 or 0x10,
 *   actor+0x3A0 (u32) speed (clamped in substate 0x20 path),
 *   actor+0x3AA (u8) body heading, actor+0x3AB (u8) target heading,
 *   actor+0x3AC (u8) mirror/foot, actor+0x3C2 (u16) target actor id (ball owner),
 *   actor+0x3C4 (u16) = 1, actor+0x3C6 (u16) challenge speed,
 *   actor+0x3C8 (u16) = 0, actor+0x3CA (u16) challenge delay/duration,
 *   actor+0x3CC (u16) = 0, actor+0x3D1 (u8) = 0
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x036 / +0x066 (s16) ball predicted x/z at frame 0,
 *   +0x03C / +0x06C (s16) ball predicted x/z at frame 3,
 *   +0x03E / +0x06E (s16) ball predicted x/z at frame 4,
 *   +0x2A9 (u8) ball owner actor id,
 *   +0x2CF[side] (u8) possession flag (must be != 0 for defending team),
 *   +0x2F4 (u8*) pointer to ball actor
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   -0x74 = -116 (-0.464 m max ball height),
 *   0x4000 = dist^2 margin (128 raw = 0.512 m),
 *   0x40000 = max owner-to-ball dist^2 (512 raw = 2.048 m),
 *   0x8FFF = dist^2 threshold (192 raw = 0.768 m),
 *   0xC400 = dist^2 threshold (224 raw = 0.896 m),
 *   0xA0 = 160 (0.64 m base reach), 0x31 = 49 (~68.9 deg), 0x49 = 73 (~102.7 deg),
 *   0x60 = 96 (~135 deg), 0x7D = 125 (~175.8 deg)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800C6D00[];
extern unsigned char D_800FF748[];
extern unsigned char D_8010C328[];
extern unsigned char D_8010C330[];

extern unsigned int func_80052D90(unsigned char *owner);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, int step);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern unsigned int func_800B1128(unsigned int val);

int func_8003DB88(unsigned char *actor)
{
    unsigned int ownerIdx;
    unsigned char *ball;
    unsigned char *owner;
    unsigned int angleToBall;
    unsigned int actorDistSq;
    unsigned int ownerDistSq;
    unsigned int relBallAngle;
    unsigned int bodyDiff;
    unsigned int moveDiff;
    int reachBase;
    int slowFlag;
    unsigned int predReach;
    unsigned int predDist;
    unsigned int row;

    if (B(Scratchpad, 0x2CF + B(actor, 0x398)) == 0) {
        return 0;
    }

    ownerIdx = B(Scratchpad, 0x2A9);
    if (ownerIdx == 0) {
        return 0;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    if (S(ball, 0x04) < -0x74) {
        return 0;
    }

    owner = D_800FF748 + (ownerIdx % 24) * 1000;
    if ((func_80052D90(owner) & 0xFF) != 0) {
        return 0;
    }

    angleToBall = func_8005B36C(
        S(actor, 0x02), S(actor, 0x06),
        S(Scratchpad, 0x036), S(Scratchpad, 0x066));

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    {
        int dx = S(actor, 0x02) - S(ball, 0x02);
        int dz = S(actor, 0x06) - S(ball, 0x06);
        int odx = S(owner, 0x02) - S(ball, 0x02);
        int odz = S(owner, 0x06) - S(ball, 0x06);
        actorDistSq = (unsigned int)(dx * dx + dz * dz);
        ownerDistSq = (unsigned int)(odx * odx + odz * odz);
    }

    if ((actorDistSq + 0x4000) < ownerDistSq) {
        return 0;
    }

    relBallAngle = B(ball, 0x3A4) - angleToBall;
    if (((relBallAngle - 0x48) & 0xFF) >= 0x71 && W(owner, 0x3A0) >= 0x29) {
        unsigned int ballFacing = B(ball, 0x3A8);
        unsigned int diff = (ballFacing - angleToBall) & 0xFF;
        int inCone = (diff < 0x81)
            ? ((int)diff < 0x60)
            : ((int)((angleToBall - ballFacing) & 0xFF) < 0x60);

        if (inCone) {
            int dx;
            int dz;
            int distToOwnerSq;
            int gap;
            unsigned int bodyHeading;
            unsigned int bdiff;
            int facingOk;
            unsigned int angleToOwner;
            int spd;
            int maxSpd;

            if (ownerDistSq > 0x40000) {
                return 0;
            }

            dx = S(actor, 0x02) - S(owner, 0x02);
            dz = S(actor, 0x06) - S(owner, 0x06);
            distToOwnerSq = dx * dx + dz * dz;
            gap = (int)(W(actor, 0x3A0) - (W(owner, 0x3A0) - 0x90) + (relBallAngle & 0x7F));
            if ((unsigned int)(gap * gap) < (unsigned int)distToOwnerSq && distToOwnerSq > 0xC400) {
                return 0;
            }

            bodyHeading = B(actor, 0x3AA);
            bdiff = (angleToBall - bodyHeading) & 0xFF;
            facingOk = (bdiff < 0x81)
                ? ((int)bdiff < 0x49)
                : ((int)((bodyHeading - angleToBall) & 0xFF) < 0x49);
            if (!facingOk) {
                return 0;
            }

            B(actor, 0x3D1) = 0;
            H(actor, 0x3C4) = 1;
            H(actor, 0x3C8) = 0;
            B(actor, 0x396) = 0x0C;
            H(actor, 0x3C2) = B(Scratchpad, 0x2A9);

            angleToOwner = func_8005B36C(
                S(actor, 0x02), S(actor, 0x06),
                S(owner, 0x02), S(owner, 0x06));
            B(actor, 0x3AC) = (unsigned char)((((angleToBall - angleToOwner) & 0xFF) < 0x81) ^ 1);

            spd = SW(actor, 0x3A0);
            H(actor, 0x3C6) = (unsigned short)spd;
            maxSpd = (int)B(actor, 0x3B4) - 2;
            if ((signed short)spd > maxSpd) {
                H(actor, 0x3C6) = (unsigned short)maxSpd;
            }

            func_8005BA30(actor, 0x18, 0);
            B(actor, 0x397) = 0x10;
            H(actor, 0x3CC) = 0;
            return 1;
        }
    }

    {
        unsigned int bodyHeading = B(actor, 0x3AA);
        unsigned int diff = (angleToBall - bodyHeading) & 0xFF;
        bodyDiff = (diff < 0x81) ? diff : ((bodyHeading - angleToBall) & 0xFF);
    }
    if ((int)bodyDiff >= 0x31) {
        return 0;
    }

    {
        unsigned int moveHeading = B(actor, 0x3A4);
        unsigned int diff = moveHeading - angleToBall;
        moveDiff = ((diff & 0xFF) < 0x81) ? diff : (angleToBall - moveHeading);
    }

    if (W(actor, 0x3A0) < 0x31) {
        reachBase = 0x28;
        slowFlag = 1;
    } else {
        reachBase = 0x32;
        slowFlag = 0;
    }

    {
        unsigned int rearDelta = (B(actor, 0x3A4) - angleToBall) - 0x80;
        unsigned int rearMag = rearDelta & 0xFF;
        unsigned int reachBonus;

        if (rearMag >= 0x81) {
            rearMag = (0U - rearDelta) & 0xFF;
        }
        reachBonus = ((unsigned int)(reachBase * (int)rearMag)) >> 7;
        predReach = (reachBonus << 2) + 0xA0;
    }

    {
        int dx = S(actor, 0x02) - S(Scratchpad, 0x03C);
        int dz = S(actor, 0x06) - S(Scratchpad, 0x06C);
        predDist = func_800B1128((unsigned int)(dx * dx + dz * dz));
    }

    if (predDist >= predReach) {
        int dx = S(actor, 0x02) - S(owner, 0x02);
        int dz = S(actor, 0x06) - S(owner, 0x06);
        if (dx * dx + dz * dz > 0x8FFF) {
            return 0;
        }
    }

    B(actor, 0x3D1) = 0;
    H(actor, 0x3C4) = 1;
    H(actor, 0x3C8) = 0;
    H(actor, 0x3C2) = B(Scratchpad, 0x2A9);

    row = (unsigned int)B(actor, 0x392) * 12 + (unsigned int)B(actor, 0x398) * 264;
    B(actor, 0x3AC) = (unsigned char)((W(D_8010C330 + row, 0) >> 4) & 1);

    if ((int)(moveDiff & 0xFF) < 0x7D && B(owner, 0x396) != 0x0A) {
        B(actor, 0x3AB) = (unsigned char)func_8005B36C(
            S(actor, 0x02), S(actor, 0x06),
            S(Scratchpad, 0x03E), S(Scratchpad, 0x06E));
    } else {
        B(actor, 0x3AB) = (unsigned char)angleToBall;
    }

    func_8005BA30(actor, 0x0B, 0);

    {
        unsigned int spd = 0;
        if (predDist >= 0xA0) {
            spd = (predDist - 0xA0) >> 2;
            if (spd >= 0x33) {
                spd = 0x32;
            }
        }
        H(actor, 0x3C6) = (unsigned short)spd;
        W(actor, 0x3A0) = spd;
    }

    if (slowFlag == 0) {
        unsigned int statIdx;
        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0x0C);
        statIdx = W(D_8010C328 + row, 0) >> 28;
        H(actor, 0x3CA) = (unsigned short)(D_800C6D00[statIdx] - 4);
    } else {
        unsigned int statIdx;
        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 6);
        statIdx = W(D_8010C328 + row, 0) >> 28;
        H(actor, 0x3CA) = D_800C6D00[statIdx];
    }

    B(actor, 0x396) = 0x0C;
    H(actor, 0x3CC) = 0;
    B(actor, 0x397) = 0x20;
    return 1;
}
