/*
 * func_8004DA18
 *
 * Purpose:
 *   Resolves a player's challenge/tackle or ball-pickup interaction: maintains
 *   possession if already owner, contests/dispossesses the current owner (knocking
 *   the ball loose and potentially staggering the opponent), or claims a free ball
 *   while checking for a nearby opponent collision.
 *
 * Arguments:
 *   actor (u8*) - pointer to the 1000-byte player actor struct
 *
 * Return value:
 *   int - 1 if tackle/contest occurred without claiming the ball, 0 if ball claimed
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x2A8 (u8)  last-touch actor ID (W)
 *     +0x2A9 (u8)  ball owner ID (0 = free) (R/W)
 *     +0x2B0 (u32) ball event code (1 triggers possession callback) (R)
 *     +0x2CF (u8[2]) per-side possession flags (R)
 *     +0x2F4 (u8*) pointer to ball actor (R)
 *     +0x31C (s16) ball timer (cleared to 0 on challenge) (W)
 *     +0x322 (u16[2]) per-attack-direction marked/opponent actor ID (R)
 *     +0x332 (u8)  dribble/touch counter (incremented if owner, else cleared) (R/W)
 *   Actor / Owner / Opponent (D_800FF748, stride 0x3E8 = 1000 bytes):
 *     +0x002 (s16) x position (R)
 *     +0x006 (s16) z position (R)
 *     +0x38D (u8)  actor ID (R)
 *     +0x396 (u8)  state (set to 0xC, 0x10, or 0x18) (W)
 *     +0x397 (u8)  substate (R/W)
 *     +0x398 (u8)  side 0/1 (R)
 *     +0x399 (u8)  attack direction flag (R)
 *     +0x3A0 (u32) speed (R)
 *     +0x3A4 (u8)  move heading (R)
 *     +0x3AA (u8)  body heading (R)
 *     +0x3AC (u8)  mirror/foot flag (W)
 *     +0x3CC (u16) cleared to 0 on tackle animation (W)
 *     +0x3D0 (u8)  action lock flag (R)
 *     +0x3D1 (u8)  cleared to 0 (W)
 *   Ball (Scratchpad+0x2F4):
 *     +0x002 (s16) x position (R)
 *     +0x004 (s16) y height (negative = up) (R)
 *     +0x006 (s16) z position (R)
 *     +0x396 (u8)  ball state (W = 3 when claimed)
 *     +0x3A0 (u32) ball speed << 8 (R)
 *     +0x3A4 (u8)  ball heading (R)
 *   Globals:
 *     D_800FF748 (u8[]) actor array (24 x 1000 bytes) (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x2C..0x50 = 44..80 kick power clamp range on challenge
 *   -0x80      = -128 raw = 0.512 m ball height threshold (owner knock-down 0x18)
 *   -0x74      = -116 raw = 0.464 m ball height threshold (free ball challenge)
 *   0xC3FF     = 50175 (~224^2 raw = ~0.896 m squared distance)
 *   0xE0FF     = 57599 (~240^2 raw = ~0.960 m squared distance)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];

extern unsigned int func_8001E6B8(unsigned int);
extern void func_8004B0F0(unsigned char*, unsigned int, unsigned int, int);
extern unsigned int func_80052D90(unsigned char*);
extern unsigned int func_8005B36C(int, int, int, int);
extern void func_8005BA30(unsigned char*, unsigned int, unsigned int);
extern void func_80055EEC(unsigned char*);
extern void func_80056694(unsigned char*);
extern void func_800676C4(unsigned int, int, int);
extern void func_80056764(unsigned char*);

int func_8004DA18(unsigned char* actor) {
    unsigned char ownerId = B(Scratchpad, 0x2A9);
    unsigned char* actorsBase = D_800FF748;
    unsigned char* ball;
    unsigned char actorId;

    if (ownerId == B(actor, 0x38D)) {
        B(Scratchpad, 0x332)++;
        func_80056764(actor);
        goto claim_ball;
    }

    B(Scratchpad, 0x332) = 0;

    if (ownerId == 0) {
        unsigned char oppId;
        unsigned char* opp;
        unsigned int oppBody;
        unsigned int oppMove;
        unsigned int diff;
        int dx, dz;

        if (W(Scratchpad, 0x2B0) == 1) {
            if (B(Scratchpad, 0x2CF + B(actor, 0x398)) == 0) {
                func_80055EEC(actor);
            } else {
                func_80056694(actor);
            }
        }

        ball = (unsigned char*)W(Scratchpad, 0x2F4);
        func_800676C4(B(actor, 0x399) ^ 1, S(ball, 2), S(ball, 6));

        oppId = B(Scratchpad, 0x322 + ((B(actor, 0x399) ^ 1) << 1));
        opp = actorsBase + (oppId % 24) * 0x3E8;

        if ((func_80052D90(opp) & 0xFF) != 0) {
            goto claim_ball;
        }

        oppBody = B(opp, 0x3AA);
        oppMove = B(opp, 0x3A4);
        diff = (oppBody - oppMove) & 0xFF;
        if (diff < 0x81) {
            if (diff < 0x40) {
                goto claim_ball;
            }
        } else if (((oppMove - oppBody) & 0xFF) < 0x40) {
            goto claim_ball;
        }

        ball = (unsigned char*)W(Scratchpad, 0x2F4);
        dx = S(opp, 2) - S(ball, 2);
        dz = S(opp, 6) - S(ball, 6);
        if (dx * dx + dz * dz > 0xC3FF) {
            goto claim_ball;
        }
        if (S(ball, 4) < -0x74) {
            goto claim_ball;
        }
        if (func_8001E6B8(4) != 0) {
            goto claim_ball;
        }

        B(opp, 0x3D1) = 0;
        B(opp, 0x396) = 0x10;
        B(opp, 0x397) = 0;
        goto claim_ball;
    } else {
        unsigned char* owner = actorsBase + (ownerId % 24) * 0x3E8;
        int power;
        int dx1, dz1, distSq1;
        int dx2, dz2, distSq2;
        unsigned int actorMove;
        unsigned int ballMove;
        unsigned int diff;
        unsigned int ang;

        if (B(owner, 0x397) == 4 && B(owner, 0x3D0) != 0) {
            return 1;
        }

        power = (int)W(actor, 0x3A0) + (int)(W(W(Scratchpad, 0x2F4), 0x3A0) >> 8) + (int)func_8001E6B8(8);
        if (power >= 0x51) {
            power = 0x50;
        } else if (power < 0x2C) {
            power = 0x2C;
        }

        func_8004B0F0(actor, B(actor, 0x3AA), (unsigned int)power, 1);
        S(Scratchpad, 0x31C) = 0;

        if ((func_80052D90(owner) & 0xFF) != 0) {
            goto claim_ball;
        }

        B(owner, 0x3D1) = 0;
        dx1 = S(actor, 2) - S(owner, 2);
        dz1 = S(actor, 6) - S(owner, 6);
        distSq1 = dx1 * dx1 + dz1 * dz1;

        ball = (unsigned char*)W(Scratchpad, 0x2F4);
        if (S(ball, 4) < -0x80) {
            B(owner, 0x396) = 0x18;
            B(owner, 0x397) = 0;
            goto claim_ball;
        }

        dx2 = S(owner, 2) - S(ball, 2);
        dz2 = S(owner, 6) - S(ball, 6);
        distSq2 = dx2 * dx2 + dz2 * dz2;
        if (distSq2 > 0xC3FF && distSq1 > 0xE0FF) {
            B(owner, 0x396) = 0x18;
            B(owner, 0x397) = 0;
            goto claim_ball;
        }

        B(owner, 0x396) = 0x10;
        B(owner, 0x397) = 0;

        actorMove = B(actor, 0x3A4);
        ballMove = B(W(Scratchpad, 0x2F4), 0x3A4);
        diff = (ballMove - actorMove) & 0xFF;
        if ((diff < 0x81) ? (diff < 0x28) : (((actorMove - ballMove) & 0xFF) < 0x28)) {
            ang = func_8005B36C(S(actor, 2), S(actor, 6), S(owner, 2), S(owner, 6));
            if (((ang - B(actor, 0x3A4)) & 0xFF) < 0x81) {
                ang = func_8005B36C(S(actor, 2), S(actor, 6), S(owner, 2), S(owner, 6));
                if (((ang - B(actor, 0x3A4)) & 0xFF) >= 0x61) {
                    return 1;
                }
            } else {
                ang = func_8005B36C(S(actor, 2), S(actor, 6), S(owner, 2), S(owner, 6));
                if (((B(actor, 0x3A4) - ang) & 0xFF) >= 0x61) {
                    return 1;
                }
            }
        }

        ball = (unsigned char*)W(Scratchpad, 0x2F4);
        ang = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
        B(actor, 0x3AC) = (((ang - B(actor, 0x3AA)) & 0xFF) < 0x81) ? 1 : 0;
        func_8005BA30(actor, 0x19, 8);
        B(actor, 0x396) = 0xC;
        B(actor, 0x397) = 0;
        H(actor, 0x3CC) = 0;
        return 1;
    }

claim_ball:
    ball = (unsigned char*)W(Scratchpad, 0x2F4);
    B(ball, 0x396) = 3;
    actorId = B(actor, 0x38D);
    B(Scratchpad, 0x2A9) = actorId;
    B(Scratchpad, 0x2A8) = actorId;
    return 0;
}
