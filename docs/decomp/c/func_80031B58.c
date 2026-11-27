/*
 * func_80031B58
 *
 * Purpose:
 *   Pass initiation and target resolution for a player actor. Either resolves a
 *   preset pass target (actor+0x39A != 0 via func_80054BA4 + func_80031E80) or
 *   computes desired pass direction via func_8004A07C, turns toward it, and
 *   invokes func_80031E80 when behind attacking x <= 4096 (16.384 m) or facing
 *   within 80/256 (112.5 deg) of the attacking direction. If no target is found
 *   (+0x3C2 == 0) in the attacking/defensive zones, may abort with +0x3C2 = 0xFF
 *   or 0xFE; otherwise selects the striking foot (+0x3AC), triggers pass
 *   animation (0x0F, 0x15), calls func_80049DF0, and advances +0x397 to 3.
 *
 * Arguments:
 *   actor (unsigned char *): pointer to 1000-byte player actor struct
 *
 * Return value:
 *   void
 *
 * Scratchpad / Actor / Global fields read & written:
 *   SCRATCH 0x040/0x070 (s16, R): ball predicted x/z
 *   D_800FF748[(id % 24) * 0x3E8]: target actor struct (+0x06 z)
 *   D_8010C330[side*264 + slot*12] (u32, R): bit 4 = preferred foot
 *   Actor +0x02/+0x06 (s16, R): world x, z
 *   Actor +0x392 (u8, R): roster slot
 *   Actor +0x397 (u8, W): sub-step set to 3 on pass execution
 *   Actor +0x398 (u8, R): side (0/1)
 *   Actor +0x399 (u8, R): attack direction flag (0/1)
 *   Actor +0x39A (u8, R): preset target flag
 *   Actor +0x3AA (u8, R): body heading
 *   Actor +0x3AB (u8, W): target heading
 *   Actor +0x3AC (u8, W): striking foot (0/1)
 *   Actor +0x3C2 (s16, R/W): chosen pass target ID (or 0xFE / 0xFF sentinel)
 *   Actor +0x3C8 (s16, R): target X coordinate set by func_80031E80
 *
 * Magic numbers (250 raw = 1 m, 256 angle units = 360 deg):
 *   0x1001 (4097) = ~16.388 m attacking-half threshold
 *   0x1BE2 (7138) = ~28.552 m deep-attack x threshold
 *   0x14F5 (5365) = ~21.46 m penalty-width |z| threshold
 *   -0x198F (-6543) = ~-26.172 m deep-defensive x threshold
 *   0x50 (80) = 112.5 deg; 0x18 (24) = 33.75 deg
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010C330[];

extern unsigned int func_80054BA4(unsigned char *actor, unsigned int arg1);
extern unsigned int func_8004A07C(unsigned char *actor);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);
extern unsigned int func_80031E80(unsigned char *actor, unsigned int heading);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_80067E7C(unsigned char *actor);
extern void         func_8005BA30(unsigned char *actor, unsigned int anim_id, unsigned int arg2);
extern void         func_80049DF0(unsigned char *actor, unsigned int heading, unsigned int arg2, unsigned int arg3);

void func_80031B58(unsigned char *actor) {
    unsigned int heading = 0; /* UNCLEAR: uninitialized in asm when actor+0x39A != 0 (func_80031E80 overwrites it in that mode) */
    signed short target_id;
    unsigned int pass_ang;
    unsigned int ball_ang;
    int ang_diff;

    if (B(actor, 0x39A) != 0) {
        H(actor, 0x3C2) = (unsigned short)func_80054BA4(actor, B(actor, 0x3C2));
        /* UNCLEAR: asm calls func_80031E80 without reloading $a0 after func_80054BA4 */
        H(actor, 0x3C2) = (unsigned short)(func_80031E80(actor, heading & 0xFFu) & 0xFFu);
    } else {
        unsigned int desired = func_8004A07C(actor);
        signed short x = S(actor, 0x02);
        int run_pass_search = 0;

        heading = func_8005B7CC(desired & 0xFFu, B(actor, 0x3AA), 0x20);

        if (B(actor, 0x399) != 0) {
            if (-x < 0x1001) {
                run_pass_search = 1;
            }
        } else {
            if (x < 0x1001) {
                run_pass_search = 1;
            }
        }

        if (!run_pass_search) {
            unsigned int atk_h = (unsigned int)B(actor, 0x399) << 7;
            unsigned char body_h = B(actor, 0x3AA);
            unsigned int diff = (atk_h - body_h) & 0xFFu;
            if (diff < 0x81u) {
                run_pass_search = ((int)diff < 0x50);
            } else {
                run_pass_search = ((int)((body_h - atk_h) & 0xFFu) < 0x50);
            }
        }

        if (run_pass_search) {
            H(actor, 0x3C2) = (unsigned short)(func_80031E80(actor, heading & 0xFFu) & 0xFFu);
        } else {
            S(actor, 0x3C2) = 0;
        }
    }

    target_id = S(actor, 0x3C2);
    if (target_id != 0) {
        int slot = target_id % 24;
        unsigned char *target = D_800FF748 + slot * 0x3E8;
        pass_ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(actor, 0x3C8), S(target, 0x06));
    } else {
        signed short signed_x = (B(actor, 0x399) != 0) ? (signed short)(-S(actor, 0x02)) : S(actor, 0x02);

        if (signed_x >= 0x1001) {
            if (signed_x < 0x1BE2) {
                signed short abs_z = S(actor, 0x06);
                if (abs_z < 0) {
                    abs_z = (signed short)(-abs_z);
                }
                if (abs_z < 0x14F5) {
                    unsigned int atk_h = (unsigned int)B(actor, 0x399) << 7;
                    unsigned char body_h = B(actor, 0x3AA);
                    unsigned int diff = (atk_h - body_h) & 0xFFu;
                    int facing_ok = (diff < 0x81u) ? ((int)diff < 0x50) : ((int)((body_h - atk_h) & 0xFFu) < 0x50);
                    if (facing_ok) {
                        if (func_80067E7C(actor) != 0) {
                            S(actor, 0x3C2) = 0x00FF;
                            return;
                        }
                        goto check_own_half;
                    }
                }
            }
            S(actor, 0x3C2) = 0x00FF;
            return;
        }

check_own_half:
        if (signed_x < -0x198F) {
            S(actor, 0x3C2) = 0x00FE;
            return;
        }
        pass_ang = B(actor, 0x3AA);
    }

    ball_ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(SCRATCH, 0x040), S(SCRATCH, 0x070));
    ang_diff = (int)((pass_ang - ball_ang) & 0xFFu);
    if (ang_diff < 0x18) {
        unsigned int stat_off = (unsigned int)B(actor, 0x392) * 12u + (unsigned int)B(actor, 0x398) * 264u;
        B(actor, 0x3AC) = (unsigned char)((W(D_8010C330, stat_off) >> 4) & 1u);
    } else {
        B(actor, 0x3AC) = (unsigned char)((unsigned int)ang_diff >> 7);
    }

    func_8005BA30(actor, 0x0F, 0x15);
    pass_ang = func_8005B7CC(pass_ang & 0xFFu, ball_ang & 0xFFu, 0x0C);
    func_80049DF0(actor, pass_ang & 0xFFu, 0x1A, 5);
    B(actor, 0x3AB) = (unsigned char)pass_ang;
    B(actor, 0x397) = 3;
}
