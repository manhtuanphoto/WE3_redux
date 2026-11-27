/*
 * func_8004D464 - Player dribble touch / aerial ball control nudge
 *
 * Purpose:
 *   Repositions the ball 0x68 (104 raw = 0.416 m) ahead of the player along
 *   body heading (+0x3AA), raising/clamping ball Y to <= -0x20 (-32 raw), and
 *   launches a controlled dribble/trap touch via func_8004B0F0:
 *     - If func_80052C6C(actor) != 0 and actor+0x38E is 1 or 0xD, computes a
 *       soft first-touch speed from the player's control stat (D_8010C32C >> 12)
 *       and angle delta vs incoming ball heading (+0x3A4).
 *     - Otherwise, if the ball is high (ball_y < -0xBF) and func_80052C6C == 0,
 *       plays trap anim 0x22 (state 0xE, substate 0x20, +0x3A0 = 0x30) and reduces
 *       touch speed by 0x12; else plays dribble touch anim 4 (state 3, substate 2)
 *       and turns toward target heading, clamping speed >= 0x3A and setting +0x3D1 = 2.
 *   Finally marks Scratchpad+0x2A9 = actor ID and ball+0x3B0 = ball+0x3B2 = 4.
 *
 * Arguments:
 *   actor      (unsigned char *): pointer to 1000-byte player actor struct.
 *   heading    (unsigned int)   : base dribble/touch heading byte.
 *   target_dir (unsigned int)   : desired turn heading byte (for func_8005B7CC).
 *   speed      (int)            : base touch speed.
 *
 * Return value:
 *   void.
 *
 * Scratchpad (0x1F800000) reads/writes:
 *   - 0x2A9 (u8, W): current ball owner ID (set to actor+0x38D).
 *   - 0x2F4 (u8 *, R): pointer to ball actor (+0x02/+0x04/+0x06, +0x3A4, +0x3B0, +0x3B2).
 *
 * Actor fields read/written:
 *   - +0x02/+0x06 (s16, R): actor X/Z coordinates.
 *   - +0x38D (u8, R): actor ID.
 *   - +0x38E (u16, R): UNCLEAR role/sub-state (checked for 1 or 0xD when func_80052C6C != 0).
 *   - +0x390 (u8, R): animation frame counter.
 *   - +0x392 (u8, R): team slot (stride 12 in D_8010C32C).
 *   - +0x396/+0x397 (u8, W): actor state / sub-state.
 *   - +0x398 (u8, R): side (0/1, stride 264 in D_8010C32C).
 *   - +0x3A0 (u32, R/W): dribble/trap timer or speed field (set to 0x30 on high trap; < 0x21 selects turn rate 0x28 vs 0x10).
 *   - +0x3AA (u8, R): body heading.
 *   - +0x3D1 (u8, W): set to 2.
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x68 (104 raw = 0.416 m): forward ball placement radius.
 *   - -0x20 (-32 raw = 0.128 m): minimum ball height above ground.
 *   - -0xBF (-191 raw = 0.764 m): high-ball trap threshold.
 *   - 0x3A (58): minimum dribble touch speed.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_8010C32C[];

extern int func_8001E694(unsigned int angle, int radius);
extern int func_8001E5A4(unsigned int angle, int radius);
extern unsigned int func_80052C6C(unsigned char *actor);
extern int func_8001E6B8(int range);
extern void func_8005BA30(unsigned char *actor, int anim, int arg2);
extern void func_8005BA78(unsigned char *actor, int anim, int frame);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int current, int step);
extern void func_8004B0F0(unsigned char *actor, unsigned int dir, int speed, int lift);

void func_8004D464(unsigned char *actor, unsigned int heading, unsigned int target_dir, int speed)
{
    unsigned char *ball;
    unsigned int out_dir = heading;
    int out_speed = speed;
    unsigned int tbl_off;
    unsigned int ctrl_stat;
    int rand_lift;

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    H(ball, 0x2) = (unsigned short)(H(actor, 0x2) + func_8001E694(B(actor, 0x3AA), 0x68));

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    H(ball, 0x4) = (unsigned short)(H(ball, 0x4) + 0x20);

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    if (S(ball, 0x4) >= -0x1F) {
        H(ball, 0x4) = (unsigned short)(-0x20);
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    H(ball, 0x6) = (unsigned short)(H(actor, 0x6) + func_8001E5A4(B(actor, 0x3AA), 0x68));

    if ((func_80052C6C(actor) & 0xFFu) != 0u &&
        (H(actor, 0x38E) == 1u || H(actor, 0x38E) == 0xDu)) {
        int base_spd;
        int ang_diff;
        unsigned int diff_byte;

        tbl_off = B(actor, 0x392) * 12u + B(actor, 0x398) * 264u;
        ctrl_stat = (W(D_8010C32C, tbl_off) >> 12) & 0xFu;
        base_spd = func_8001E6B8((9 - (int)ctrl_stat) << 1) + 4;

        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        ang_diff = (int)(heading - B(ball, 0x3A4)) - 0x80;
        diff_byte = (unsigned int)ang_diff & 0xFFu;
        if (diff_byte < 0x81u) {
            out_speed = base_spd + ((int)diff_byte / 12);
        } else {
            unsigned int neg_diff = (unsigned int)(-ang_diff) & 0xFFu;
            out_speed = base_spd + ((int)neg_diff / 12);
        }
    } else {
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        if (S(ball, 0x4) < -0xBF && (func_80052C6C(actor) & 0xFFu) == 0u) {
            func_8005BA30(actor, 0x22, 0);
            out_speed -= 0x12;
            W(actor, 0x3A0) = 0x30;
            B(actor, 0x396) = 0xE;
            B(actor, 0x397) = 0x20;
        } else {
            out_speed += 4;
            if ((func_80052C6C(actor) & 0xFFu) == 0u) {
                func_8005BA78(actor, 4, B(actor, 0x390));
                B(actor, 0x396) = 3;
                B(actor, 0x397) = 2;
            }
        }

        tbl_off = B(actor, 0x392) * 12u + B(actor, 0x398) * 264u;
        ctrl_stat = (W(D_8010C32C, tbl_off) >> 12) & 0xFu;
        out_speed += func_8001E6B8((9 - (int)ctrl_stat) << 1);

        if (W(actor, 0x3A0) < 0x21u) {
            out_dir = func_8005B7CC(target_dir & 0xFFu, B(actor, 0x3AA), 0x28);
        } else {
            out_dir = func_8005B7CC(target_dir & 0xFFu, B(actor, 0x3AA), 0x10);
        }

        if (out_speed < 0x3B) {
            out_speed = 0x3A;
        }
        B(actor, 0x3D1) = 2;
    }

    tbl_off = B(actor, 0x392) * 12u + B(actor, 0x398) * 264u;
    ctrl_stat = (W(D_8010C32C, tbl_off) >> 12) & 0xFu;
    rand_lift = func_8001E6B8((9 - (int)ctrl_stat) << 1);

    func_8004B0F0(actor, out_dir & 0xFFu, out_speed, rand_lift - 8);

    B(Scratchpad, 0x2A9) = B(actor, 0x38D);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    B(ball, 0x3B0) = 4;
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    B(ball, 0x3B2) = 4;
}
