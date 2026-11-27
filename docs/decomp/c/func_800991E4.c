/*
 * func_800991E4
 *
 * Purpose:
 *   Goal net collision and net-mesh deformation state machine (dispatched on
 *   SCRATCH+0xE0 net state: 0, 2, 4, 5, 10, 12, 13). Handles ball-in-goal
 *   back-net and side-net rebounds, velocity damping, goal-trigger flag
 *   (SCRATCH+0xDE), and net vertex displacement (D_800E6D90..D_800E6DFC).
 *
 * Arguments:
 *   actor (unsigned char *) - pointer to 1000-byte ball actor struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Actor fields (actor, and ball = *(u8**)(SCRATCH+0x2F4)):
 *     +0x002 (s16) x position (R/W)
 *     +0x004 (s16) y height, negative = up (R/W)
 *     +0x006 (s16) z position (R/W)
 *     +0x008 (s32) x velocity (R/W)
 *     +0x00C (s32) y velocity (R/W)
 *     +0x010 (s32) z velocity (R/W)
 *     +0x026 (s16/u16) rotation / spin (R/W)
 *     +0x3A0 (u32) speed << 8 (R/W)
 *     +0x3A4 (u8)  move heading, 0..255 = 360 deg (R/W)
 *   Scratchpad (0x1F800000):
 *     +0x0BE (s16) net x delta / velocity (R/W)
 *     +0x0C0 (s16) net y delta / velocity (W)
 *     +0x0C2 (s16) net z delta / velocity (R/W)
 *     +0x0C6 (s16) net x spring / rebound velocity (R/W)
 *     +0x0C8 (s16) net y spring / rebound velocity (W)
 *     +0x0CA (s16) net z spring / rebound velocity (R/W)
 *     +0x0D0 (s16) net timer/counter (W)
 *     +0x0DE (u8)  goal / net event flag (W)
 *     +0x0E0 (s16) net state machine index 0..13 (R/W)
 *     +0x29A (u8)  match phase, 1 = open play (R)
 *     +0x2F4 (u8*) pointer to ball actor (R)
 *   Globals:
 *     D_800CA70C, D_800CA70E, D_800CA710 (u16) default net anchor vertex xyz (R)
 *     D_800E6D90, D_800E6D92, D_800E6D94 (s16/u16) active net vertex 0 xyz (W)
 *     D_800E6DE8, D_800E6DEA, D_800E6DEC (s16) net vertex (top/roof) xyz (W)
 *     D_800E6DF0, D_800E6DF2, D_800E6DF4 (s16) net corner vertex A xyz (W)
 *     D_800E6DF8, D_800E6DFA, D_800E6DFC (s16) net corner vertex B xyz (W)
 *     D_800FF6A4 (u8) goal quadrant 1..4 (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x32E0 = 13024 raw (52.096 m) - goal entrance X threshold
 *   0x3320 = 13088 raw (52.352 m) - goal line X
 *   0x3360 = 13152 raw (52.608 m), 0x3365 = 13157 raw (52.628 m) - inner net X
 *   0x3500 = 13568 raw (54.272 m), 0x3585 = 13701 raw (54.804 m),
 *   0x35A5 = 13733 raw (54.932 m), 0x3625 = 13861 raw (55.444 m),
 *   0x3646 = 13894 raw (55.576 m), 0x3705 = 14085 raw (56.340 m) - back net X depths
 *   0x362  = 866 raw (3.464 m), 0x371 = 881 raw (3.524 m), 0x392 = 914 raw (3.656 m),
 *   0x3B2  = 946 raw (3.784 m), 0x3D2 = 978 raw (3.912 m), 0x3F2 = 1010 raw (4.040 m),
 *   0x492  = 1170 raw (4.680 m) - goal net half-width Z bounds
 *   -0x20  = -32 raw (0.128 m), -0x40 = -64 raw (0.256 m),
 *   -0x226 = -550 raw (2.200 m), -0x246 = -582 raw (2.328 m, crossbar),
 *   -0x25E = -606 raw (2.424 m), -0x29E = -670 raw (2.680 m, net roof) - heights
 *   0x9800 = 38912 (152 raw/frame << 8 = 18.24 m/s) - ball speed threshold
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define SCRATCH ((unsigned char*)0x1F800000)

extern unsigned short D_800CA70C;
extern unsigned short D_800CA70E;
extern unsigned short D_800CA710;
extern unsigned short D_800E6D90;
extern unsigned short D_800E6D92;
extern unsigned short D_800E6D94;
extern unsigned short D_800E6DE8;
extern unsigned short D_800E6DEA;
extern unsigned short D_800E6DEC;
extern unsigned short D_800E6DF0;
extern unsigned short D_800E6DF2;
extern unsigned short D_800E6DF4;
extern unsigned short D_800E6DF8;
extern unsigned short D_800E6DFA;
extern unsigned short D_800E6DFC;
extern unsigned char D_800FF6A4;

extern int func_8001E694(unsigned int angle, int scale);
extern int func_8001E5A4(unsigned int angle, int scale);
extern unsigned int func_8005B7CC(unsigned int cur, unsigned int target, unsigned int step);
extern void func_80098428(void);
extern void func_800989C8(unsigned char *actor, int flag);

void func_800991E4(unsigned char *actor)
{
    int state = S(SCRATCH, 0xE0);
    unsigned char *ball;
    int x, y, z;
    int abs_x, abs_z;

    if ((unsigned int)state >= 14U) {
        return;
    }

    switch (state) {
    case 0:
        if (B(SCRATCH, 0x29A) == 1) {
            B(SCRATCH, 0xDE) = 2;
        }
        return;

    case 2: {
        unsigned int ang = B(W(SCRATCH, 0x2F4), 0x3A4);
        unsigned int neg_ang = (-ang) & 0xFFU;
        int aligned;
        unsigned int spd;

        /* Check if ball heading is within +-31 angle units (+-43.6 deg) of 0 or 0x80 (along X) */
        if (neg_ang < 0x81U) {
            aligned = ((int)neg_ang < 0x20);
        } else {
            aligned = ((signed char)ang < 0x20);
        }
        if (!aligned) {
            ang = B(W(SCRATCH, 0x2F4), 0x3A4);
            neg_ang = (0x80U - ang) & 0xFFU;
            if (neg_ang < 0x81U) {
                aligned = ((int)neg_ang < 0x20);
            } else {
                aligned = ((int)((ang + 0x80U) & 0xFFU) < 0x20);
            }
        }
        if (aligned) {
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            if (W(ball, 0x3A0) > 0x9800U) {
                S(SCRATCH, 0xE0) = 0;
                return;
            }
        }

        D_800E6DEA = (unsigned short)(short)-0x226;
        S(actor, 4) = -0x246;
        x = S(W(SCRATCH, 0x2F4), 2);
        abs_x = (x < 0) ? -x : x;
        D_800E6DEC = (unsigned short)(0x3320 - abs_x);

        z = S(actor, 6);
        abs_z = (z < 0) ? -z : z;
        if (abs_z >= 0x372) {
            W(actor, 0x3A0) = 0;
        }

        spd = (W(actor, 0x3A0) * 13U) / 20U;
        W(actor, 0x3A0) = spd;
        if (spd == 0) {
            SW(W(SCRATCH, 0x2F4), 0xC) = 0;
        }

        if (S(actor, 2) > 0) {
            D_800E6DE8 = (unsigned short)(-S(actor, 6));
        } else {
            D_800E6DE8 = H(actor, 6);
        }

        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        x = S(ball, 2);
        abs_x = (x < 0) ? -x : x;
        if (abs_x < 0x3365) {
            S(ball, 2) = (x >= 0) ? 0x3365 : -0x3365;
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            x = S(ball, 2);
        }
        abs_x = (x < 0) ? -x : x;
        if (abs_x >= 0x35A6) {
            S(ball, 2) = (x >= 0) ? 0x35A5 : -0x35A5;
        }
        return;
    }

    case 4:
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        B(SCRATCH, 0xDE) = 0;
        if (S(ball, 4) < -0x25E) {
            S(ball, 4) = -0x25E;
        }
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        if (S(ball, 4) >= -0x20) {
            z = S(ball, 6);
            abs_z = (z < 0) ? -z : z;
            if (abs_z < 0x392) {
                unsigned int target_ang = (z > 0) ? 0x40U : 0xC0U;
                unsigned int new_ang;
                B(ball, 0x3A4) = (unsigned char)(-B(ball, 0x3A4));
                ball = (unsigned char *)W(SCRATCH, 0x2F4);
                new_ang = func_8005B7CC(B(ball, 0x3A4), target_ang, 0x10);
                ball = (unsigned char *)W(SCRATCH, 0x2F4);
                B(ball, 0x3A4) = (unsigned char)new_ang;
                ball = (unsigned char *)W(SCRATCH, 0x2F4);
                W(ball, 0x3A0) = W(ball, 0x3A0) / 10U;
                ball = (unsigned char *)W(SCRATCH, 0x2F4);
                SW(ball, 0xC) = SW(ball, 0xC) / 2;
            }
        }

        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        x = S(ball, 2);
        abs_x = (x < 0) ? -x : x;
        if (abs_x >= 0x3586) {
            S(ball, 2) = (x >= 0) ? 0x3585 : -0x3585;
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
        }
        z = S(ball, 6);
        abs_z = (z < 0) ? -z : z;
        if (abs_z < 0x362) {
            S(ball, 6) = (z >= 0) ? 0x362 : -0x362;
        }
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        x = S(ball, 2);
        abs_x = (x < 0) ? -x : x;
        if (abs_x < 0x3365) {
            S(ball, 2) = (x >= 0) ? 0x3365 : -0x3365;
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
        }

        z = S(ball, 6);
        abs_z = (z < 0) ? -z : z;
        if (abs_z < 0x3D2) {
            unsigned char q = D_800FF6A4;
            if (q == 1) {
                D_800E6DFC = (unsigned short)(0x3320 - H(actor, 2));
                D_800E6DF8 = H(actor, 6);
                D_800E6DFA = H(actor, 4);
            } else if (q == 2) {
                D_800E6DF4 = (unsigned short)(H(actor, 2) + 0x3320);
                D_800E6DF0 = (unsigned short)(-H(actor, 6));
                D_800E6DF2 = H(actor, 4);
            } else if (q == 3) {
                D_800E6DF4 = (unsigned short)(0x3320 - H(actor, 2));
                D_800E6DF0 = H(actor, 6);
                D_800E6DF2 = H(actor, 4);
            } else if (q == 4) {
                D_800E6DFC = (unsigned short)(H(actor, 2) + 0x3320);
                D_800E6DF8 = (unsigned short)(-H(actor, 6));
                D_800E6DFA = H(actor, 4);
            }
            return;
        }

        if (abs_z < 0x3D3) {
            x = S(ball, 2);
            abs_x = (x < 0) ? -x : x;
            if (abs_x < 0x3646) {
                return;
            }
        }
        func_80098428();
        return;

    case 5:
        if (S(actor, 2) > 0) {
            S(actor, 2) = 0x3500;
            z = S(actor, 6);
            abs_z = (z < 0) ? -z : z;
            if (abs_z >= 0x393) {
                S(actor, 6) = (z > 0) ? 0x392 : -0x392;
            }
        } else {
            S(actor, 2) = -0x3500;
            z = S(actor, 6);
            abs_z = (z < 0) ? -z : z;
            if (abs_z >= 0x393) {
                S(actor, 6) = (z > 0) ? 0x392 : -0x392;
            }
        }
        y = S(actor, 4);
        if (y < -0x40) {
            y += 5;
        } else {
            y = -0x40;
        }
        S(actor, 4) = (short)y;
        D_800E6D92 = H(actor, 4);
        if (S(actor, 2) > 0) {
            D_800E6D94 = (unsigned short)(0x3360 - S(actor, 2));
            D_800E6D90 = H(actor, 6);
        } else {
            D_800E6D94 = (unsigned short)(S(actor, 2) + 0x3360);
            D_800E6D90 = (unsigned short)(-H(actor, 6));
        }
        return;

    case 10: {
        int vx, vz, bounce;
        B(SCRATCH, 0xDE) = 1;
        x = S(actor, 2);
        abs_x = (x < 0) ? -x : x;
        if (abs_x < 0x32E0) {
            return;
        }
        z = S(actor, 6);
        abs_z = (z < 0) ? -z : z;
        if (abs_z >= 0x393) {
            /* Side net impact */
            S(SCRATCH, 0xE0) = 0xD;
            vx = func_8001E694(B(actor, 0x3A4), (int)(W(actor, 0x3A0) >> 8));
            SW(actor, 8) = vx;
            vz = func_8001E5A4(B(actor, 0x3A4), (int)(W(actor, 0x3A0) >> 8));
            SW(actor, 0x10) = vz;
            S(SCRATCH, 0xC0) = 0;
            S(SCRATCH, 0xBE) = (short)vx;
            S(SCRATCH, 0xC2) = (short)(vz / 3);
            S(SCRATCH, 0xC8) = 0;
            S(SCRATCH, 0xC6) = (short)(-vx);
            bounce = (-vz) / 2;
            bounce += (vz >= 0) ? -5 : 5;
            S(SCRATCH, 0xCA) = (short)bounce;
            W(actor, 0x3A0) = 0;
            func_800989C8(actor, 0);
            SW(actor, 0xC) = SW(actor, 0xC) / 2;
            return;
        }

        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        x = S(ball, 2);
        abs_x = (x < 0) ? -x : x;
        if (abs_x >= 0x35A6) {
            /* Back net impact */
            S(SCRATCH, 0xE0) = 0xC;
            vx = func_8001E694(B(actor, 0x3A4), (int)(W(actor, 0x3A0) >> 8));
            SW(actor, 8) = vx;
            vz = func_8001E5A4(B(actor, 0x3A4), (int)(W(actor, 0x3A0) >> 8));
            SW(actor, 0x10) = vz;
            S(SCRATCH, 0xC0) = 0;
            S(SCRATCH, 0xBE) = (short)(vx / 3);
            S(SCRATCH, 0xC2) = (short)vz;
            bounce = (-vx) / 2;
            bounce += (vx >= 0) ? -5 : 5;
            S(SCRATCH, 0xC6) = (short)bounce;
            S(SCRATCH, 0xC8) = 0;
            S(SCRATCH, 0xCA) = (short)(-vz);
            W(actor, 0x3A0) = 0;
            func_800989C8(actor, 0);
            SW(actor, 0xC) = SW(actor, 0xC) / 2;
            return;
        }

        if (S(actor, 4) < -0x29E) {
            S(actor, 4) = -0x29E;
            S(SCRATCH, 0xD0) = 0;
            S(SCRATCH, 0xC0) = 0;
            SW(actor, 0xC) = -SW(actor, 0xC);
        }
        return;
    }

    case 12:
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        x = S(ball, 2);
        abs_x = (x < 0) ? -x : x;
        if (abs_x >= 0x3321) {
            z = S(ball, 6);
            abs_z = (z < 0) ? -z : z;
            if (abs_z >= 0x372) {
                S(ball, 6) = (z >= 0) ? 0x371 : -0x371;
            }
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            x = S(ball, 2);
            abs_x = (x < 0) ? -x : x;
            if (abs_x < 0x3626) {
                S(ball, 2) = (x >= 0) ? 0x3625 : -0x3625;
            } else {
                int c2, c6, be;
                S(actor, 0x26) = (short)(S(actor, 0x26) + S(SCRATCH, 0xC2));
                c2 = (S(SCRATCH, 0xC2) * 8) / 10;
                S(SCRATCH, 0xC2) = (short)c2;
                c6 = S(SCRATCH, 0xC6);
                S(SCRATCH, 0xBE) = (short)(S(SCRATCH, 0xBE) + c6 / 25);
                ball = (unsigned char *)W(SCRATCH, 0x2F4);
                if (S(ball, 4) >= -0x3F) {
                    c6 = (S(SCRATCH, 0xC6) * 18) / 20;
                    S(SCRATCH, 0xC6) = (short)c6;
                    be = (S(SCRATCH, 0xBE) * 18) / 20;
                    S(SCRATCH, 0xBE) = (short)be;
                }
            }
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            if (S(ball, 4) < -0x29E) {
                S(ball, 4) = -0x29E;
                SW(actor, 0xC) = -SW(actor, 0xC);
            }
        }

        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        S(ball, 2) = (short)(S(ball, 2) + S(SCRATCH, 0xBE));
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        S(ball, 6) = (short)(S(ball, 6) + S(SCRATCH, 0xC2));
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        x = S(ball, 2);
        abs_x = (x < 0) ? -x : x;
        if (abs_x >= 0x3706) {
            S(ball, 2) = (x >= 0) ? 0x3705 : -0x3705;
        }
        if (H(SCRATCH, 0xBE) == 0 && H(SCRATCH, 0xC2) == 0) {
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            if (S(ball, 4) >= -0x20) {
                SW(ball, 0xC) = (SW(ball, 0xC) * 8) / 10;
            }
        }
        func_800989C8(actor, 0);
        return;

    case 13: {
        int be_val, ca_val;
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        z = S(ball, 6);
        abs_z = (z < 0) ? -z : z;
        if (abs_z >= 0x3B2) {
            be_val = (S(SCRATCH, 0xBE) * 7) / 10;
            S(SCRATCH, 0xBE) = (short)be_val;
            S(actor, 0x26) = (short)(S(actor, 0x26) + be_val);
            ca_val = S(SCRATCH, 0xCA);
            S(SCRATCH, 0xC2) = (short)(S(SCRATCH, 0xC2) + ca_val / 10);
        }
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        if (S(ball, 4) < -0x29E) {
            S(ball, 4) = -0x29E;
            SW(actor, 0xC) = -SW(actor, 0xC);
        }
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        z = S(ball, 6);
        abs_z = (z < 0) ? -z : z;
        if (abs_z < 0x3F2) {
            S(ball, 6) = (z >= 0) ? 0x3F2 : -0x3F2;
            H(SCRATCH, 0xC2) = 0;
            H(SCRATCH, 0xCA) = 0;
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
        }
        S(ball, 2) = (short)(S(ball, 2) + S(SCRATCH, 0xBE));
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        S(ball, 6) = (short)(S(ball, 6) + S(SCRATCH, 0xC2));
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        z = S(ball, 6);
        abs_z = (z < 0) ? -z : z;
        if (abs_z >= 0x493) {
            S(ball, 6) = (z >= 0) ? 0x492 : -0x492;
        }
        if (H(SCRATCH, 0xBE) == 0 && H(SCRATCH, 0xC2) == 0) {
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            if (S(ball, 4) >= -0x20) {
                SW(ball, 0xC) = (SW(ball, 0xC) * 8) / 10;
            }
        }
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        x = S(ball, 2);
        abs_x = (x < 0) ? -x : x;
        if (abs_x < 0x3365) {
            S(ball, 2) = (x >= 0) ? 0x3365 : -0x3365;
            ball = (unsigned char *)W(SCRATCH, 0x2F4);
            x = S(ball, 2);
        }
        abs_x = (x < 0) ? -x : x;
        if (abs_x >= 0x35A6) {
            S(ball, 2) = (x >= 0) ? 0x35A5 : -0x35A5;
        }
        D_800E6D90 = D_800CA70C;
        D_800E6D92 = D_800CA70E;
        D_800E6D94 = D_800CA710;
        func_800989C8(actor, 0);
        return;
    }

    case 1:
    case 3:
    case 6:
    case 7:
    case 8:
    case 9:
    case 11:
    default:
        return;
    }
}
