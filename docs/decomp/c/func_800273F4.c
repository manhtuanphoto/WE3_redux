/*
 * func_800273F4
 *
 * Purpose:
 *   Per-frame player AI / ball-chasing and interception state machine (substate
 *   dispatcher on actor+0x397: 0x00, 0x01, 0x10, 0x18, 0x20, 0x21, 0x30, 0x31, 0x40).
 *   Handles loose-ball / pass interception approach, speed/turn adjustment,
 *   challenge/tackle or slide triggers, and transition to other player states.
 *
 * Arguments:
 *   actor (u8 *) - pointer to 1000-byte player actor struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Actor fields:
 *     +0x002 (s16) x, +0x006 (s16) z
 *     +0x38D (u8)  actor id (R)
 *     +0x38E (u16) current anim id (R)
 *     +0x390 (u8)  anim frame (R)
 *     +0x392 (u8)  team slot (R)
 *     +0x396 (u8)  state (W)
 *     +0x397 (u8)  substate (R/W)
 *     +0x398 (u8)  side 0/1 (R)
 *     +0x399 (u8)  attack direction flag (R)
 *     +0x39A (u8)  dash/ai flag (R)
 *     +0x3A0 (u32) speed (R/W)
 *     +0x3A4 (u8)  move heading (R/W)
 *     +0x3A7 (u8)  written = 1 on entry (W)
 *     +0x3AA (u8)  body heading (R/W)
 *     +0x3AB (u8)  target/facing heading (R)
 *     +0x3C2 (s16) queued next substate (R)
 *     +0x3C4 (s16) lead/chase distance or offset (R/W)
 *     +0x3C6 (s16/u16) queued next state (R/W)
 *     +0x3CA (s16) x step delta in substate 0x21 (R)
 *     +0x3CE (s16) z step delta in substate 0x21 (R)
 *     +0x3D0 (u8)  wait/hold timer (R/W)
 *     +0x3D1 (u8)  facing/posture flag (W)
 *     +0x3D4 (u16) written from SCRATCH+0x12+side*2 (W)
 *   Scratchpad (0x1F800000):
 *     +0x012 (u16[2]) per-side value (R)
 *     +0x036/+0x066 (s16) ball/reference x/z (R)
 *     +0x04E (s16) ball height / param (R)
 *     +0x05E (s16) ball height / param (R)
 *     +0x0F2 (s16) scratch y/offset param (R/W)
 *     +0x0F8/+0x0FC (s16) predicted target x/z (R/W)
 *     +0x0FE/+0x100 (s16) alternate target x/z (R)
 *     +0x28C (u8)  direction table index into D_800C6B74 (R)
 *     +0x2A9 (u8)  ball owner (R/W)
 *     +0x2AA (u8)  previous owner (R)
 *     +0x2B0 (u32) ball event code (R)
 *     +0x2B8 (s16) target/lock sentinel 0x7FFF (R)
 *     +0x2CF (u8[2]) per-side possession flags (R)
 *     +0x2F4 (u8*) ptr to ball actor (R)
 *     +0x31C (s16) ball timer, frames since last touch (R)
 *     +0x328 (u8)  sprint/mode flag (R)
 *   Globals:
 *     D_800C6B74 (u8[])  heading lookup table
 *     D_800FF748 (u8[])  actor array (1000-byte stride)
 *     D_8010C32A (u8[])  player stat table base+0 (side*264 + slot*12)
 *     D_8010C330 (u8[])  player stat table base+6 (side*264 + slot*12)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x23E2 = 9186 raw (36.744 m), 0x23E3 = 9187 raw (36.748 m) - attacking third X
 *   0x25E2 = 9698 raw (38.792 m) - deep attacking X
 *   0x3320 = 13088 raw (52.352 m), 0x3340 = 13120 raw (52.48 m) - goal line X
 *   0x12F4 = 4852 raw (19.408 m) - penalty box half-width Z
 *   0x2100 = 8448 raw (33.792 m) - touchline Z
 *   0x198F = 6543 raw (26.172 m) - own half threshold X
 *   0x1000 = 64^2 (0.256 m), 0x23FFF = ~384^2 (1.536 m), 0x23FFFF = ~1536^2 (6.144 m)
 *   0x400 = 1024 raw (4.096 m), 0xC0 = 192 raw (0.768 m), 0x80 = 128 raw (0.512 m)
 *   0x73/0x74 = 115/116 raw (0.46 m) height, 0x160 = 352 raw (1.408 m) height
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned char D_800C6B74[];
extern unsigned char D_800FF748[];
extern unsigned char D_8010C32A[];
extern unsigned char D_8010C330[];

void func_800273F4(unsigned char *actor)
{
    unsigned int sprint_flag;
    unsigned int substate;
    unsigned char *ball;
    int dist;
    int target_ang;
    int ball_ang;
    int ref_ang;
    int step_param;
    unsigned char *target_actor;

    /* UNCLEAR: these locals are read uninitialized on specific switch paths in the original binary */
    int uninit_ref_ang;
    int uninit_target_ang;
    unsigned char *uninit_actor_or_ptr;

    B(actor, 0x3A7) = 1;
    func_8005BD1C(actor);

    sprint_flag = 1;
    if (B(actor, 0x39A) != 0 && B(SCRATCH, 0x328) == 0) {
        sprint_flag = 0;
    }

    substate = B(actor, 0x397);
    switch (substate) {
    case 0x00: {
        if (B(SCRATCH, 0x2A9) == 0 && B(actor, 0x38D) != B(SCRATCH, 0x2AA)) {
            unsigned int poss = B(SCRATCH, 0x2CF + B(actor, 0x398));
            if ((poss == 0 && S(SCRATCH, 0x31C) < 5) ||
                (poss == 1 && S(SCRATCH, 0x31C) < 9)) {
                func_80049F90(actor);
                func_8005B908(actor);
                return;
            }
        }

        {
            signed short next_sub = S(actor, 0x3C2);
            B(actor, 0x3D1) = 1;
            if (next_sub == 0xFF) {
                B(actor, 0x397) = 0x10;
            } else {
                B(actor, 0x397) = (unsigned char)next_sub;
            }
        }

        {
            int in_attack_box = 0;
            if (W(SCRATCH, 0x2B0) == 3) {
                in_attack_box = 1;
            } else {
                signed short x = S(actor, 0x02);
                int deep_x = B(actor, 0x399) ? (-x >= 0x23E3) : (x >= 0x23E3);
                if (deep_x) {
                    signed short abs_z = S(actor, 0x06);
                    if (abs_z < 0) {
                        abs_z = -abs_z;
                    }
                    if (abs_z < 0x12F4) {
                        in_attack_box = 1;
                    }
                }
            }

            if (in_attack_box) {
                unsigned int stat = (W(D_8010C330, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) >> 11) & 0x7F;
                S(actor, 0x3C4) = (signed short)(((int)(stat + 100) * 640) / 180);
            } else if (W(SCRATCH, 0x2B0) == 6) {
                S(actor, 0x3C4) = 0x30;
            } else {
                int r = func_8001E6B8(0x40);
                unsigned int stat = (W(D_8010C330, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) >> 11) & 0x7F;
                S(actor, 0x3C4) = (signed short)(((r + 0x100) * (int)(stat + 100)) / 180);
            }
        }
        break;
    }

    case 0x01: {
        if (B(SCRATCH, 0x2A9) == 0 &&
            B(SCRATCH, 0x2CF + B(actor, 0x398)) == 1 &&
            B(actor, 0x38D) != B(SCRATCH, 0x2AA) &&
            S(SCRATCH, 0x31C) < 5) {
            func_80049F90(actor);
            func_8005B908(actor);
            return;
        }

        B(actor, 0x3D1) = (S(SCRATCH, 0x4E) < -0x74) ? 1 : 2;
        func_8004840C(actor);

        if (B(actor, 0x38D) != B(SCRATCH, 0x2AA) &&
            S(SCRATCH, 0x31C) < 0x11 &&
            H(actor, 0x38E) != 4) {
            sprint_flag = 0;
        }

        {
            signed short pred_x = S(SCRATCH, 0xF8);
            int deep_pred = B(actor, 0x399) ? (-pred_x >= 0x23E2) : (pred_x >= 0x23E2);
            if (deep_pred && B(SCRATCH, 0x2CF + B(actor, 0x398)) == 0) {
                unsigned char ball_dir = B((unsigned char *)W(SCRATCH, 0x2F4), 0x3A4);
                int abs_dir = (ball_dir < 0x80) ? ball_dir : -(int)ball_dir;
                if (((abs_dir - 0x10) & 0xFF) < 0x41) {
                    if (B(actor, 0x399) == 0) {
                        S(SCRATCH, 0xF8) -= 0x80;
                    } else {
                        S(SCRATCH, 0xF8) += 0x80;
                    }
                }
            }
        }

        {
            int dx = S(actor, 0x02) - S(SCRATCH, 0xF8);
            int dz = S(actor, 0x06) - S(SCRATCH, 0xFC);
            if (dx * dx + dz * dz < 0x1000) {
                unsigned int opp_dist;
                unsigned int spd;
                unsigned char heading;

                if (H(actor, 0x38E) != 1) {
                    func_8005BA30(actor, 1, 0);
                }
                opp_dist = func_800494C0(actor);
                /* UNCLEAR: uninit_ref_ang ($s5) is read uninitialized on case 0x01 in the original binary */
                if (opp_dist <= 0x18FFFF) {
                    unsigned char opp_dir = (B((unsigned char *)W(SCRATCH, 0x2F4), 0x3A4) + 0x80) & 0xFF;
                    uninit_ref_ang = func_8005B7CC(uninit_ref_ang & 0xFF, opp_dir, 0x18);
                } else if (B(SCRATCH, 0x28C) != 0) {
                    uninit_ref_ang = func_8005B7CC(D_800C6B74[B(SCRATCH, 0x28C)], uninit_ref_ang & 0xFF, 0x20);
                }
                heading = func_8005B7CC(uninit_ref_ang & 0xFF, B(actor, 0x3AA), 0x10);
                B(actor, 0x3AA) = heading;
                B(actor, 0x3A4) = heading;
                spd = W(actor, 0x3A0);
                W(actor, 0x3A0) = (spd < 0x13) ? 0 : (spd - 0x12);
            } else {
                func_800479B8(actor, sprint_flag);
            }
        }

        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        ball_ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(ball, 0x02), S(ball, 0x06));
        if (func_800434D4(actor, ball_ang & 0xFF) != 0) {
            return;
        }
        if (func_800552F4(actor) != 0) {
            break;
        }
        func_80045504(actor, 0x11);
        break;
    }

    case 0x10: {
        if (B(SCRATCH, 0x2A9) == 0 &&
            B(SCRATCH, 0x2CF + B(actor, 0x398)) == 1 &&
            B(actor, 0x38D) != B(SCRATCH, 0x2AA) &&
            S(SCRATCH, 0x31C) < 5) {
            func_80049F90(actor);
            func_8005B908(actor);
            return;
        }

        B(actor, 0x3D1) = 1;
        S(SCRATCH, 0xF2) = -S(actor, 0x3C4);

        {
            int flag = 0;
            if (B(SCRATCH, 0x2CF + B(actor, 0x398)) == 0 &&
                (W(SCRATCH, 0x2B0) == 2 || W(SCRATCH, 0x2B0) == 8)) {
                flag = 1;
            }
            step_param = func_800484F8(S(actor, 0x02), S(actor, 0x06), H(actor, 0x3A0), flag);
        }

        if (B(SCRATCH, 0x2CF + B(actor, 0x398)) == 1 && W(SCRATCH, 0x2B0) == 1) {
            signed short alt_x = S(SCRATCH, 0xFE);
            signed short alt_z = S(SCRATCH, 0x100);
            signed short bx;
            signed short bz;
            int d1x, d1z, d2x, d2z;

            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            bx = S(ball, 0x02);
            bz = S(ball, 0x06);
            d1x = bx - alt_x;
            d1z = bz - alt_z;
            d2x = bx - S(SCRATCH, 0xF8);
            d2z = bz - S(SCRATCH, 0xFC);
            if (d1x * d1x + d1z * d1z < d2x * d2x + d2z * d2z) {
                S(SCRATCH, 0xF8) = alt_x;
                S(SCRATCH, 0xFC) = alt_z;
            }
        }

        if (S(SCRATCH, 0x2B8) == 0x7FFF) {
            signed short pred_x = S(SCRATCH, 0xF8);
            int deep_pred = B(actor, 0x399) ? (-pred_x >= 0x23E2) : (pred_x >= 0x23E2);
            if (deep_pred && B(SCRATCH, 0x2CF + B(actor, 0x398)) == 0) {
                unsigned char ball_dir = B((unsigned char *)W(SCRATCH, 0x2F4), 0x3A4);
                int abs_dir = (ball_dir < 0x80) ? ball_dir : -(int)ball_dir;
                if (((abs_dir - 0x10) & 0xFF) < 0x41) {
                    if (B(actor, 0x399) == 0) {
                        S(SCRATCH, 0xF8) -= 0x80;
                    } else {
                        S(SCRATCH, 0xF8) += 0x80;
                    }
                }
            }
        }

        target_ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(SCRATCH, 0xF8), S(SCRATCH, 0xFC));
        ref_ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(SCRATCH, 0x36), S(SCRATCH, 0x66));
        {
            int dx = S(actor, 0x02) - S(SCRATCH, 0xF8);
            int dz = S(actor, 0x06) - S(SCRATCH, 0xFC);
            dist = func_800B1128(dx * dx + dz * dz);
        }

        {
            signed short abs_x = S(actor, 0x02);
            if (abs_x < 0) {
                abs_x = -abs_x;
            }

            if ((unsigned int)dist < 0x40 || abs_x >= 0x3340) {
                unsigned int opp_dist;
                unsigned int spd;
                unsigned char heading;

                if (H(actor, 0x38E) != 1) {
                    func_8005BA30(actor, 1, 0);
                }
                opp_dist = func_800494C0(actor);
                if (opp_dist <= 0x18FFFF) {
                    unsigned char opp_dir = (B((unsigned char *)W(SCRATCH, 0x2F4), 0x3A4) + 0x80) & 0xFF;
                    ref_ang = func_8005B7CC(ref_ang & 0xFF, opp_dir, 0x18);
                } else if (B(SCRATCH, 0x28C) != 0) {
                    ref_ang = func_8005B7CC(D_800C6B74[B(SCRATCH, 0x28C)], ref_ang & 0xFF, 0x20);
                }
                heading = func_8005B7CC(ref_ang & 0xFF, B(actor, 0x3AA), 0x10);
                B(actor, 0x3AA) = heading;
                B(actor, 0x3A4) = heading;
                spd = W(actor, 0x3A0);
                W(actor, 0x3A0) = (spd < 0x13) ? 0 : (spd - 0x12);
            } else {
                int used_fast_path = 0;
                if (S(SCRATCH, 0x2B8) != 0x7FFF) {
                    unsigned int opp_dist = func_800494C0(actor);
                    if (opp_dist <= 0xFFFF && (unsigned int)dist < 0x200) {
                        func_80048FC4(actor, target_ang & 0xFF, ref_ang & 0xFF, dist & 0xFFFF);
                        used_fast_path = 1;
                    }
                }

                if (!used_fast_path) {
                    unsigned int est_frames = (unsigned int)dist / 34;
                    unsigned int lead_frames = (H(actor, 0x38E) != 4) ? (est_frames + 8) : (est_frames + 12);

                    if (func_80053D34(actor) != 0) {
                        unsigned char actor_dir = B(actor, 0x3A4);
                        unsigned char ball_dir = B((unsigned char *)W(SCRATCH, 0x2F4), 0x3A4);
                        unsigned int diff = (ball_dir - actor_dir) & 0xFF;
                        int same_dir = (diff < 0x81) ? (diff < 0x21) : (((actor_dir - ball_dir) & 0xFF) < 0x21);

                        if (same_dir) {
                            int dx, dz;
                            ball = (unsigned char *)W(SCRATCH, 0x2F4);
                            dx = S(actor, 0x02) - S(ball, 0x02);
                            dz = S(actor, 0x06) - S(ball, 0x06);
                            if (dx * dx + dz * dz <= 0x23FFF) {
                                unsigned int ball_spd = W(ball, 0x3A0) >> 8;
                                if ((unsigned int)dist >= ball_spd) {
                                    dist = ball_spd;
                                }
                            }
                        }
                        func_80048AD8(actor, target_ang & 0xFF, ref_ang & 0xFF, dist & 0xFFFF);
                    } else if ((unsigned int)dist < 0x100 && (lead_frames & 0xFF) < (step_param & 0xFF)) {
                        func_80048FC4(actor, target_ang & 0xFF, ref_ang & 0xFF, dist & 0xFFFF);
                    } else {
                        func_800479B8(actor, (S(SCRATCH, 0x31C) < 0x18) ? 0 : 1);
                    }
                }

                {
                    unsigned char body_dir = B(actor, 0x3AA);
                    unsigned int diff = (ref_ang - body_dir) & 0xFF;
                    int facing_ref = (diff < 0x81) ? (diff < 0x11) : (((body_dir - ref_ang) & 0xFF) < 0x11);
                    if (facing_ref && B(actor, 0x3AA) == B(actor, 0x3A4)) {
                        ball = (unsigned char *)W(SCRATCH, 0x2F4);
                        if (S(ball, 0x04) >= -0x73) {
                            int vy = SW(ball, 0x0C) / 736;
                            if (vy < 0) {
                                vy = -vy;
                            }
                            if (vy < 10) {
                                B(actor, 0x397) = 1;
                            }
                        }
                    }
                }
            }
        }

        if (func_800552F4(actor) != 0) {
            break;
        }
        func_800547A4(actor, ref_ang & 0xFF);
        func_80045504(actor, 0x11);
        break;
    }

    case 0x18: {
        unsigned int opp_dist;
        unsigned char heading;
        unsigned int spd;

        B(actor, 0x3D1) = 1;
        if (H(actor, 0x38E) != 1) {
            func_8005BA30(actor, 1, 0);
        }
        target_ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(SCRATCH, 0x36), S(SCRATCH, 0x66));
        opp_dist = func_800494C0(actor);
        if (opp_dist <= 0x18FFFF) {
            unsigned char opp_dir = (B((unsigned char *)W(SCRATCH, 0x2F4), 0x3A4) + 0x80) & 0xFF;
            target_ang = func_8005B7CC(target_ang & 0xFF, opp_dir, 0x18);
        } else if (B(SCRATCH, 0x28C) != 0) {
            target_ang = func_8005B7CC(D_800C6B74[B(SCRATCH, 0x28C)], target_ang & 0xFF, 0x20);
        }
        heading = func_8005B7CC(target_ang & 0xFF, B(actor, 0x3AA), 0x10);
        B(actor, 0x3AA) = heading;
        spd = W(actor, 0x3A0);
        W(actor, 0x3A0) = (spd < 0x13) ? 0 : (spd - 0x12);
        func_8004840C(actor);
        func_80045504(actor, 0x11);
        break;
    }

    case 0x20: {
        unsigned char aim_ang;
        int cos_off, sin_off;
        signed short tx, tz;
        int dx, dz;

        B(actor, 0x3D1) = 0;
        S(SCRATCH, 0xF2) = -0xD0;
        step_param = func_800489A8();
        aim_ang = B(actor, 0x3AB);
        cos_off = func_8001E694(aim_ang, 0xC0);
        tx = (signed short)(H(SCRATCH, 0xF8) + cos_off);
        sin_off = func_8001E5A4(aim_ang, 0xC0);
        tz = (signed short)(H(SCRATCH, 0xFC) + sin_off);

        if (B(actor, 0x39A) != 0) {
            signed short x = S(actor, 0x02);
            int deep_x = B(actor, 0x399) ? (-x >= 0x25E2) : (x >= 0x25E2);
            if (deep_x) {
                H(actor, 0x3D4) = H(SCRATCH, 0x12 + B(actor, 0x398) * 2);
            }
        }

        ref_ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), tx, tz);
        func_8003172C(actor, ref_ang & 0xFF);
        if ((func_80045504(actor, 0x11) & 0xFF) != 0) {
            break;
        }
        if (func_80028C68(actor) != 0) {
            H(actor, 0x3C6) = 0xFF;
            break;
        }

        dx = S(actor, 0x02) - tx;
        dz = S(actor, 0x06) - tz;
        dist = func_800B1128(dx * dx + dz * dz);

        {
            int keep_chasing = 0;
            if (W(SCRATCH, 0x2B0) == 2 && (unsigned int)dist >= 0x40) {
                if (S(SCRATCH, 0x2B8) != 0x7FFF) {
                    keep_chasing = 1;
                } else {
                    signed short pred_x = S(SCRATCH, 0xF8);
                    int deep_pred = B(actor, 0x399) ? (-pred_x >= 0x23E2) : (pred_x >= 0x23E2);
                    if (deep_pred) {
                        keep_chasing = 1;
                    }
                }
            }

            if (!keep_chasing) {
                unsigned int stat = (W(D_8010C330, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) >> 11) & 0x7F;
                B(actor, 0x397) = 0x10;
                S(actor, 0x3C4) = (signed short)(((int)(stat + 100) * 352) / 180);
                break;
            }
        }

        if ((unsigned int)dist < 0x40) {
            unsigned int spd;
            if (H(actor, 0x38E) != 1) {
                func_8005BA30(actor, 1, 0);
            }
            spd = W(actor, 0x3A0);
            W(actor, 0x3A0) = (spd < 0x13) ? 0 : (spd - 0x12);
            break;
        }

        {
            unsigned int eta_frames;
            unsigned int div_step = step_param & 0xFF;
            /* MIPS divu with break 7 trap if divisor == 0 */
            eta_frames = (unsigned int)dist / div_step;
            if (H(actor, 0x38E) == 4) {
                eta_frames += 4;
            }
            if (eta_frames < 0x29) {
                eta_frames = 0x28;
            }

            target_ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(SCRATCH, 0x36), S(SCRATCH, 0x66));
            if (eta_frames < 0x30) {
                func_80048D94(actor, ref_ang & 0xFF, target_ang & 0xFF, eta_frames & 0xFFFF);
            } else if (S(SCRATCH, 0x31C) < 0x10) {
                func_80048D94(actor, ref_ang & 0xFF, target_ang & 0xFF, eta_frames & 0xFFFF);
            } else if (H(actor, 0x38E) == 4) {
                func_800479B8(actor, 2);
            } else if ((unsigned int)dist < 0x400) {
                func_80048D94(actor, ref_ang & 0xFF, target_ang & 0xFF, eta_frames & 0xFFFF);
            } else {
                func_800479B8(actor, 2);
            }
        }
        break;
    }

    case 0x21: {
        H(actor, 0x02) += H(actor, 0x3CA);
        H(actor, 0x06) += H(actor, 0x3CE);
        func_80045504(actor, 3);

        if (B(actor, 0x390) == 5) {
            if ((func_8003D274(actor, 0, 0) & 0xFF) != 0) {
                int jitter = func_8005C7A0(6);
                unsigned int stat_lo = H(D_8010C32A, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) & 0xF;
                int kick_ang = B(actor, 0x3AA) + jitter;
                int r1 = func_8001E6B8(0x10 - stat_lo);
                unsigned int power = W(actor, 0x3A0) + r1 + 0xA;
                int r2 = func_8001E6B8(4);

                func_8004B0F0(actor, kick_ang & 0xFF, power, r2 - 2);
                B(SCRATCH, 0x2A9) = B(actor, 0x38D);
                B((unsigned char *)W(SCRATCH, 0x2F4), 0x396) = 4;
            }
        }

        if (B(actor, 0x390) == 0) {
            if (S(actor, 0x3C6) != 0xFF) {
                func_8005BA30(actor, 4, 4);
                B(actor, 0x397) = 0;
                B(actor, 0x396) = (unsigned char)H(actor, 0x3C6);
            } else if (B(actor, 0x38D) == B(SCRATCH, 0x2A9)) {
                ball = (unsigned char *)W(SCRATCH, 0x2F4);
                ball_ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(ball, 0x02), S(ball, 0x06));
                B(actor, 0x3AA) = func_8005B7CC(ball_ang & 0xFF, B(actor, 0x3AA), 0xC);
                func_8005BA30(actor, 4, 4);
                B(actor, 0x396) = 3;
                B(actor, 0x397) = 2;
            } else {
                func_8005BA30(actor, 0x26, 7);
                B(actor, 0x396) = 0x18;
                B(actor, 0x397) = 0;
            }
        }
        goto after_anim;
    }

    case 0x30: {
        int advance_substate = 0;

        if (B(actor, 0x399) != 0) {
            B(actor, 0x3D1) = (S(actor, 0x02) < 0) ? 2 : 1;
        } else {
            B(actor, 0x3D1) = (S(actor, 0x02) > 0) ? 2 : 1;
        }

        target_ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(SCRATCH, 0x36), S(SCRATCH, 0x66));
        if (B(SCRATCH, 0x2CF + B(actor, 0x398)) == 1) {
            advance_substate = 1;
        } else {
            func_8004840C(actor);
            if (W(SCRATCH, 0x2B0) != 4) {
                advance_substate = 1;
            } else {
                unsigned char ball_dir = B((unsigned char *)W(SCRATCH, 0x2F4), 0x3A4);
                unsigned int atk_heading = B(actor, 0x399) << 7;
                unsigned int diff = (atk_heading - ball_dir) & 0xFF;
                int ball_toward_goal = (diff < 0x81) ? (diff < 0x41) : (((ball_dir - atk_heading) & 0xFF) < 0x41);
                if (!ball_toward_goal) {
                    advance_substate = 1;
                }
            }
        }

        if (!advance_substate) {
            unsigned char ball_dir = B((unsigned char *)W(SCRATCH, 0x2F4), 0x3A4);
            unsigned int diff = (ball_dir - target_ang) & 0xFF;
            int angle_ok = (diff < 0x81) ? (diff < 0x60) : (((target_ang - ball_dir) & 0xFF) < 0x60);

            if (angle_ok) {
                signed short abs_pred_x = S(SCRATCH, 0xF8);
                signed short abs_x = S(actor, 0x02);
                if (abs_pred_x < 0) {
                    abs_pred_x = -abs_pred_x;
                }
                if (abs_x < 0) {
                    abs_x = -abs_x;
                }
                if (abs_pred_x >= abs_x + 0x400) {
                    advance_substate = 1;
                }
            }
        }

        if (advance_substate) {
            B(actor, 0x397)++;
        } else {
            unsigned char ball_dir = B((unsigned char *)W(SCRATCH, 0x2F4), 0x3A4);
            unsigned int diff = (ball_dir - target_ang) & 0xFF;
            int need_side_step = (diff < 0x81) ? (diff >= 0x60) : (((target_ang - ball_dir) & 0xFF) >= 0x60);

            if (need_side_step) {
                unsigned char cur_ball_dir = B((unsigned char *)W(SCRATCH, 0x2F4), 0x3A4);
                int sidestep_dir = (((target_ang - cur_ball_dir) & 0xFF) < 0x81)
                    ? (cur_ball_dir - 0x40)
                    : (cur_ball_dir + 0x40);
                unsigned char opp_dir = (cur_ball_dir + 0x80) & 0xFF;
                func_80048FC4(actor, sidestep_dir & 0xFF, opp_dir, 0xFF);
            } else {
                unsigned int spd;
                if (H(actor, 0x38E) != 1) {
                    func_8005BA30(actor, 1, 0);
                }
                spd = W(actor, 0x3A0);
                W(actor, 0x3A0) = (spd < 0x13) ? 0 : (spd - 0x12);
            }

            H(SCRATCH, 0xF8) = H(actor, 0x02);
            H(SCRATCH, 0xFC) = H(actor, 0x06);
            {
                unsigned int stat_lo = H(D_8010C32A, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) & 0xF;
                B(actor, 0x3D0) = (unsigned char)(func_8001E6B8(0xD - stat_lo) + 0xD);
            }
        }
        func_80045504(actor, 0x12);
        break;
    }

    case 0x31: {
        if (S(SCRATCH, 0x31C) < 9 && B(SCRATCH, 0x2A9) == 0) {
            unsigned int evt = W(SCRATCH, 0x2B0);
            if (evt == 4 || evt == 0xB || evt == 0xD) {
                func_80049F90(actor);
                break;
            }
        }

        {
            unsigned int hold_zero = (B(actor, 0x3D0) == 0) ? 1 : 0;
            signed short bx = S((unsigned char *)W(SCRATCH, 0x2F4), 0x02);
            int ball_in_atk_half = B(actor, 0x399) ? (bx >= 0) : (bx > 0);

            if (ball_in_atk_half) {
                func_8004840C(actor);
                func_800479B8(actor, hold_zero);
                if (W(SCRATCH, 0x2B0) == 4 && S(SCRATCH, 0x31C) == 0x14) {
                    /* UNCLEAR: uninit_actor_or_ptr ($s0) is read uninitialized on case 0x31 in the original binary */
                    func_800512B8(uninit_actor_or_ptr, 1);
                }
            } else {
                unsigned int stat = (W(D_8010C330, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) >> 11) & 0x7F;
                H(SCRATCH, 0xF2) -= (unsigned short)(((int)(stat + 100) * 320) / 180);
                func_80038F18(actor, 0, 0, 1);
                /* UNCLEAR: uninit_target_ang ($s2) is read uninitialized on case 0x31 in the original binary */
                if (func_800434D4(actor, uninit_target_ang & 0xFF) == 0) {
                    int dx = S(actor, 0x02) - S(SCRATCH, 0xF8);
                    int dz = S(actor, 0x06) - S(SCRATCH, 0xFC);
                    if (dx * dx + dz * dz <= 0x23FFFF &&
                        S(SCRATCH, 0x31C) >= 0x20 &&
                        S(SCRATCH, 0x5E) < -0x160) {
                        B(actor, 0x396) = 0x24;
                        B(actor, 0x397) = 0;
                    }
                }
            }

            if ((func_80053EB8(actor) & 0xFF) == 0) {
                signed short bx2 = S((unsigned char *)W(SCRATCH, 0x2F4), 0x02);
                int deep_own = B(actor, 0x399) ? (-bx2 < -0x198F) : (bx2 < -0x198F);
                B(actor, 0x396) = deep_own ? 0xF : 0x18;
                B(actor, 0x397) = 0;
            }
        }
        func_80045504(actor, 0x12);
        break;
    }

    case 0x40: {
        unsigned int owner;
        signed short abs_z, abs_x;
        int set_state7 = 0;

        B(actor, 0x3D1) = 0;
        owner = B(SCRATCH, 0x2A9);
        if (owner != 0) {
            if (B(actor, 0x38D) == owner) {
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 0;
                break;
            }
            target_actor = D_800FF748 + (owner % 24) * 1000;
            target_ang = B(target_actor, 0x3AB);
        } else {
            target_actor = (unsigned char *)W(SCRATCH, 0x2F4);
            target_ang = B(target_actor, 0x3A4);
        }

        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        ball_ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(ball, 0x02), S(ball, 0x06));
        ref_ang = (((ball_ang - target_ang) & 0xFF) < 0x81)
            ? (target_ang - 0x40)
            : (target_ang + 0x40);
        target_ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(target_actor, 0x02), S(target_actor, 0x06));
        func_80048D94(actor, ref_ang & 0xFF, target_ang & 0xFF, 0xFF);

        abs_z = S(actor, 0x06);
        if (abs_z < 0) {
            abs_z = -abs_z;
        }
        abs_x = S(actor, 0x02);
        if (abs_x < 0) {
            abs_x = -abs_x;
        }

        if (abs_z >= 0x2100 || abs_x >= 0x3320) {
            set_state7 = 1;
        } else if (B(SCRATCH, 0x2A9) == 0) {
            unsigned char ball_dir = B((unsigned char *)W(SCRATCH, 0x2F4), 0x3A4);
            unsigned int diff = (target_ang - ball_dir) & 0xFF;
            int facing_ball = (diff < 0x81) ? (diff < 0x31) : (((ball_dir - target_ang) & 0xFF) < 0x31);
            if (facing_ball) {
                set_state7 = 1;
            } else {
                unsigned int evt = W(SCRATCH, 0x2B0);
                if (evt == 0) {
                    set_state7 = 1;
                } else if (evt >= 4 && evt != 8) {
                    set_state7 = 1;
                }
            }
        } else {
            unsigned int target_state = B(target_actor, 0x396);
            /* Note: asm does lbu $a0, 0x396($s0); bltz $a0 (always false for u8) */
            if (target_state < 2 || target_state == 3) {
                set_state7 = 1;
            }
        }

        if (set_state7) {
            B(actor, 0x396) = 7;
            B(actor, 0x397) = 0;
        }
        break;
    }

    default:
        break;
    }

    func_8005B908(actor);
after_anim:
    func_800498AC(actor);
}
