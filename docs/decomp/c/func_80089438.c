/*
 * func_80089438
 * Purpose: Set-piece / goal / stoppage event handler. Updates all 22 player actors via
 *          func_80089C08, checks match restart/stoppage state (D_800EF9C8), plays sound/HUD
 *          events, places outfield players into formation celebration/reset positions when
 *          func_80087BF8() succeeds, or handles goal/offside/out-of-bounds outcomes.
 * Arguments:
 *   void
 * Return value:
 *   void
 * Scratchpad reads/writes:
 *   R: 0x2AD (scoring/event actor id), 0x2BC (s16 x), 0x2BE (s16 z),
 *      0x2FD[side] (team tactic), 0x2F4 (ball actor ptr)
 *   W: 0x2A8 (last-touch actor), 0x2A9 (ball owner = 0), 0x2F8 (= 0xFF), 0x331 (= 0)
 *   Ball actor W: +0x04 (y = -32 / -0.128 m), +0x396 (state = 0)
 * Actor reads/writes (loop over 22 players at D_800FF748 + 0x3E8):
 *   R: +0x38D (actor id), +0x398 (side), +0x399 (attack dir)
 *   W: +0x02/+0x06 (x/z), +0x396 (state = 0x11 or 0x2E), +0x397 (substate = 0x10 or 0),
 *      +0x3A0 (speed = 0)
 * Globals:
 *   D_800FF748 (actor array, 1000-byte stride), D_800EF9C8 (u8 event type 0..4+),
 *   D_800C7DF8 / D_800C7DFA (formation s16 x/z table, 40 bytes per tactic, 4 bytes per slot),
 *   D_800FF748 + 0xABDB (0x8010A323: own-goal / team-flip flag).
 * Constants:
 *   0x500 (1280 raw = 5.12 m) random celebration position jitter;
 *   0x12F4 (4852 raw = 19.408 m) penalty-box Z threshold;
 *   -0x23E2 (-9186 raw = -36.744 m) penalty-box X threshold.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCH  ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_800EF9C8;
extern unsigned char D_800C7DF8[];
extern unsigned char D_800C7DFA[];

extern void func_800171C4(int arg0);
extern void func_800171DC(void);
extern void func_800171FC(int arg0);
extern void func_8001722C(int arg0);
extern void func_8001DD50(int arg0);
extern int func_8001E6B8(int n);
extern int func_8004FBC4(int arg0);
extern void func_8004FD14(int arg0);
extern unsigned int func_800526C8(unsigned int actor_id);
extern void func_80055974(void);
extern void func_800597C8(int arg0);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern int func_8005C7A0(int range);
extern unsigned char func_8005CE14(unsigned int actor_id);
extern unsigned char func_8005CE30(unsigned int actor_id);
extern int func_80087BF8(void);
extern void func_80089BD8(void);
extern void func_80089C08(unsigned char *actor);
extern void func_80097D6C(void);
extern void func_800AB620(int sfx);

void func_80089438(void) {
    unsigned char *actor;
    unsigned char *ball;
    int i;
    int actor_id;
    int slot_idx;
    int tactic;
    int tbl_off;
    int tx;
    int tz;
    unsigned int own_goal_flag;
    unsigned int side1;
    unsigned int side2;
    int anim;
    int abs_z;
    int bx;
    int in_box;

    actor = D_800FF748 + 0x3E8;
    for (i = 0; i < 0x16; i++) {
        func_80089C08(actor);
        actor += 0x3E8;
    }

    if (func_8004FBC4(0) == 0) {
        return;
    }

    func_8001DD50(0xB);
    func_80089BD8();
    func_8004FD14(1);

    switch (D_800EF9C8) {
    case 0:
        func_800AB620(0x11);
        /* Fall through */
    default:
        if (func_80087BF8() != 0) {
            actor = D_800FF748 + 0x3E8;
            for (i = 0; i < 0x16; i++) {
                actor_id = B(actor, 0x38D);
                if (actor_id != 1 && actor_id != 0xC) {
                    if ((unsigned int)actor_id < 0xD) {
                        slot_idx = actor_id - 2;
                    } else {
                        slot_idx = actor_id - 0xD;
                    }

                    tactic = B(SCRATCH, 0x2FD + B(actor, 0x398));
                    tbl_off = slot_idx * 4 + tactic * 40;

                    tx = S(D_800C7DF8, tbl_off);
                    if (B(actor, 0x399) != 0) {
                        tx = -tx;
                    }
                    S(actor, 2) = (signed short)tx;

                    tz = S(D_800C7DFA, tbl_off);
                    if (B(actor, 0x399) != 0) {
                        tz = -tz;
                    }
                    S(actor, 6) = (signed short)tz;

                    S(actor, 2) = (signed short)(func_8005C7A0(0x500) + (S(actor, 2) * 6) / 10);
                    S(actor, 6) = (signed short)(func_8005C7A0(0x500) + (S(actor, 6) * 6) / 10);
                }

                if (func_8001E6B8(4) == 0) {
                    B(actor, 0x396) = 0x2E;
                    B(actor, 0x397) = 0;
                } else {
                    own_goal_flag = B(D_800FF748, 0xABDB); /* 0x8010A323 */
                    side1 = func_800526C8(B(actor, 0x38D)) & 0xFF;
                    side2 = func_800526C8(B(SCRATCH, 0x2AD)) & 0xFF;
                    if ((own_goal_flag == 0 && side1 != side2) ||
                        (own_goal_flag != 0 && side1 == side2)) {
                        W(actor, 0x3A0) = 0;
                        if (func_8001E6B8(2) != 0) {
                            anim = 0x21;
                        } else if (func_8001E6B8(2) != 0) {
                            anim = 0x46;
                        } else {
                            anim = 0x47;
                        }
                        func_8005BA30(actor, anim, 0);
                        B(actor, 0x396) = 0x11;
                        B(actor, 0x397) = 0x10;
                    } else {
                        B(actor, 0x396) = 0x2E;
                        B(actor, 0x397) = 0;
                    }
                }
                actor += 0x3E8;
            }

            func_80097D6C();
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            B(SCRATCH, 0x2A9) = 0;
            B(SCRATCH, 0x2A8) = 0;
            B(ball, 0x396) = 0;
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            S(ball, 4) = -0x20; /* -0.128 m */
            W(SCRATCH, 0x2F8) = 0xFF;
            B(SCRATCH, 0x331) = 0;
            func_80055974();
            func_800171DC();
        } else {
            if (B(D_800FF748, 0xABDB) == 0) {
                B(SCRATCH, 0x2A8) = func_8005CE30(B(SCRATCH, 0x2AD));
            } else {
                B(SCRATCH, 0x2A8) = func_8005CE14(B(SCRATCH, 0x2AD));
            }
            func_8001722C(3);
        }
        return;

    case 1:
        func_800171C4(0xC);
        func_800171FC(1);
        return;

    case 2:
        func_800171C4(9);
        func_800171FC(1);
        return;

    case 3:
        func_800171C4(4);
        func_800171FC(1);
        return;

    case 4:
        actor = D_800FF748 + (B(SCRATCH, 0x2AD) % 24) * 0x3E8;
        func_800171C4(2);
        abs_z = S(SCRATCH, 0x2BE);
        if (abs_z < 0) {
            abs_z = -abs_z;
        }
        in_box = 0;
        if (abs_z < 0x12F4) { /* < 19.408 m */
            bx = S(SCRATCH, 0x2BC);
            if (B(actor, 0x399) != 0) {
                bx = -bx;
            }
            if (bx < -0x23E2) { /* < -36.744 m */
                in_box = 1;
                func_800171FC(2);
            }
        }
        func_800597C8(in_box);
        return;
    }
}
