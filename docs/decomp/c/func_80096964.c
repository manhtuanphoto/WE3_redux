/*
 * func_80096964 - Foul / card cutscene camera state machine (state 0x2F0: 0..4).
 *
 * Purpose:
 *   Drives the camera look-at target (0x1FC/0x200/0x204) and camera eye position
 *   (0x1F0/0x1F4/0x1F8) during a foul/card sequence focused on the offending actor
 *   (scratch 0x2AD):
 *   - Case 0: Resets sub-timer (0x2E8) and advances state 0x2F0 -> 1.
 *   - Case 1: Smoothly tracks the ball (90/10 EMA) for 24 frames (0.80 s) or while
 *             offender is in anim 0x21, then picks a random camera vantage point
 *             (D_800EF934/36/38) and camera distance (D_800F2B08), sets 0x200 = -0x120,
 *             and advances state 0x2F0 -> 2.
 *   - Case 2: Computes initial camera eye position & elevation from offender position
 *             and vantage point, picks random camera mode (0x2F2 = rand%3), and
 *             advances state 0x2F0 -> 3.
 *   - Case 3: Smoothly tracks offender (97/3 EMA), handles special contact flag
 *             (actor +0x3C8 == 4 -> close-up jump & advance to 4; == 2 -> timeout
 *             at 129 frames = 4.30 s -> func_800171FC(3)), and aborts if ball and
 *             offender are on opposite halves with |offender.x| >= 0x110B (17.45 m).
 *   - Case 4: Locks camera eye offset from offender position using 0x2E7/0x2EA.
 *
 * Arguments:
 *   None (reads offender actor ID from scratch 0x1F8002AD).
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F8001F0/0x1F4/0x1F8 (s32, R/W): camera eye X/Y/Z.
 *   - 0x1F8001FC/0x200/0x204 (s32, R/W): camera look-at target X/Y/Z.
 *   - 0x1F8002AD (u8, R): offending actor ID (indexed into D_800FF748 via id % 24).
 *   - 0x1F8002E3 (u8, R): camera height/mode selector (1 or 5 -> -0x300, else -0x2E0).
 *   - 0x1F8002E7 (u8, R/W): camera orbit azimuth angle (0..255).
 *   - 0x1F8002E8 (s16, R/W): cutscene frame timer.
 *   - 0x1F8002EA (s16, R/W): camera horizontal orbit radius.
 *   - 0x1F8002F0 (u8, R/W): camera sequence state (0..4).
 *   - 0x1F8002F2 (u8, R/W): camera variation flag (0..2).
 *   - 0x1F8002F4 (u8*, R): pointer to ball actor.
 *
 * Actor fields reads/writes:
 *   - offender: +0x02/+0x06 (s16 x/z, R), +0x38E (u16 anim id, R),
 *               +0x3AA (u8 body heading, R), +0x3C8 (s16 contact/card flag, R).
 *   - ball:     +0x02/+0x04/+0x06 (s16 x/y/z, R).
 *
 * Globals reads/writes:
 *   - D_800FF748[]: 24-element array of 1000-byte actor structs.
 *   - D_800EF934/D_800EF936/D_800EF938 (s16, R/W): camera vantage X/Y/Z.
 *   - D_800F2B08 (s16, R/W): camera distance parameter.
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x18 (24 frames) = 0.80 s; 0x81 (129 frames) = 4.30 s.
 *   - 0x3320 (13088) = 52.35 m (goal line X); 0x3FE8 (16360) = 65.44 m;
 *     0x4200 (16896) = 67.58 m; 0x5535 (21813) = 87.25 m.
 *   - 0x1600 (5632) = 22.53 m; 0xD33 (3379) = 13.52 m; 0x1080 (4224) = 16.90 m.
 *   - -0x120 (-288) = 1.15 m above ground; -0x2C0 (-704) = 2.82 m;
 *     -0x2E0 (-736) = 2.94 m; -0x300 (-768) = 3.07 m; -0x380 (-896) = 3.58 m.
 *   - 0x110B (4363) = 17.45 m (half-pitch separation check).
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];
extern signed short D_800EF934;
extern signed short D_800EF936;
extern signed short D_800EF938;
extern signed short D_800F2B08;

extern int func_8001E6B8(int n);
extern unsigned int func_800B1128(int val);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8005B61C(int x1, int z1, int x2, int z2);
extern int func_8001E5A4(int angle, int scale);
extern int func_8001E694(int angle, int scale);
extern void func_800171FC(int mode);

void func_80096964(void)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *actor;
    unsigned char state;

    actor = D_800FF748 + (B(scratch, 0x2AD) % 24) * 1000;
    state = B(scratch, 0x2F0);
    if (state >= 5) {
        return;
    }

    switch (state) {
    case 0:
        S(scratch, 0x2E8) = 0;
        B(scratch, 0x2F0) = B(scratch, 0x2F0) + 1;
        break;

    case 1:
        if (S(scratch, 0x2E8) >= 0x18 && H(actor, 0x38E) != 0x21) {
            int mode = func_8001E6B8(3);
            int val;

            switch (mode) {
            case 0:
                D_800EF934 = 0;
                if (func_8001E6B8(2) == 0) {
                    val = -0x4200;
                } else {
                    val = 0x4200;
                }
                D_800EF938 = (signed short)val;
                break;

            case 1:
                if (func_8001E6B8(2) != 0) {
                    val = (S(actor, 2) <= 0) ? -0x5535 : 0x5535;
                } else {
                    val = (S(actor, 2) <= 0) ? 0x3320 : -0x3320;
                }
                D_800EF934 = (signed short)val;
                D_800EF938 = 0;
                break;

            case 2:
                D_800EF934 = (S(actor, 2) <= 0) ? -0x3FE8 : 0x3FE8;
                val = (S(actor, 6) <= 0) ? -0x4200 : 0x4200;
                D_800EF938 = (signed short)val;
                break;
            }

            if (D_800EF938 == 0 && (int)S(actor, 2) * (int)D_800EF934 > 0) {
                int step;
                D_800F2B08 = (signed short)((func_8001E6B8(0x18) << 8) + 0x1600);
                step = ((int)D_800F2B08 - 0xD33) / 256;
                val = -0x380 - (step << 4);
            } else {
                int s0_off;
                int r2;
                int mul_off;
                int base_y;
                unsigned char cam_mode;

                D_800F2B08 = (signed short)((func_8001E6B8(0x10) << 8) + 0xD33);
                s0_off = (((int)D_800F2B08 - 0xD33) / 256) << 4;
                r2 = func_8001E6B8(2);
                mul_off = ((((int)D_800F2B08 - 0xD33) / 256) << 4) * r2;
                cam_mode = B(scratch, 0x2E3);
                if (cam_mode == 1 || cam_mode == 5) {
                    base_y = -0x300;
                } else {
                    base_y = -0x2E0;
                }
                val = base_y - mul_off - s0_off;
            }

            D_800EF936 = (signed short)val;
            SW(scratch, 0x200) = -0x120;
            B(scratch, 0x2F0) = B(scratch, 0x2F0) + 1;
        } else {
            unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
            SW(scratch, 0x1FC) = (SW(scratch, 0x1FC) * 9 + S(ball, 2)) / 10;
            SW(scratch, 0x200) = (SW(scratch, 0x200) * 9 + S(ball, 4)) / 10;
            SW(scratch, 0x204) = (SW(scratch, 0x204) * 9 + S(ball, 6)) / 10;
            S(scratch, 0x2E8) = (signed short)(S(scratch, 0x2E8) + 1);
        }
        break;

    case 2:
    {
        int dx, dz;
        unsigned int dist;
        int elev_ang, horiz_ang;
        int radius;

        SW(scratch, 0x1FC) = S(actor, 2);
        SW(scratch, 0x204) = S(actor, 6);
        dx = D_800EF934 - SW(scratch, 0x1FC);
        dz = D_800EF938 - SW(scratch, 0x204);
        dist = func_800B1128(dx * dx + dz * dz);

        elev_ang = func_8005B36C(
            0,
            (signed short)(SW(scratch, 0x200) - D_800EF936),
            (signed short)dist,
            D_800EF936
        );
        SW(scratch, 0x1F4) = func_8001E5A4(elev_ang & 0xFF, D_800F2B08);

        if ((int)dist - 0x1600 < (int)D_800F2B08) {
            radius = 0x1600;
        } else {
            radius = (int)dist - (int)D_800F2B08;
        }
        S(scratch, 0x2EA) = (signed short)radius;

        horiz_ang = func_8005B36C(
            D_800EF934,
            D_800EF938,
            (signed short)SW(scratch, 0x1FC),
            (signed short)SW(scratch, 0x204)
        );
        B(scratch, 0x2E7) = (unsigned char)horiz_ang;

        SW(scratch, 0x1F0) = D_800EF934 + func_8001E694(horiz_ang & 0xFF, S(scratch, 0x2EA));
        SW(scratch, 0x1F8) = D_800EF938 + func_8001E5A4(horiz_ang & 0xFF, S(scratch, 0x2EA));
        B(scratch, 0x2F2) = (unsigned char)func_8001E6B8(3);
        S(scratch, 0x2E8) = 0;
        B(scratch, 0x2F0) = B(scratch, 0x2F0) + 1;
        break;
    }

    case 3:
    {
        int dx, dz;
        unsigned int dist;
        int flag = 0;
        int radius;
        int horiz_ang;
        int heading_diff;
        signed short contact_flag;
        unsigned char *ball;

        SW(scratch, 0x1FC) = S(actor, 2);
        SW(scratch, 0x204) = S(actor, 6);
        dx = D_800EF934 - SW(scratch, 0x1FC);
        dz = D_800EF938 - SW(scratch, 0x204);
        dist = func_800B1128(dx * dx + dz * dz);

        if (B(scratch, 0x2F2) == 0 && S(actor, 0x3C8) != 0) {
            int d = (int)B(actor, 0x3AA) - (int)B(scratch, 0x2E7);
            if (d < 0) {
                d = -d;
            }
            if (d < 0x40) {
                flag = (D_800F2B08 >= 0xD34) ? 1 : 0;
            }
        }

        if ((int)dist - 0x1600 < (int)D_800F2B08) {
            radius = 0x1600;
        } else {
            radius = (int)dist - (int)D_800F2B08;
        }
        S(scratch, 0x2EA) = (signed short)radius;

        horiz_ang = func_8005B61C(
            D_800EF934,
            D_800EF938,
            (signed short)SW(scratch, 0x1FC),
            (signed short)SW(scratch, 0x204)
        );
        B(scratch, 0x2E7) = (unsigned char)horiz_ang;

        heading_diff = (int)B(actor, 0x3AA) - (horiz_ang & 0xFF);
        if (heading_diff < 0) {
            heading_diff = -heading_diff;
        }
        if (heading_diff >= 0x40 || B(scratch, 0x2F2) != 0 || (flag & 0xFF) != 0) {
            int elev_ang = func_8005B61C(
                0,
                (signed short)(SW(scratch, 0x200) - D_800EF936),
                (signed short)dist,
                D_800EF936
            );
            int target_y = func_8001E5A4(elev_ang & 0xFF, D_800F2B08);
            int target_x;
            int target_z;

            SW(scratch, 0x1F4) = (SW(scratch, 0x1F4) * 97 + target_y * 3) / 100;
            target_x = D_800EF934 + func_8001E694(B(scratch, 0x2E7), S(scratch, 0x2EA));
            SW(scratch, 0x1F0) = (SW(scratch, 0x1F0) * 97 + target_x * 3) / 100;
            target_z = D_800EF938 + func_8001E5A4(B(scratch, 0x2E7), S(scratch, 0x2EA));
            SW(scratch, 0x1F8) = (SW(scratch, 0x1F8) * 97 + target_z * 3) / 100;
        }

        contact_flag = S(actor, 0x3C8);
        if (contact_flag == 2) {
            signed short timer = S(scratch, 0x2E8);
            S(scratch, 0x2E8) = (signed short)(timer + 1);
            if (timer >= 0x81) {
                func_800171FC(3);
            }
        } else if (contact_flag == 4) {
            int do_jump = 1;
            if (D_800EF934 == 0) {
                int d = (int)B(actor, 0x3AA) - (int)B(scratch, 0x2E7);
                if (d < 0) {
                    d = -d;
                }
                if (d >= 0x80) {
                    do_jump = 0;
                }
            }
            if (do_jump) {
                unsigned char ang = (S(actor, 6) <= 0) ? 0xC0 : 0x40;
                int rand_r;
                int rand_y;
                B(scratch, 0x2E7) = ang;
                rand_r = (func_8001E6B8(8) << 8) + 0x1080;
                S(scratch, 0x2EA) = (signed short)rand_r;
                rand_y = (func_8001E6B8(10) << 4) - 0x2C0;
                SW(scratch, 0x1F4) = rand_y;
                SW(scratch, 0x1FC) = S(actor, 2);
                SW(scratch, 0x204) = S(actor, 6);
                SW(scratch, 0x1F0) = SW(scratch, 0x1FC) + func_8001E694(B(scratch, 0x2E7), S(scratch, 0x2EA));
                SW(scratch, 0x1F8) = SW(scratch, 0x204) + func_8001E5A4(B(scratch, 0x2E7), S(scratch, 0x2EA));
                B(scratch, 0x2F0) = B(scratch, 0x2F0) + 1;
            }
        }

        ball = (unsigned char *)W(scratch, 0x2F4);
        if ((int)S(ball, 2) * (int)S(actor, 2) < 0) {
            int abs_x = S(actor, 2);
            if (abs_x < 0) {
                abs_x = -abs_x;
            }
            if (abs_x >= 0x110B) {
                func_800171FC(3);
            }
        }
        break;
    }

    case 4:
        SW(scratch, 0x1FC) = S(actor, 2);
        SW(scratch, 0x204) = S(actor, 6);
        SW(scratch, 0x1F0) = SW(scratch, 0x1FC) + func_8001E694(B(scratch, 0x2E7), S(scratch, 0x2EA));
        SW(scratch, 0x1F8) = SW(scratch, 0x204) + func_8001E5A4(B(scratch, 0x2E7), S(scratch, 0x2EA));
        break;
    }
}
