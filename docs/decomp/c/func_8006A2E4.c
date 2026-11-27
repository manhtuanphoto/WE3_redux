/*
 * func_8006A2E4 - Attacking/off-ball AI target-coordinate shaping for a player actor.
 *
 * Purpose:
 *   Adjusts the scratchpad target coordinates (0x1F8000F8 X, 0x1F8000FC Z) for an
 *   off-ball AI player based on team tactics/mentality, pitch zone (0x1F80030B),
 *   player role (actor+0x391, 11..12 = forward/striker), offside-line/anchor X
 *   (0x1F800314), ball Z, teammate/opponent proximity, and tactical state flags.
 *
 * Arguments:
 *   actor (unsigned char*): player actor struct (1000 bytes).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F800009 (u8, R): flag enabling initial tactical X/Z pull toward anchor/ball.
 *   - 0x1F8000F8 / 0x1F8000FC (s16, R/W): target X / Z coordinates.
 *   - 0x1F8002F4 (u8*, R): pointer to ball actor.
 *   - 0x1F8002FD[2] (u8, R): per-side tactical/formation mode.
 *   - 0x1F8002FF[2] (u8, R): per-side attacking mentality (0..2).
 *   - 0x1F80030B (u8, R): pitch/attacking zone mode (0..5).
 *   - 0x1F800314[2] (s32, R): per-attack-dir tactical X anchor / offside line.
 *   - 0x1F80031E[2] (u8[2][2], R): nearest-to-ball actor ID per attack direction.
 *   - 0x1F800328 (u8, R): difficulty / level index.
 *
 * Actor fields reads/writes:
 *   - actor: +0x06 (s16 z, R), +0x391 (u8 tactical role, R; 0xB..0xC = forward, >=0xE = wide/support),
 *            +0x398 (u8 side, R), +0x399 (u8 attack dir, R; 0 -> +1, 1 -> -1).
 *   - ball:  +0x02/+0x06 (s16 x/z, R).
 *   - teammate: +0x06 (s16 z, R), +0x391 (u8 role, R).
 *
 * Globals reads/writes:
 *   - D_800FF748 (actor array base, R).
 *   - D_8010C1D8 (+0x0D[2] u8, R; +0x3A u16, R): match/tactical state flags.
 *
 * Magic numbers (250 raw = 1 m):
 *   - 0x12F4 (4852) = 19.41 m Z (penalty area width).
 *   - 0xBC0 / 0xBC1 (3008 / 3009) = 12.03 m X (ball advancement threshold).
 *   - 0x600 (1536) = 6.14 m Z spacing check against teammate.
 *   - 0x28F5 (10485) = 41.94 m X clamp limit (touchline/attacking depth cap).
 *   - 0x80 (128 = 1 << 7) = 0.51 m X offset ahead of anchor.
 *   - 0x200 / 0x400 / 0xC00 (512 / 1024 / 3072) = 2.05 / 4.10 / 12.29 m push radius in func_800A1484.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];
extern unsigned char D_8010C1D8[];

extern int func_8005A7E4(int cur, int target, int step);
extern int func_8005A76C(int cur, int target, int div_or_step);
extern void func_800A1484(unsigned char *actor, unsigned char *px, unsigned char *pz, int radius);

void func_8006A2E4(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *ball;
    unsigned char *teammate;
    int atk_dir;
    int dir_sign;
    int is_fwd;
    int step;
    int mode;
    int zone_mode;
    int ball_z, ball_x;
    int val;
    int idx, dz, x;

    atk_dir = B(actor, 0x399);
    dir_sign = (atk_dir == 0) ? 1 : -1;

    if (B(scratch, 9) != 0) {
        mode = B(scratch, 0x2FF + B(actor, 0x398));
        is_fwd = ((unsigned int)(B(actor, 0x391) - 0xB) < 2U);

        if (mode == 0) {
            step = is_fwd ? 0x23 : 0x14;
            S(scratch, 0xF8) = (signed short)func_8005A7E4(S(scratch, 0xF8), SW(scratch, 0x314 + (atk_dir << 2)), step);
            ball = (unsigned char *)W(scratch, 0x2F4);
            S(scratch, 0xFC) = (signed short)func_8005A76C(S(scratch, 0xFC), S(ball, 6), 2);
        } else if (mode == 1) {
            step = is_fwd ? 0x19 : 0xA;
            S(scratch, 0xF8) = (signed short)func_8005A7E4(S(scratch, 0xF8), SW(scratch, 0x314 + (atk_dir << 2)), step);
            ball = (unsigned char *)W(scratch, 0x2F4);
            S(scratch, 0xFC) = (signed short)func_8005A76C(S(scratch, 0xFC), S(ball, 6), 1);
        } else if (mode == 2) {
            if (is_fwd) {
                S(scratch, 0xF8) = (signed short)func_8005A7E4(S(scratch, 0xF8), SW(scratch, 0x314 + (atk_dir << 2)), 0xF);
            }
        }
    }

    zone_mode = B(scratch, 0x30B);
    is_fwd = ((unsigned int)(B(actor, 0x391) - 0xB) < 2U);

    switch (zone_mode) {
    case 0:
        ball = (unsigned char *)W(scratch, 0x2F4);
        S(scratch, 0xFC) = (signed short)func_8005A76C(S(scratch, 0xFC), S(ball, 6), 1);
        if (is_fwd) {
            int form = B(scratch, 0x2FD + B(actor, 0x398));
            if (form != 10 && (form < 14 || form >= 16)) {
                S(scratch, 0xF8) = (signed short)func_8005A76C(
                    S(scratch, 0xF8),
                    SW(scratch, 0x314 + (B(actor, 0x399) << 2)),
                    1
                );
            }
        }
        break;

    case 1:
        if (!is_fwd) {
            ball = (unsigned char *)W(scratch, 0x2F4);
            S(scratch, 0xFC) = (signed short)func_8005A76C(S(scratch, 0xFC), S(ball, 6), 1);
        } else {
            int form = B(scratch, 0x2FD + B(actor, 0x398));
            step = (form <= 10 || (form >= 14 && form < 16)) ? 1 : 2;
            ball = (unsigned char *)W(scratch, 0x2F4);
            S(scratch, 0xFC) = (signed short)func_8005A76C(S(scratch, 0xFC), S(ball, 6), step);
            S(scratch, 0xF8) = (signed short)func_8005A76C(
                S(scratch, 0xF8),
                SW(scratch, 0x314 + (B(actor, 0x399) << 2)),
                3
            );
        }
        break;

    case 2:
        if (!is_fwd) {
            ball = (unsigned char *)W(scratch, 0x2F4);
            S(scratch, 0xFC) = (signed short)func_8005A76C(S(scratch, 0xFC), S(ball, 6), 2);
        } else {
            int form = B(scratch, 0x2FD + B(actor, 0x398));
            if (form == 10 || (form >= 14 && form < 16)) {
                ball = (unsigned char *)W(scratch, 0x2F4);
                S(scratch, 0xFC) = (signed short)func_8005A76C(S(scratch, 0xFC), S(ball, 6), 2);
            } else {
                step = (form < 10 || form >= 16) ? 0x19 : 2;
                ball = (unsigned char *)W(scratch, 0x2F4);
                val = (signed short)func_8005A7E4(S(scratch, 0xFC), S(ball, 6), step);
                S(scratch, 0xFC) = (signed short)((val * 5) / 6);
            }
            S(scratch, 0xF8) = (signed short)func_8005A76C(
                S(scratch, 0xF8),
                SW(scratch, 0x314 + (B(actor, 0x399) << 2)),
                4
            );
        }
        break;

    case 3:
        if (!is_fwd) {
            ball = (unsigned char *)W(scratch, 0x2F4);
            ball_z = S(ball, 6);
            if (ball_z < 0) ball_z = -ball_z;
            step = (ball_z < 0x12F4) ? 3 : 2;
            S(scratch, 0xFC) = (signed short)func_8005A76C(S(scratch, 0xFC), S(ball, 6), step);
            func_800A1484(
                actor,
                scratch + 0xF8,
                scratch + 0xFC,
                (3 - B(scratch, 0x2FF + B(actor, 0x398))) << 9
            );
        } else {
            int form = B(scratch, 0x2FD + B(actor, 0x398));
            if (form == 10 || (form >= 14 && form < 16)) {
                ball = (unsigned char *)W(scratch, 0x2F4);
                S(scratch, 0xFC) = (signed short)func_8005A76C(S(scratch, 0xFC), S(ball, 6), 2);
            } else {
                ball = (unsigned char *)W(scratch, 0x2F4);
                ball_z = S(ball, 6);
                if (ball_z < 0) ball_z = -ball_z;
                if (ball_z < 0x12F4) {
                    S(scratch, 0xFC) = (signed short)func_8005A76C(S(scratch, 0xFC), S(ball, 6), 3);
                } else {
                    S(scratch, 0xFC) = (signed short)func_8005A7E4(S(scratch, 0xFC), S(ball, 6), 0x19);
                }
                S(scratch, 0xFC) = (signed short)((S(scratch, 0xFC) * 2) / 3);
            }
            S(scratch, 0xF8) = (signed short)func_8005A76C(
                S(scratch, 0xF8),
                SW(scratch, 0x314 + (B(actor, 0x399) << 2)),
                5
            );
        }
        break;

    case 4:
    case 5:
        step = (B(scratch, 0x328) < 2) ? 2 : 3;
        ball = (unsigned char *)W(scratch, 0x2F4);
        S(scratch, 0xFC) = (signed short)func_8005A76C(S(scratch, 0xFC), S(ball, 6), step);
        if (is_fwd) {
            S(scratch, 0xF8) = (signed short)func_8005A76C(
                S(scratch, 0xF8),
                SW(scratch, 0x314 + (B(actor, 0x399) << 2)),
                5
            );
            func_800A1484(actor, scratch + 0xF8, scratch + 0xFC, 0xC00);
        } else {
            func_800A1484(
                actor,
                scratch + 0xF8,
                scratch + 0xFC,
                (3 - B(scratch, 0x2FF + B(actor, 0x398))) << 10
            );
        }
        break;
    }

    mode = B(scratch, 0x2FD + B(actor, 0x398));
    if (mode == 0 || mode == 4) {
        atk_dir = B(actor, 0x399);
        idx = B(scratch, 0x31E + (atk_dir << 1));
        teammate = D_800FF748 + (idx % 24) * 1000;
        if (B(teammate, 0x391) >= 0xE) {
            dz = S(teammate, 6) - S(actor, 6);
            if (dz < 0) dz = -dz;
            if (dz < 0x600) {
                x = (signed short)SW(scratch, 0x314 + (atk_dir << 2));
                S(scratch, 0xF8) = (signed short)x;
                if ((x < 0 ? -x : x) >= 0x28F6) {
                    S(scratch, 0xF8) = (x >= 0) ? 0x28F5 : -0x28F5;
                }
            }
        }
    }

    if ((unsigned int)(B(actor, 0x391) - 0xB) < 2U) {
        atk_dir = B(actor, 0x399);
        ball = (unsigned char *)W(scratch, 0x2F4);
        ball_x = S(ball, 2);
        if ((atk_dir == 0 && ball_x < -0xBC0) || (atk_dir == 1 && ball_x >= 0xBC1)) {
            ball_z = S(ball, 6);
            if (ball_z < 0) ball_z = -ball_z;
            if (ball_z < 0x12F4) {
                idx = B(scratch, 0x31E + (atk_dir << 1));
                teammate = D_800FF748 + (idx % 24) * 1000;
                if (B(teammate, 0x391) >= 0xE) {
                    dz = S(teammate, 6) - S(actor, 6);
                    if (dz < 0) dz = -dz;
                    if (dz < 0x600) {
                        x = (signed short)(SW(scratch, 0x314 + (atk_dir << 2)) - (dir_sign << 7));
                        S(scratch, 0xF8) = (signed short)x;
                        if ((x < 0 ? -x : x) >= 0x28F6) {
                            S(scratch, 0xF8) = (x >= 0) ? 0x28F5 : -0x28F5;
                        }
                    }
                }
            }
        }
    }

    if (B(D_8010C1D8, 0xD + B(actor, 0x398)) == 4) {
        atk_dir = B(actor, 0x399);
        ball = (unsigned char *)W(scratch, 0x2F4);
        ball_x = S(ball, 2);
        if ((atk_dir == 0 && ball_x > 0) || (atk_dir == 1 && ball_x < 0)) {
            S(scratch, 0xF8) = (signed short)func_8005A76C(
                S(scratch, 0xF8),
                SW(scratch, 0x314 + (atk_dir << 2)),
                5
            );
        }
    }

    if (H(D_8010C1D8, 0x3A) != 0) {
        S(scratch, 0xF8) = (signed short)func_8005A76C(
            S(scratch, 0xF8),
            SW(scratch, 0x314 + (B(actor, 0x399) << 2)),
            7
        );
        ball = (unsigned char *)W(scratch, 0x2F4);
        S(scratch, 0xFC) = (signed short)func_8005A76C(S(scratch, 0xFC), S(ball, 6), 3);
    }
}
