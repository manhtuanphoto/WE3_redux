/*
 * func_8005896C - Goal-scored commentary, crowd reaction, and hat-trick handler.
 *
 * Purpose:
 *   Triggers match commentary and crowd/excitement audio/state calls when a player
 *   scores, handling own-goal/special mode (D_8010A323 != 0), penalty/shootout or
 *   specific match sub-phases, hat-trick detection (D_800E6C68 == 3), and score-
 *   differential-based equalizer/lead/comeback commentary and crowd updates.
 *
 * Arguments:
 *   actor (unsigned char*): scoring player actor struct (1000 bytes).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F8002CE (u8, R): special mode / replay flag.
 *   - 0x1F8002D1[2] (u8, R): team scores (side 0 / side 1).
 *   - 0x1F8002DD[2] (u8, R): per-side crowd/commentary channel index passed to func_8005A1F0.
 *   - 0x1F8002DF (u8, R): match period / half index (0..3).
 *   - 0x1F8002E1 (u8, R): match mode / ruleset byte.
 *   - 0x1F8002E2 (u8, R): match phase / condition byte (checked == 1 or == 4).
 *
 * Actor / global fields reads/writes:
 *   - actor: +0x392 (u8 team slot, R), +0x398 (u8 side, R).
 *   - D_800FF748: +0x88FD + side*96 + i*3 (u8, R: goal-scorer team slots, 16 entries/side),
 *                 +0xAC70 + side (u8, R/W: per-side special goal counter),
 *                 +0xAC72 (u8, W: last scoring/conceding side flag).
 *   - D_8010A323 (u8, R), D_8010A397 (u8, R), D_8010A3A9 (u8, R/W),
 *     D_800E6C68 (u8, R/W: scorer goal count), D_8010A5A8+0xE (u8, R),
 *     D_800C7D6C[] (u8, R: 7-entry commentary index lookup table).
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 3600 (0xE10 frames = 120 s at 30 Hz; game-clock units returned by func_8004F3CC)
 *     used as divisor for late-half / late-extra-time checks (>= 0x28 = 40, >= 0xB = 11).
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];
extern unsigned char D_8010A5A8[];
extern unsigned char D_8010A323;
extern unsigned char D_8010A397;
extern unsigned char D_8010A3A9;
extern unsigned char D_800E6C68;
extern unsigned char D_800C7D6C[];

extern void func_800AB620(int id);
extern int func_8001E6B8(int n);
extern void func_80057AE4(unsigned char *actor, int a1, int a2);
extern int func_80087BF8(void);
extern unsigned int func_8004F3CC(void);
extern void func_8005A1F0(int a0, int a1);
extern void func_80058FCC(unsigned char *actor, int a1);

void func_8005896C(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *base;
    unsigned char *table;
    int s0;
    int side;
    int count;
    int i;
    int period;
    int mode;
    int score_diff;
    int flag_side;
    int phase_byte;

    func_800AB620(0x10);
    base = D_800FF748;
    table = D_8010A5A8;

    if (D_8010A323 != 0) {
        side = B(actor, 0x398);
        count = B(base, 0xAC70 + side);
        if (count == 0) {
            func_800AB620(func_8001E6B8(3) + 0xD58);
            s0 = func_8001E6B8(3);
            if ((unsigned int)(B(scratch, 0x2DF) - 2) < 2U) {
                /* UNCLEAR: in asm, `bne $v1, 1, .L80058A24` has `addiu $s0, $zero, 2`
                 * in its delay slot, which sets s0 = 2 regardless of whether (s0 & 0xFF) == 1. */
                if ((s0 & 0xFF) == 1) {
                    s0 = 2;
                } else {
                    s0 = 2;
                }
            }
            func_80057AE4(actor, 0, 0);
            func_800AB620((s0 & 0xFF) + 0xD5B);
        } else {
            func_800AB620(((count - 1) % 3) + 0xD5E);
            count = B(base, 0xAC70 + B(actor, 0x398));
            func_800AB620(((count - 1) % 3) + 0xD61);
        }
        side = B(actor, 0x398);
        B(base, 0xAC70 + side) = (unsigned char)(B(base, 0xAC70 + side) + 1);
        B(base, 0xAC72) = (unsigned char)(B(actor, 0x398) ^ 1);
        return;
    }

    if ((func_80087BF8() & 0xFF) != 0 || B(scratch, 0x2CE) == 1) {
        s0 = (func_8001E6B8(4) & 0xFF) << 1;
        func_800AB620(s0 + 0xCF2);
        func_800AB620(s0 + 0xCF3);
        if (D_8010A397 == 8) {
            func_800AB620(func_8001E6B8(7) + 0xE00);
        }
        return;
    }

    D_8010A3A9 = (unsigned char)(D_8010A3A9 + 1);
    func_800AB620((D_800C7D6C[(unsigned int)D_8010A3A9 % 7U] << 1) + 0xCFA);
    func_800AB620((D_800C7D6C[(unsigned int)D_8010A3A9 % 7U] << 1) + 0xCFB);
    func_80057AE4(actor, 0, 1);

    if (D_8010A397 == 8) {
        func_800AB620(func_8001E6B8(7) + 0xE00);
        return;
    }

    D_800E6C68 = 0;
    for (i = 0; i < 16; i++) {
        if (B(base, 0x88FD + B(actor, 0x398) * 96 + i * 3) == B(actor, 0x392)) {
            D_800E6C68 = (unsigned char)(D_800E6C68 + 1);
        }
    }

    period = B(scratch, 0x2DF);
    if ((unsigned int)period < 2U) {
        if (period != 1) {
            s0 = 0;
        } else {
            s0 = ((func_8004F3CC() / 3600U) < 0x28U) ^ 1;
        }
    } else {
        mode = B(scratch, 0x2E1);
        if (mode != 0 && mode != 3) {
            s0 = 1;
        } else if ((B(table, 0xE) & 2) != 0) {
            s0 = 1;
        } else if (period != 3) {
            s0 = 0;
        } else {
            s0 = 0;
            if ((func_8004F3CC() / 3600U) >= 0xBU) {
                s0 = 1;
            }
        }
    }

    if (D_800E6C68 == 3 && (s0 & 0xFF) == 0) {
        func_800AB620(func_8001E6B8(7) + 0xD1C);
        return;
    }

    side = B(actor, 0x398);
    flag_side = side;
    score_diff = (int)B(scratch, 0x2D1 + side) - (int)B(scratch, 0x2D1 + (side ^ 1));
    phase_byte = B(scratch, 0x2E2);
    if (phase_byte != 1 && phase_byte != 4) {
        flag_side = 0;
    }

    if (score_diff == 0) {
        if ((B(scratch, 0x2D1) + B(scratch, 0x2D2)) == 0) {
            func_8005A1F0(B(scratch, 0x2DD + B(actor, 0x398)), 0x1036);
            func_800AB620(func_8001E6B8(2) + 0xD0E);
        } else if ((s0 & 0xFF) != 0) {
            side = B(actor, 0x398);
            if ((flag_side & 0xFF) == side) {
                func_8005A1F0(B(scratch, 0x2DD + side), 0x1036);
                func_800AB620(func_8001E6B8(3) + 0xD12);
            } else {
                func_8005A1F0(B(scratch, 0x2DD + (side ^ 1)), 0xFD6);
                func_800AB620(func_8001E6B8(5) + 0xD15);
            }
        } else {
            side = B(actor, 0x398);
            if ((flag_side & 0xFF) == side) {
                func_8005A1F0(B(scratch, 0x2DD + side), 0x1006);
                func_800AB620(0xD10);
            } else {
                func_8005A1F0(B(scratch, 0x2DD + (side ^ 1)), 0xFD6);
                func_800AB620(0xD11);
            }
        }
    } else if (score_diff > 0) {
        if (score_diff == 1 && func_8001E6B8(2) != 0) {
            func_8005A1F0(B(scratch, 0x2DD + B(actor, 0x398)), 0x1006);
            func_800AB620(func_8001E6B8(2) + 0xD1A);
        } else {
            func_80058FCC(actor, 1);
        }
    } else if (score_diff == -1) {
        func_8005A1F0(B(scratch, 0x2DD + B(actor, 0x398)), 0x1006);
        if ((s0 & 0xFF) != 0) {
            func_800AB620(func_8001E6B8(3) + 0xD0A);
        } else {
            func_800AB620(func_8001E6B8(2) + 0xD08);
        }
        B(base, 0xAC72) = B(actor, 0x398);
    } else {
        func_80058FCC(actor, 1);
    }
}
