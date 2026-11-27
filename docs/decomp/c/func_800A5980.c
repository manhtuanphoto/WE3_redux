/*
 * func_800A5980
 *
 * Purpose:
 *   Computes an AI player's adjusted target formation position (out_x, out_z)
 *   relative to the ball and game phase/possession side. Queries base target
 *   coordinates via func_8005A85C, shifts them forward (when mode_flag matches
 *   the actor's attack direction) or backward (when mode_flag matches the
 *   opposing direction) based on actor role (+0x391, 1..25) and ball x/z zone,
 *   clamps within pitch bounds, and enforces a ~9.5 m (~10-yard) standoff
 *   circle around the ball.
 *
 * Arguments:
 *   actor     - pointer to 1000-byte actor struct
 *   out_x     - output pointer for target x (s16)
 *   out_z     - output pointer for target z (s16)
 *   mode_flag - possession/side selector byte (0 or 1 applies shifts; other
 *               values skip both role switches and only apply final clamping)
 *
 * Return value:
 *   None (void)
 *
 * Actor fields read:
 *   +0x02 (s16) x, +0x06 (s16) z, +0x391 (u8) role/position index (1..25),
 *   +0x399 (u8) attack direction flag (0 = +x, 1 = -x),
 *   +0x3E3 (u8) override/special formation flag
 *
 * Actor fields written:
 *   None
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x2F4 (ptr) ball actor (+0x02 s16 ball x, +0x06 s16 ball z),
 *   +0x30C[(atk_dir ^ 1) * 2] (s16) defensive/offside x-line threshold
 *
 * Scratchpad fields written:
 *   None
 *
 * External data read:
 *   D_800E7000 (u8) global match/tactic mode byte
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x1D37 (7479) -> 29.916 m x-threshold; 0xE9C (3740) -> 14.96 m x-threshold
 *   0x110A (4362) -> 17.448 m x-threshold; 0x1990 (6544) -> 26.176 m x-threshold
 *   0x1081 (4225) -> 16.90 m z-threshold; 0xBC0/0xBC1 (3008/3009) -> 12.032 m x
 *   0x800..0x1B00 (2048..6912) -> 8.192 m..27.648 m longitudinal shifts
 *   0x27E2 (10210) -> 40.84 m; 0x200 (512) -> 2.048 m
 *   0x23E2 (9186) -> 36.744 m; 0x28F5 (10485) -> 41.94 m; 0x928 (2344) -> 9.376 m
 *   0x2A20 (10784) -> 43.136 m max x; 0x20D0 (8400) -> 33.6 m max z (touchline)
 *   0x500 (1280) -> 5.12 m min z-offset from ball
 *   0x53DC21 (5495841 = ~2344^2) -> ~9.38 m radius squared around ball
 *   0x948 (2376) -> 9.504 m (~10-yard) standoff radius around ball
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800E7000;

extern int func_8001E5A4(unsigned int angle, int scale);
extern int func_8001E694(unsigned int angle, int scale);
extern signed short func_8005A76C(int first, int second, signed short weight);
extern void func_8005A85C(unsigned char *actor, signed short *x, signed short *z);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);

void func_800A5980(unsigned char *actor, signed short *out_x, signed short *out_z, unsigned int mode_flag)
{
    unsigned char *scratch = Scratchpad;
    unsigned char *ball;
    signed short dir_sign;
    signed short init_ball_x;
    signed short target_x;
    signed short target_z;
    unsigned int flag;
    unsigned char atk_dir;
    int abs_val;
    int ball_x;
    int ball_z;
    int dx;
    int dz;
    unsigned int angle;

    dir_sign = (B(actor, 0x399) != 0) ? -1 : 1;
    init_ball_x = S((unsigned char *)W(scratch, 0x2F4), 0x02);
    func_8005A85C(actor, &target_x, &target_z);

    flag = mode_flag & 0xFF;
    atk_dir = B(actor, 0x399);
    if ((flag == 0 && atk_dir == 0) || (flag == 1 && atk_dir == 1)) {
        if (B(actor, 0x3E3) != 0) {
            target_x = (signed short)(dir_sign * 10210);
            target_z = (signed short)(target_z / 3);
        } else {
            switch (B(actor, 0x391)) {
            case 9:
            case 10:
            case 13:
                if (D_800E7000 >= 2) {
                    target_x = (signed short)(dir_sign * 2048);
                    goto after_first_switch;
                }
                /* Fall through. */
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
                ball = (unsigned char *)W(scratch, 0x2F4);
                ball_x = S(ball, 0x02);
                if ((atk_dir == 0 && ball_x > 0) || (atk_dir != 0 && ball_x < 0)) {
                    abs_val = (init_ball_x >= 0) ? init_ball_x : -init_ball_x;
                    if (abs_val >= 0x1D37) {
                        target_x = (signed short)(target_x + dir_sign * 6400);
                        target_z = (signed short)(target_z / 2);
                    } else if (abs_val >= 0xE9C) {
                        target_x = (signed short)(target_x + dir_sign * 5888);
                        target_z = (signed short)((target_z * 14) / 20);
                    } else {
                        target_x = (signed short)(target_x + dir_sign * 6144);
                        target_z = (signed short)((target_z * 18) / 20);
                    }
                    if (B(actor, 0x391) < 6) {
                        target_z = (signed short)((target_z * 16) / 20);
                    }
                } else {
                    abs_val = (ball_x >= 0) ? ball_x : -ball_x;
                    if (abs_val < 0x110A) {
                        target_x = (signed short)(target_x + dir_sign * 6144);
                    } else if (abs_val < 0x1990) {
                        target_x = (signed short)(target_x + dir_sign * 6656);
                    } else {
                        target_x = (signed short)(target_x + dir_sign * 6912);
                    }
                }
                ball = (unsigned char *)W(scratch, 0x2F4);
                ball_z = S(ball, 0x06);
                if (((ball_z >= 0) ? ball_z : -ball_z) >= 0x1081) {
                    target_z = func_8005A76C(target_z, ball_z, 1);
                }
                break;

            case 14:
            case 15:
            case 16:
                if (D_800E7000 != 0) {
                    target_x = 0;
                    goto after_first_switch;
                }
                /* Fall through. */
            case 11:
            case 12:
                if (D_800E7000 >= 2) {
                    target_x = (signed short)(dir_sign * 2048);
                    goto after_first_switch;
                }
                ball = (unsigned char *)W(scratch, 0x2F4);
                ball_x = S(ball, 0x02);
                if ((atk_dir == 0 && ball_x > 0) || (atk_dir != 0 && ball_x < 0)) {
                    target_x = (signed short)(target_x + dir_sign * 6144);
                    abs_val = (init_ball_x >= 0) ? init_ball_x : -init_ball_x;
                    if (abs_val >= 0x1D37) {
                        target_z = (signed short)((target_z * 12) / 20);
                    } else if (abs_val >= 0xE9C) {
                        target_z = (signed short)((target_z * 15) / 20);
                    } else {
                        target_z = (signed short)((target_z * 18) / 20);
                    }
                } else {
                    ball_z = S(ball, 0x06);
                    target_z = func_8005A76C(target_z, ball_z, 1);
                    ball_x = S((unsigned char *)W(scratch, 0x2F4), 0x02);
                    abs_val = (ball_x >= 0) ? ball_x : -ball_x;
                    if (abs_val < 0x110A) {
                        target_x = (signed short)(target_x + dir_sign * 5120);
                    } else if (abs_val < 0x1990) {
                        target_x = (signed short)(target_x + dir_sign * 4096);
                    } else {
                        target_x = (signed short)(target_x + dir_sign * 3840);
                    }
                }
                ball = (unsigned char *)W(scratch, 0x2F4);
                ball_z = S(ball, 0x06);
                if (((ball_z >= 0) ? ball_z : -ball_z) >= 0x1081) {
                    target_z = func_8005A76C(target_z, ball_z, 1);
                }
                break;

            case 19:
            case 20:
                ball = (unsigned char *)W(scratch, 0x2F4);
                ball_x = S(ball, 0x02);
                if ((atk_dir == 0 && ball_x > 0) || (atk_dir != 0 && ball_x < 0)) {
                    abs_val = (init_ball_x >= 0) ? init_ball_x : -init_ball_x;
                    if (abs_val >= 0x1D37) {
                        unsigned char role = B(actor, 0x391);
                        target_x = (signed short)(target_x + dir_sign * 5632);
                        if (role == 19 || role == 20) {
                            int actor_side_z = (S(actor, 0x06) >= 0) ? 1 : -1;
                            int ball_side_z = (S(ball, 0x06) >= 0) ? 1 : -1;
                            if (actor_side_z == ball_side_z) {
                                target_x = (signed short)(target_x + dir_sign * 2048);
                            }
                        }
                    } else if (abs_val >= 0xE9C) {
                        target_x = (signed short)(target_x + dir_sign * 5376);
                    } else {
                        target_x = (signed short)(target_x + dir_sign * 4608);
                    }
                } else {
                    abs_val = (ball_x >= 0) ? ball_x : -ball_x;
                    if (abs_val < 0x110A) {
                        target_x = (signed short)(target_x + dir_sign * 3328);
                    } else if (abs_val < 0x1990) {
                        target_x = (signed short)(target_x + dir_sign * 2816);
                    } else {
                        target_x = (signed short)(target_x + dir_sign * 2304);
                    }
                }
                break;

            case 17:
            case 18:
            case 21:
            case 22:
            case 23:
            case 24:
                ball = (unsigned char *)W(scratch, 0x2F4);
                ball_x = S(ball, 0x02);
                if ((atk_dir == 0 && ball_x > 0) || (atk_dir != 0 && ball_x < 0)) {
                    abs_val = (init_ball_x >= 0) ? init_ball_x : -init_ball_x;
                    if (abs_val >= 0x1D37) {
                        unsigned char role = B(actor, 0x391);
                        target_x = (signed short)(target_x + dir_sign * 3072);
                        if (role == 19 || role == 20) {
                            int actor_side_z = (S(actor, 0x06) >= 0) ? 1 : -1;
                            int ball_side_z = (S(ball, 0x06) >= 0) ? 1 : -1;
                            if (actor_side_z == ball_side_z) {
                                target_x = (signed short)(target_x + dir_sign * 2048);
                            }
                        }
                    } else if (abs_val >= 0xE9C) {
                        target_x = (signed short)(target_x + dir_sign * 3840);
                    } else {
                        target_x = (signed short)(target_x + dir_sign * 3072);
                    }
                } else {
                    abs_val = (ball_x >= 0) ? ball_x : -ball_x;
                    if (abs_val < 0x110A) {
                        target_x = (signed short)(target_x + dir_sign * 2816);
                    } else if (abs_val < 0x1990) {
                        target_x = (signed short)(target_x + dir_sign * 2560);
                    } else {
                        target_x = (signed short)(target_x + dir_sign * 2304);
                    }
                }
                ball = (unsigned char *)W(scratch, 0x2F4);
                ball_x = S(ball, 0x02);
                dx = S(actor, 0x02) - ball_x;
                dz = S(actor, 0x06) - S(ball, 0x06);
                if (dx * dx + dz * dz <= 0x53DC21) {
                    target_x = (signed short)((target_x + ball_x * 2) / 3);
                    ball = (unsigned char *)W(scratch, 0x2F4);
                    if (target_z - S(ball, 0x06) < 0) {
                        target_z = (signed short)(target_z - 0x200);
                    } else {
                        target_z = (signed short)(target_z + 0x200);
                    }
                }
                if (target_x * dir_sign > 0) {
                    target_x = 0;
                }
                break;

            default:
                break;
            }
        }
    }

