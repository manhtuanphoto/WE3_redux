/*
 * func_8007CF7C
 * Purpose: Handles goal-scored bookkeeping: resets match/camera flags, computes
 *          the goal minute, records scorer/assist/minute in the match goal log,
 *          increments team and per-period scores, triggers goal audio/HUD
 *          celebration state, and advances match phase mode.
 * Arguments: none (void)
 * Return value: void
 * Actor fields read:
 *   - +0x38D (u8): actor ID
 *   - +0x392 (u8): team slot (0..10)
 *   - +0x394 (u16): player ID / appearance code (checked against 0x30A)
 *   - +0x398 (u8): team side (0/1)
 * Scratchpad fields read/written:
 *   - 0x1F8001E8 (u8): set to 1
 *   - 0x1F8001EC (u8): cleared to 0
 *   - 0x1F800294 (s16): cleared to 0
 *   - 0x1F8002A1 (u8): cleared to 0
 *   - 0x1F8002AB (u8): previous ball owner ID (0xFF = none)
 *   - 0x1F8002AD (u8): goal scorer actor index (mod 24)
 *   - 0x1F8002C2 (u8): cleared to 0
 *   - 0x1F8002D1 (u8[2]): team goal counts (capped at 99 = 0x63; max 32 = 0x20 logged)
 *   - 0x1F8002D3 (u8[2][4]): per-period goal counts (capped at 99 = 0x63)
 *   - 0x1F8002DF (u8): current match period (0..1 = regular halves, >=2 = extra time)
 *   - 0x1F8002E0 (u8): home/away side swap flag
 *   - 0x1F8002F0/0x2F1/0x309 (u8): cleared to 0
 * External globals read/written:
 *   - D_800FF748: base of 24-element 1000-byte actor array
 *   - D_800FF748 + 0x88FD..0x88FF (0x80108045): goal log (3 bytes/goal: scorer, assist, minute; 96 bytes/side)
 *   - D_800FF748 + 0xABDB (0x8010A323): own-goal flag (0/1)
 *   - D_80105D54, D_80105D2C (u8): cleared to 0
 *   - D_8010A370..D_8010A384 (s16): HUD/replay banner coordinates
 *   - D_800E6D5A (u8): 1 if scoring player's team still trails by >= 2 goals
 *   - D_800E6D5B (u8): 1 if scorer ID == 0x30A
 *   - D_800E6D5C (u8): celebration animation variant (via func_800815C8)
 *   - D_800F3566 (s16): packed celebration/cutscene selector
 * Magic numbers:
 *   - 3600 ticks/min (60 Hz * 60 s): converts func_8004F3CC() clock to minutes
 *   - 45 / 15 / 90: regular half length, extra-time half length, extra-time base minute
 *   - 0x20 (32): max goal log entries per team
 *   - 0x63 (99): max displayed score per team/period
 *   - 0x521, 0x522: goal crowd/SFX sound IDs
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCHPAD ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_80105D54;
extern unsigned char D_80105D2C;
extern signed short D_8010A370[];
extern signed short D_8010A374;
extern signed short D_8010A376;
extern signed short D_8010A378;
extern signed short D_8010A37A;
extern signed short D_8010A37C;
extern signed short D_8010A384;
extern unsigned char D_800E6D5A;
extern unsigned char D_800E6D5B;
extern unsigned char D_800E6D5C;
extern signed short D_800F3566;

extern void func_8005C49C(void);
extern unsigned int func_8004F3CC(void);
extern unsigned int func_800526C8(unsigned int);
extern void func_8005896C(unsigned char *);
extern void func_8002283C(void);
extern void func_8004A2D0(void);
extern void func_800AB620(int);
extern int func_8001E6B8(int);
extern unsigned char func_800815C8(unsigned char *);
extern void func_800171DC(void);

void func_8007CF7C(void)
{
    unsigned char *scratch = SCRATCHPAD;
    unsigned char *actor;
    unsigned char period;
    unsigned int elapsed;
    unsigned int minute;
    unsigned char side;
    unsigned char ownGoal;
    unsigned char scoringSide;
    unsigned int celebFlag;
    unsigned int celebA = 0;
    unsigned int celebB = 0;
    int scoreDiff;

    B(scratch, 0x2F1) = 0;
    B(scratch, 0x2F0) = 0;
    B(scratch, 0x309) = 0;
    B(scratch, 0x2A1) = 0;
    B(scratch, 0x2C2) = 0;
    B(scratch, 0x1EC) = 0;
    B(scratch, 0x1E8) = 1;
    D_80105D54 = 0;
    D_80105D2C = 0;

    func_8005C49C();

    D_8010A370[B(scratch, 0x2E0)] = -0xE1;
    D_8010A370[B(scratch, 0x2E0) ^ 1] = 0xD8;

    actor = D_800FF748 + (B(scratch, 0x2AD) % 24U) * 1000;

    D_8010A376 = -0x24;
    D_8010A374 = -0x24;
    D_8010A37A = 0x220;
    D_8010A378 = 0x220;
    D_8010A384 = -0x20;
    D_8010A37C = -0x20;

    period = B(scratch, 0x2DF);
    elapsed = func_8004F3CC();
    if (period < 2) {
        minute = period * 45U + (elapsed / 3600U);
    } else {
        minute = (period - 2U) * 15U + (elapsed / 3600U) + 90U;
    }

    side = B(actor, 0x398);
    B(D_800FF748, 0x88FE + B(scratch, 0x2D1 + side) * 3 + side * 96) = 0xFF;

    ownGoal = B(D_800FF748, 0xABDB);
    if (ownGoal == 0) {
        unsigned char teamSide = B(actor, 0x398);
        unsigned char goalCount = B(scratch, 0x2D1 + teamSide);

        if (goalCount < 0x20) {
            unsigned int entryOff = goalCount * 3 + teamSide * 96;
            unsigned char prevOwner;

            B(D_800FF748, 0x88FD + entryOff) = B(actor, 0x392);
            B(D_800FF748, 0x88FF + B(scratch, 0x2D1 + B(actor, 0x398)) * 3 + B(actor, 0x398) * 96) = (unsigned char)minute;

            prevOwner = B(scratch, 0x2AB);
            if (prevOwner != 0xFF && B(actor, 0x38D) != prevOwner) {
                unsigned int prevTeam = func_800526C8(prevOwner) & 0xFF;
                /* UNCLEAR: $a0 is not reloaded before the second call (likely intended
                 * func_800526C8(B(actor, 0x38D))); func_800526C8 clobbers $a0 to (prevOwner < 12). */
                unsigned int curTeam = func_800526C8(prevOwner < 0xC ? 1U : 0U) & 0xFF;

                if (prevTeam == curTeam) {
                    unsigned char *assistActor = D_800FF748 + (prevOwner % 24U) * 1000;
                    unsigned char s = B(actor, 0x398);
                    B(D_800FF748, 0x88FE + B(scratch, 0x2D1 + s) * 3 + s * 96) = B(assistActor, 0x392);
                }
            }
        }
    } else {
        unsigned char oppSide = B(actor, 0x398) ^ 1;
        unsigned char oppGoals = B(scratch, 0x2D1 + oppSide);

        if (oppGoals < 0x20) {
            B(D_800FF748, 0x88FF + oppGoals * 3 + oppSide * 96) = (unsigned char)minute;
        }
    }

    func_8005896C(actor);

    scoringSide = B(actor, 0x398) ^ B(D_800FF748, 0xABDB);
    if (B(scratch, 0x2D1 + scoringSide) < 0x63) {
        B(scratch, 0x2D1 + scoringSide)++;
    }

    {
        unsigned char *periodGoalsPtr = scratch + 0x2D3 + ((B(actor, 0x398) ^ B(D_800FF748, 0xABDB)) << 2) + B(scratch, 0x2DF);
        if (*periodGoalsPtr < 0x63) {
            (*periodGoalsPtr)++;
        }
    }

    S(scratch, 0x294) = 0;
    scoreDiff = (int)B(scratch, 0x2D1 + B(actor, 0x398)) - (int)B(scratch, 0x2D1 + (B(actor, 0x398) ^ 1));
    D_800E6D5A = (scoreDiff < -1) ? 1 : 0;

    func_8002283C();
    func_8004A2D0();
    func_800AB620(0x521);
    func_800AB620(0x522);

    celebFlag = (func_8001E6B8(3) > 0) ? 1U : 0U;
    if ((celebFlag & 0xFF) != 0) {
        celebA = (unsigned int)func_8001E6B8(3);
        if ((celebA & 0xFF) == 0) {
            celebB = (unsigned int)(func_8001E6B8(3) + 3);
        } else {
            celebB = (unsigned int)func_8001E6B8(6);
        }
        if ((celebB & 0xFF) == 3) {
            celebA = 0;
        }
    } else {
        celebA = (unsigned int)(func_8001E6B8(3) + 1);
        celebB = (unsigned int)func_8001E6B8(4);
        if (((celebB - 2) & 0xFF) < 2U) {
            celebA = 3;
        }
    }

    if (D_800E6D5A != 0) {
        celebB = 0;
        celebA = 0;
        celebFlag = 0;
    }
    if (B(D_800FF748, 0xABDB) != 0) {
        celebB = 0;
        celebA = 0;
        celebFlag = 0;
    }

    D_800E6D5B = (H(actor, 0x394) == 0x30A) ? 1 : 0;
    if (D_800E6D5B != 0) {
        D_800F3566 = 0;
    } else {
        D_800F3566 = (signed short)((celebFlag << 7) + ((celebA & 0xFF) << 4) + (celebB & 0xFF));
    }

    actor = D_800FF748 + (B(scratch, 0x2AD) % 24U) * 1000;
    if ((celebFlag & 0xFF) == 0 && ((celebB - 2) & 0xFF) < 2U) {
        D_800E6D5C = func_800815C8(actor);
    } else {
        D_800E6D5C = 0;
    }

    func_800171DC();
}
