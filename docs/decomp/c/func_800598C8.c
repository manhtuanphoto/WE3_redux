/*
 * func_800598C8
 *
 * Purpose:
 *   Triggers kick/shot audio and shot commentary SFX for an attacking actor.
 *   Plays SFX 0x10 unconditionally, checks if D_8010A394 == 0 and the ball is
 *   beyond X = 0xB00 in the attacking half, calls func_80057AE4, and selects
 *   crowd/commentary SFX IDs (0xDDF..0xDF8) based on squared distance to the
 *   goal (+/-0x3320, 0), shot angle (ball+0x3A8), defender count inside
 *   |x|<0x23E2 && |z|<0x12F4, and player attribute nibble (D_8010C32C & 0xF).
 *   Sets D_800FF748+0xAC4C = 1 when shot commentary triggers.
 *
 * Arguments:
 *   actor: (unsigned char*) pointer to shooting player entity struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x2F4 (ptr) ball entity (+0x02 s16 x, +0x3A8 u8 shot/heading angle) (R)
 *   D_8010A394 (u16): flag suppressing shot commentary when != 0 (R)
 *   D_8010C32C: player attributes table (stride 0x100 per team, 0x0C per slot; low nibble & 0xF) (R)
 *   D_800C7D9A[]: u16 SFX ID table (R)
 *   D_800FF748: player array base (stride 0x3E8); +0xAC4C (s16) set to 1 (W)
 *
 * Magic numbers:
 *   0x0B00 (2816): minimum ball depth in attacking half
 *   0x3320 (13088): goal line X coordinate
 *   0x4000000 (8192^2), 0x23FFFFF: squared distance thresholds to goal
 *   0x23E2 (9186), 0x12F4 (4852): field bounds for counting players
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define P(p,o)  (*(unsigned char**)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned short D_8010A394;
extern unsigned char  D_800FF748[];
extern unsigned int   D_8010C32C[];
extern unsigned short D_800C7D9A[];

extern void func_800AB620(unsigned int sfx);
extern void func_80057AE4(unsigned char *actor, int arg1, int arg2);
extern int  func_8001E6B8(int n);
extern unsigned int func_8005CE14(unsigned int player_id);

void func_800598C8(unsigned char *actor) {
    unsigned char *ball;
    signed short bx_signed;
    int dx;
    int az;
    unsigned int dist_sq;
    unsigned char atk_ang;
    unsigned char ball_ang;
    unsigned int ang_diff;

    func_800AB620(0x10);

    if (D_8010A394 != 0) {
        return;
    }

    ball = P(Scratchpad, 0x2F4);
    bx_signed = S(ball, 0x02);
    if (B(actor, 0x399) != 0) {
        bx_signed = (signed short)(-bx_signed);
    }
    if (bx_signed < 0xB00) {
        return;
    }

    func_80057AE4(actor, 0, 0);

    if (S(actor, 0x3C8) != 0) {
        func_800AB620((unsigned int)(func_8001E6B8(2) + 0xDF5));
        return;
    }

    if (B(actor, 0x399) != 0) {
        dx = S(actor, 0x02) + 0x3320;
    } else {
        dx = S(actor, 0x02) - 0x3320;
    }
    az = S(actor, 0x06);
    dist_sq = (unsigned int)((dx * dx) + (az * az));

    ball = P(Scratchpad, 0x2F4);
    atk_ang = (unsigned char)(B(actor, 0x399) << 7);
    ball_ang = B(ball, 0x3A8);
    if ((unsigned char)(atk_ang - ball_ang) < 0x81) {
        ang_diff = (unsigned int)(atk_ang - ball_ang);
    } else {
        ang_diff = (unsigned int)(ball_ang - atk_ang);
    }

    if (dist_sq > 0x4000000U) {
        unsigned int base_slot = func_8005CE14(B(actor, 0x38D)) & 0xFF;
        unsigned char *p = D_800FF748 + ((base_slot % 24U) * 0x3E8);
        unsigned int count = 0;
        int i;

        for (i = 0; i < 10; i++) {
            signed short px = S(p, 0x02);
            signed short pz = S(p, 0x06);
            if (((px >= 0) ? px : -px) < 0x23E2 &&
                ((pz >= 0) ? pz : -pz) < 0x12F4) {
                count++;
            }
            p += 0x3E8;
        }

        if (func_8001E6B8(2) != 0 && (count & 0xFF) != 0 && (count & 0xFF) < 4) {
            if (func_8001E6B8(3) == 0) {
                func_800AB620(0xDE3);
            } else {
                /* Note: asm uses loop counter ($s0 == 10 at loop exit) as index */
                func_800AB620(D_800C7D9A[10]);
            }
        } else {
            unsigned int stat = D_8010C32C[(B(actor, 0x398) * 0x100 + B(actor, 0x392) * 0x0C) >> 2] & 0xF;
            if (stat < 8) {
                func_800AB620((unsigned int)(func_8001E6B8(2) + 0xDDF));
            } else {
                func_800AB620((unsigned int)(func_8001E6B8(4) + 0xDE6));
            }
        }
    } else {
        unsigned int stat;
        int r = func_8001E6B8(2);

        if (r != 0) {
            stat = D_8010C32C[(B(actor, 0x398) * 0x100 + B(actor, 0x392) * 0x0C) >> 2] & 0xF;
            if (stat >= 8) {
                func_800AB620((unsigned int)(func_8001E6B8(2) + 0xDF7));
                return;
            }
        }

        if (func_8001E6B8(2) != 0 && (ang_diff & 0xFF) < 0x20 && dist_sq <= 0x23FFFFFU) {
            func_800AB620(0xDEF);
            return;
        }

        {
            unsigned char sfx_roll = (unsigned char)(func_8001E6B8(5) & 0xFF);
            if (sfx_roll != 1) {
                func_800AB620((unsigned int)(sfx_roll + 0xDF0));
            }
        }
    }

    S(D_800FF748, 0xAC4C) = 1;
}