after_first_switch:
    flag = mode_flag & 0xFF;
    atk_dir = B(actor, 0x399);
    if ((flag == 0 && atk_dir == 1) || (flag == 1 && atk_dir == 0)) {
        switch (B(actor, 0x391)) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            target_x = (signed short)(target_x - dir_sign * 2048);
            break;

        case 6:
        case 7:
        case 8:
            target_x = (signed short)(target_x - dir_sign * 2560);
            break;

        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
            ball = (unsigned char *)W(scratch, 0x2F4);
            ball_x = S(ball, 0x02);
            if ((atk_dir == 0 && ball_x < 0) || (atk_dir != 0 && ball_x > 0)) {
                abs_val = (init_ball_x >= 0) ? init_ball_x : -init_ball_x;
                if (abs_val >= 0x1D37) {
                    target_x = (signed short)(target_x - dir_sign * 4608);
                    target_z = (signed short)(target_z / 2);
                } else if (abs_val >= 0xE9C) {
                    target_x = (signed short)(target_x - dir_sign * 4096);
                    target_z = (signed short)((target_z * 11) / 20);
                } else {
                    target_x = (signed short)(target_x - dir_sign * 3584);
                    target_z = (signed short)((target_z * 13) / 20);
                }
            } else {
                abs_val = (ball_x >= 0) ? ball_x : -ball_x;
                if (abs_val < 0x110A) {
                    target_x = (signed short)(target_x - dir_sign * 3328);
                    target_z = (signed short)((target_z * 15) / 20);
                } else if (abs_val < 0x1990) {
                    target_x = (signed short)(target_x - dir_sign * 3072);
                } else {
                    target_x = (signed short)(target_x - dir_sign * 2816);
                }
            }
            break;

        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
            ball = (unsigned char *)W(scratch, 0x2F4);
            ball_x = S(ball, 0x02);
            if ((atk_dir == 0 && ball_x < -0xBC0) || (atk_dir != 0 && ball_x >= 0xBC1)) {
                signed short line_x = S(scratch, 0x30C + (atk_dir ^ 1) * 2);
                abs_val = (target_x >= 0) ? target_x : -target_x;
                if (abs_val < line_x) {
                    target_x = (signed short)(target_x - dir_sign * 1280);
                }
            }
            atk_dir = B(actor, 0x399);
            /* Fall through. */

        case 17:
        case 18:
        case 19:
        case 20:
            ball = (unsigned char *)W(scratch, 0x2F4);
            ball_x = S(ball, 0x02);
            if ((atk_dir == 0 && ball_x < 0) || (atk_dir != 0 && ball_x > 0)) {
                abs_val = (init_ball_x >= 0) ? init_ball_x : -init_ball_x;
                if (abs_val >= 0x1D37) {
                    target_x = (signed short)(target_x - dir_sign * 4096);
                    target_z = (signed short)((target_z * 8) / 20);
                } else if (abs_val >= 0xE9C) {
                    target_x = (signed short)(target_x - dir_sign * 3584);
                    target_z = (signed short)((target_z * 9) / 20);
                } else {
                    target_x = (signed short)(target_x - dir_sign * 3072);
                    target_z = (signed short)((target_z * 11) / 20);
                }
            } else {
                abs_val = (ball_x >= 0) ? ball_x : -ball_x;
                if (abs_val < 0x110A) {
                    target_x = (signed short)(target_x - dir_sign * 2816);
                    target_z = (signed short)((target_z * 13) / 20);
                } else if (abs_val < 0x1990) {
                    target_x = (signed short)(target_x - dir_sign * 2816);
                    target_z = (signed short)((target_z * 15) / 20);
                } else {
                    target_x = (signed short)(target_x - dir_sign * 2816);
                }
            }
            break;

        default:
            break;
        }

        if (((target_x >= 0) ? target_x : -target_x) >= 0x23E3) {
            target_x = (target_x >= 0) ? 0x23E2 : -0x23E2;
            ball_x = S((unsigned char *)W(scratch, 0x2F4), 0x02);
            dx = target_x - ball_x;
            if (((dx >= 0) ? dx : -dx) < 0x928) {
                target_x = (signed short)(ball_x - dir_sign * 0x928);
            } else {
                target_x = (target_x >= 0) ? 0x28F5 : -0x28F5;
            }
        }
    }

    if (((target_x >= 0) ? target_x : -target_x) >= 0x2A21) {
        target_x = (target_x > 0) ? 0x2A20 : -0x2A20;
    }
    if (((target_z >= 0) ? target_z : -target_z) >= 0x20D1) {
        target_z = (target_z > 0) ? 0x20D0 : -0x20D0;
    }

    ball_z = S((unsigned char *)W(scratch, 0x2F4), 0x06);
    dz = target_z - ball_z;
    if (((dz >= 0) ? dz : -dz) < 0x500) {
        target_z = (dz >= 0) ? (signed short)(ball_z + 0x500) : (signed short)(ball_z - 0x500);
    }

    ball = (unsigned char *)W(scratch, 0x2F4);
    ball_x = S(ball, 0x02);
    ball_z = S(ball, 0x06);
    dx = ball_x - target_x;
    dz = ball_z - target_z;
    if (dx * dx + dz * dz <= 0x53DC21) {
        angle = func_8005B36C(ball_x, ball_z, S(actor, 0x02), S(actor, 0x06)) & 0xFF;
        ball = (unsigned char *)W(scratch, 0x2F4);
        target_x = (signed short)(S(ball, 0x02) + func_8001E694(angle, 0x948));
        ball = (unsigned char *)W(scratch, 0x2F4);
        target_z = (signed short)(S(ball, 0x06) + func_8001E5A4(angle, 0x948));
    }

    *out_x = target_x;
    *out_z = target_z;
}
