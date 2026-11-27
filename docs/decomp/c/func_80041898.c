/*
 * func_80041898
 *
 * Purpose:
 *   Per-frame update for a player actor in pressing / chasing-ball state
 *   (state 0x0B): tracks the predicted ball landing point, handles partner-
 *   pressure switching when CPU-controlled, moves toward the ball or decelerates
 *   when within 0.256 m, runs tackle/defensive triggers and state transitions,
 *   and updates the sprint/action hold timer (+0x3B0).
 *
 * Arguments:
 *   actor (unsigned char *): pointer to 1000-byte player actor struct.
 *
 * Return value:
 *   void.
 *
 * Scratchpad (0x1F800000) reads/writes:
 *   - 0x012[2] (u16, R): input button state per side.
 *   - 0x022[2] (u16, R): held button mask per side.
 *   - 0x026[2] (u16, R): secondary held button mask per side.
 *   - 0x036/0x04E/0x066 (s16, R): ball predicted xyz at frame 0.
 *   - 0x0F8/0x0FC (s16, R/W): predicted target x/z.
 *   - 0x240/0x241 (u8, R): human-controlled player actor IDs (side 0/1).
 *   - 0x2A9 (u8, R): current ball owner (0 = free).
 *   - 0x2F4 (unsigned char *, R): pointer to ball actor.
 *   - 0x31C (s16, R): ball timer (frames since last touch).
 *   - 0x31E (u8[2][2], R): marked/closest actor IDs per attack direction.
 *   - 0x328 (u8, R): CPU difficulty/reaction flag.
 *   - 0x332 (u8, R): ball-control / trap flag.
 *
 * Actor fields reads/writes:
 *   - +0x02/+0x06 (s16, R): position x/z.
 *   - +0x04 (s16, W): height y (cleared to 0 in substate 0).
 *   - +0x38D (u8, R): actor ID.
 *   - +0x38E (u16, R): current anim ID.
 *   - +0x392 (u8, R): team slot (0..10).
 *   - +0x397 (u8, R/W): substate (0 -> 1, or >=2 skips movement update).
 *   - +0x398 (u8, R): side (0/1).
 *   - +0x399 (u8, R): attack direction flag (0/1).
 *   - +0x39A (u8, R): CPU control flag.
 *   - +0x3A0 (u32, R/W): current speed.
 *   - +0x3A7 (u8, W): set to 1.
 *   - +0x3AA (u8, R/W): body heading (0..255 = 360 deg).
 *   - +0x3AE (u16, R/W): stamina/energy counter (-2, plus -2 when sprinting).
 *   - +0x3B0 (u8, R/W): action/sprint hold counter (initialized to 8 frames = 0.27s).
 *   - +0x3B1 (u8, W): snapshot of 0x2A9 on substate 0 entry.
 *   - +0x3CC/+0x3CE (u16, R/W): latched target x/z.
 *   - +0x3D1 (u8, W): set to 1 or 2 (jump/aerial readiness).
 *   - +0x3D4/+0x3D6 (u16, R/W): input/action masks.
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - -0x74 (-116): 0.464 m ball height threshold for +0x3D1.
 *   - 0x90000 (589824): 768^2 raw^2 (3.072 m ball proximity cap for latched target).
 *   - -0x1FE1 (-8161): -32.644 m X threshold for partner-pressure switching.
 *   - 0x2000 (8192): ball speed threshold (0x2000 >> 8 = 32 raw/frame = 3.84 m/s).
 *   - 0x200 (512): 2.048 m X offset when yielding target to closer teammate.
 *   - 0x1000 (4096): 64^2 raw^2 (0.256 m target arrival threshold).
 *   - 0xFFFF (65535): ~256^2 raw^2 (1.024 m penalty-box auto-shot/clear radius).
 *   - -0x23E2 (-9186): -36.744 m X penalty-box threshold.
 *   - 0x8B6 (2230): 8.92 m Z penalty-box half-width.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];
extern unsigned int D_8010C328[];

extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_80043018(unsigned char *actor, int angle, int arg2);
extern void func_8005BD1C(unsigned char *actor);
extern void func_8004840C(unsigned char *actor);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_80049F90(unsigned char *actor);
extern int func_8005B7CC(int target, int cur, int step);
extern void func_800479B8(unsigned char *actor, int sprint);
extern void func_8005B908(unsigned char *actor);
extern int func_800434D4(unsigned char *actor, int angle);
extern int func_80045504(unsigned char *actor, int state);
extern int func_800552F4(unsigned char *actor);
extern int func_8003F560(unsigned char *actor);
extern int func_8003DB88(unsigned char *actor);

void func_80041898(unsigned char *actor)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    int ball0_dir;
    int sprint;
    unsigned char substate;
    unsigned int dist_sq; /* UNCLEAR: left uninitialized on the S(scratch, 0x31C) < 9 early-branch */

    B(actor, 0x3A7) = 1;
    H(actor, 0x3AE) -= 2;

    ball0_dir = func_8005B36C(S(actor, 2), S(actor, 6), S(scratch, 0x36), S(scratch, 0x66));

    if (B(actor, 0x39A) != 0) {
        if (H(actor, 0x38E) == 4 ||
            func_80043018(actor, ball0_dir & 0xFF, -1) != 0 ||
            (B(scratch, 0x328) != 0 && B(scratch, 0x2A9) == 0)) {
            H(actor, 0x3AE) -= 2;
            sprint = 1;
        } else {
            sprint = 0;
        }
    } else {
        if ((H(scratch, 0x22 + B(actor, 0x398) * 2) & H(actor, 0x3D4)) != 0 &&
            (B(scratch, 0x2A9) != 0 || S(scratch, 0x31C) >= 0x10)) {
            H(actor, 0x3AE) -= 2;
            sprint = 1;
        } else if (B(actor, 0x38D) == B(scratch, 0x240) || B(actor, 0x38D) == B(scratch, 0x241)) {
            sprint = 0;
        } else {
            H(actor, 0x3AE) -= 2;
            sprint = 1;
        }
    }

    substate = B(actor, 0x397);
    if (substate == 0) {
        unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
        B(actor, 0x3B0) = 8;
        S(actor, 4) = 0;
        B(actor, 0x3B1) = B(scratch, 0x2A9);
        H(actor, 0x3CC) = H(ball, 2);
        H(actor, 0x3CE) = H(ball, 6);
        B(actor, 0x397) += 1;
    } else if (substate != 1) {
        goto update_timer;
    }

    if (B(scratch, 0x2A9) == 0 && S(scratch, 0x4E) >= -0x74) {
        B(actor, 0x3D1) = 2;
    } else {
        B(actor, 0x3D1) = 1;
    }

    func_8005BD1C(actor);
    func_8004840C(actor);

    if ((B(scratch, 0x2A9) == 0 || B(scratch, 0x332) == 0) && S(scratch, 0x31C) < 9) {
        if (S(scratch, 0x31C) >= 2) {
            int pred_dir = func_8005B36C(S(actor, 2), S(actor, 6), S(scratch, 0xF8), S(scratch, 0xFC));
            int need_reset;
            if (((pred_dir - B(actor, 0x3AA)) & 0xFF) < 0x81) {
                int d = func_8005B36C(S(actor, 2), S(actor, 6), S(scratch, 0xF8), S(scratch, 0xFC));
                need_reset = ((d - B(actor, 0x3AA)) & 0xFF) >= 0x40;
            } else {
                int d = func_8005B36C(S(actor, 2), S(actor, 6), S(scratch, 0xF8), S(scratch, 0xFC));
                need_reset = ((B(actor, 0x3AA) - d) & 0xFF) >= 0x40;
            }
            if (need_reset && H(actor, 0x38E) != 1) {
                func_8005BA30(actor, 1, 0);
            }
        }
        func_80049F90(actor);
    } else {
        int use_latched = 0;

        if (B(scratch, 0x2A9) != 0) {
            unsigned int stat_word = *(unsigned int *)((unsigned char *)D_8010C328 +
                                                       B(actor, 0x392) * 12 + B(actor, 0x398) * 264);
            if (S(scratch, 0x31C) < -4 - (int)(stat_word >> 28)) {
                unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
                int dx = S(actor, 2) - S(ball, 2);
                int dz = S(actor, 6) - S(ball, 6);
                if (dx * dx + dz * dz <= 0x90000) {
                    H(scratch, 0xF8) = H(actor, 0x3CC);
                    H(scratch, 0xFC) = H(actor, 0x3CE);
                    use_latched = 1;
                }
            }
        }

        if (!use_latched) {
            if (B(scratch, 0x2A9) == 0 && B(actor, 0x39A) != 0) {
                int x_chk = (B(actor, 0x399) != 0) ? -S(scratch, 0xF8) : S(scratch, 0xF8);
                if (x_chk >= -0x1FE1) {
                    unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
                    if (W(ball, 0x3A0) >= 0x2000) {
                        unsigned char partner_id = B(scratch, 0x31E + (B(actor, 0x399) ^ 1) * 2);
                        unsigned char *partner = D_800FF748 + (partner_id % 24) * 1000;
                        unsigned short saved_z = H(scratch, 0xFC);
                        unsigned short saved_x = H(scratch, 0xF8);
                        int dx_self, dz_self, dx_partner, dz_partner;

                        func_8004840C(partner);

                        dx_self = S(actor, 2) - (signed short)saved_x;
                        dz_self = S(actor, 6) - (signed short)saved_z;
                        dx_partner = S(partner, 2) - S(scratch, 0xF8);
                        dz_partner = S(partner, 6) - S(scratch, 0xFC);

                        if (dx_partner * dx_partner + dz_partner * dz_partner <
                            dx_self * dx_self + dz_self * dz_self) {
                            if (B(actor, 0x399) != 0) {
                                S(scratch, 0xF8) = S(scratch, 0xF8) + 0x200;
                            } else {
                                S(scratch, 0xF8) = S(scratch, 0xF8) - 0x200;
                            }
                        } else {
                            H(scratch, 0xF8) = saved_x;
                            H(scratch, 0xFC) = saved_z;
                        }
                    }
                }
            }
            H(actor, 0x3CC) = H(scratch, 0xF8);
            H(actor, 0x3CE) = H(scratch, 0xFC);
        }

        {
            int dx = S(actor, 2) - S(scratch, 0xF8);
            int dz = S(actor, 6) - S(scratch, 0xFC);
            dist_sq = (unsigned int)(dx * dx + dz * dz);
        }

        if (dist_sq < 0x1000) {
            unsigned int speed;
            if (H(actor, 0x38E) != 1) {
                func_8005BA30(actor, 1, 0);
            }
            B(actor, 0x3AA) = func_8005B7CC(ball0_dir & 0xFF, B(actor, 0x3AA), 0x10);
            speed = W(actor, 0x3A0);
            if (speed < 0x13) {
                W(actor, 0x3A0) = 0;
            } else {
                W(actor, 0x3A0) = speed - 0x12;
            }
        } else {
            func_800479B8(actor, sprint);
        }
    }

    func_8005B908(actor);
    if (func_800434D4(actor, ball0_dir & 0xFF) != 0) {
        return;
    }

    if (B(actor, 0x39A) != 0 ||
        (B(actor, 0x38D) != B(scratch, 0x240) && B(actor, 0x38D) != B(scratch, 0x241))) {
        if (dist_sq <= 0xFFFF && B(scratch, 0x2A9) == 0) {
            int x_chk = (B(actor, 0x399) != 0) ? -S(scratch, 0xF8) : S(scratch, 0xF8);
            if (x_chk < -0x23E2) {
                int abs_z = S(scratch, 0xFC);
                if (abs_z < 0) abs_z = -abs_z;
                if (abs_z < 0x8B6) {
                    unsigned short btn = H(scratch, 0x12 + B(actor, 0x398) * 2);
                    H(actor, 0x3D4) = btn;
                    H(actor, 0x3D6) = btn;
                }
            }
        }
    }

    if ((func_80045504(actor, 0xB) & 0xFF) != 0) {
        return;
    }
    if (func_800552F4(actor) == 0) {
        if ((func_8003F560(actor) & 0xFF) != 0) {
            return;
        }
        if (func_8003DB88(actor) != 0) {
            return;
        }
    }

update_timer:
    B(actor, 0x3B0) -= 1;
    if (B(actor, 0x38D) == B(scratch, 0x2A9)) {
        B(actor, 0x3B0) = 8;
    }

    if (B(actor, 0x39A) == 0) {
        if ((H(scratch, 0x26 + B(actor, 0x398) * 2) & H(actor, 0x3D4)) != 0) {
            B(actor, 0x3B0) += 1;
        } else if (B(scratch, 0x2A9) != 0 && B(actor, 0x3B0) >= 8) {
            B(actor, 0x3B0) = 8;
        }
    } else {
        if (B(actor, 0x38D) == B(scratch, 0x31E + B(actor, 0x399) * 2)) {
            B(actor, 0x3B0) += 1;
        }
    }
}
