/*
 * func_80076184
 *
 * Purpose:
 *   Advances one frame of instant replay playback: activates all 23 actors on
 *   the initial replay frame (D_800F0EE0), restores all 22 player actors'
 *   positions, heights, headings, mirror bits, and animations from the 5-slot
 *   replay buffer table (func_80188F78[Scratchpad[0x2E8] % 5]), restores the
 *   ball state, smoothly tracks the replay camera around the ball when within
 *   bounds, and advances or ends replay at D_8010C2A0 (wrapping modulo 120).
 *
 * Arguments:
 *   None (void).
 *
 * Return value:
 *   None (void).
 *
 * Actor fields read/written:
 *   Actors 0..22 (on start frame):
 *     Writes: +0x00 (= 1)
 *   Players 1..22 (loop i = 0..21):
 *     Reads:  +0x38E (anim id), +0x390 (anim frame), +0x3AA (body heading)
 *     Writes: +0x02 (x), +0x04 (y height), +0x06 (z), +0x26 (rotation),
 *             +0x38E (anim id), +0x390 (anim frame), +0x3A7 (= 1 for certain anims),
 *             +0x3AA (body heading), +0x3AC (mirror bit), +0x3AD (= 0 or 2)
 *   Actor 23 (ball/ref slot after loop):
 *     Passed to func_8005BA30(actor23, 0, 0)
 *   Ball actor (Scratchpad + 0x2F4):
 *     Reads:  +0x02 (x), +0x04 (y), +0x06 (z)
 *     Writes: +0x02 (x), +0x04 (y), +0x06 (z), +0x24 (= 0), +0x26/+0x28 (rotations)
 *
 * Scratchpad fields read/written:
 *   Reads:  0x1F0/0x1F4/0x1F8 (camera eye xyz), 0x1FC/0x200/0x204 (camera look-at xyz),
 *           0x288 (replay frame index), 0x294 (out-of-bounds frame counter),
 *           0x2E7 (camera yaw angle), 0x2E8 (replay buffer slot),
 *           0x2EA (camera distance), 0x2F4 (ball actor ptr)
 *   Writes: 0xDE (= 1), 0x1F0/0x1F4/0x1F8 (smoothed camera eye xyz),
 *           0x1FC/0x204 (camera look-at xz), 0x288 (next frame % 120),
 *           0x294 (out-of-bounds timer), 0x2E7 (camera yaw), 0x2EA (camera distance)
 *
 * Magic numbers / unit conversions (250 raw = 1 m, 30 frames/s):
 *   120 frames = 4.000 s replay circular buffer length
 *   6 frames   = 0.200 s out-of-bounds grace before ending replay
 *   0x1600 = 5632 raw (22.528 m minimum replay camera distance)
 *   0x3360 = 13152 raw (52.608 m goal-line |x| bound)
 *   0x35C5 = 13765 raw (55.060 m goal-depth |x| bound)
 *   0x392  = 914 raw (3.656 m goal-mouth |z| half-width)
 *   -0x266 = -614 raw (2.456 m; compared against |y| >= 0 so always false)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned short D_800F0EE0;
extern unsigned short D_8010C2A0;
extern unsigned char D_800CA578[];
extern unsigned char *func_80188F78[];
extern signed short D_800EF934;
extern unsigned short D_800EF936;
extern signed short D_800EF938;
extern signed short D_800F2B08;

extern void func_8004FB68(int arg0);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_800477A0(unsigned char *ball, int arg1);
extern void func_800989C8(unsigned char *ball, int arg1);
extern unsigned int func_800B1128(unsigned int val);
extern int func_8005B61C(int x1, int y1, int x2, int y2);
extern int func_8001E5A4(int angle, int scale);
extern int func_8001E694(int angle, int scale);
extern void func_800171DC(void);

void func_80076184(void)
{
    unsigned char *buf;
    unsigned char *actor;
    unsigned char *ball;
    int i;
    int f;
    int abs_x;
    int abs_z;
    int abs_y;

    func_8004FB68(0);

    if (D_800F0EE0 == (unsigned short)S(Scratchpad, 0x288)) {
        actor = D_800FF748;
        for (i = 0; i < 0x17; i++, actor += 0x3E8) {
            B(actor, 0x00) = 1;
        }
    }

    buf = func_80188F78[S(Scratchpad, 0x2E8) % 5];
    actor = D_800FF748 + 0x3E8;

    for (i = 0; i < 0x16; i++, actor += 0x3E8) {
        int j;
        unsigned char *height_rec;
        unsigned int anim_idx;
        unsigned int mirror_bits;
        int rot;

        f = S(Scratchpad, 0x288);
        H(actor, 0x38E) = B((unsigned char *)W(buf, 0x00), f * 22 + i);
        B(actor, 0x390) = B((unsigned char *)W(buf, 0x04), f * 22 + i);

        for (j = 0; j < 0x13; j++) {
            if (H(actor, 0x38E) == D_800CA578[j]) {
                H(actor, 0x38E) = 1;
                B(actor, 0x390) = 0;
            }
        }

        f = S(Scratchpad, 0x288);
        B(actor, 0x3AA) = B((unsigned char *)W(buf, 0x08), f * 22 + i);
        H(actor, 0x04) = 0;
        H(actor, 0x02) = H((unsigned char *)W(buf, 0x0C), (f * 22 + i) * 2);

        height_rec = (unsigned char *)W(buf, 0x10);
        if (S(height_rec, 4) != 0) {
            int k = 0;
            while (S(height_rec + k * 6, 4) != 0) {
                unsigned char *rec = height_rec + k * 6;
                if (S(rec, 0) == S(Scratchpad, 0x288) && S(rec, 2) == i) {
                    H(actor, 0x04) = H(rec, 4);
                }
                k++;
            }
        }

        f = S(Scratchpad, 0x288);
        H(actor, 0x06) = H((unsigned char *)W(buf, 0x14), (f * 22 + i) * 2);
        mirror_bits = W((unsigned char *)W(buf, 0x18), S(Scratchpad, 0x288) * 4);
        B(actor, 0x3AC) = (unsigned char)((mirror_bits >> (22 - i)) & 1);

        func_8005BA30(actor, B(actor, 0x38E), B(actor, 0x390));

        rot = 0x1C0 - B(actor, 0x3AA);
        S(actor, 0x26) = (signed short)((rot % 256) << 4);

        anim_idx = (unsigned int)(H(actor, 0x38E) - 1);
        if (anim_idx < 0x77U) {
            switch (anim_idx) {
            case 118:
                B(actor, 0x3A7) = 1;
                B(actor, 0x3AD) = 0;
                break;

            case 32:
            case 38:
            case 43:
            case 45:
            case 97:
            case 98:
            case 106:
            case 109:
            case 116:
                B(actor, 0x3AD) = 0;
                break;

            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 7:
            case 8:
            case 9:
            case 10:
            case 12:
            case 13:
            case 14:
            case 15:
            case 17:
            case 19:
            case 20:
            case 21:
            case 22:
            case 23:
            case 26:
            case 27:
            case 33:
            case 34:
            case 37:
            case 39:
            case 41:
            case 44:
            case 48:
            case 49:
            case 82:
            case 83:
            case 84:
            case 86:
            case 87:
            case 89:
            case 96:
            case 99:
            case 105:
                B(actor, 0x3A7) = 1;
                B(actor, 0x3AD) = 2;
                break;

            default:
                B(actor, 0x3AD) = 2;
                break;
            }
        } else {
            B(actor, 0x3AD) = 2;
        }
    }

    f = S(Scratchpad, 0x288);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    H(ball, 0x02) = H((unsigned char *)W(buf, 0x1C), f * 2);

    f = S(Scratchpad, 0x288);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    H(ball, 0x04) = H((unsigned char *)W(buf, 0x20), f * 2);

    f = S(Scratchpad, 0x288);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    H(ball, 0x06) = H((unsigned char *)W(buf, 0x24), f * 2);

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    H(ball, 0x24) = 0;

    f = S(Scratchpad, 0x288);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    H(ball, 0x26) = H((unsigned char *)W(buf, 0x28), f * 2);

    f = S(Scratchpad, 0x288);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    H(ball, 0x28) = H((unsigned char *)W(buf, 0x2C), f * 2);

    func_8005BA30(actor, 0, 0);
    func_800477A0((unsigned char *)W(Scratchpad, 0x2F4), 2);
    B(Scratchpad, 0xDE) = 1;
    func_800989C8((unsigned char *)W(Scratchpad, 0x2F4), 1);

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    abs_x = S(ball, 0x02);
    if (abs_x < 0) {
        abs_x = -abs_x;
    }

    if (abs_x >= 0x3360) {
        if (abs_x >= 0x35C5) {
            goto out_of_bounds;
        }
        abs_z = S(ball, 0x06);
        if (abs_z < 0) {
            abs_z = -abs_z;
        }
        if (abs_z >= 0x392) {
            goto out_of_bounds;
        }
        abs_y = S(ball, 0x04);
        if (abs_y < 0) {
            abs_y = -abs_y;
        }
        /* UNCLEAR: signed comparison of non-negative abs_y < -0x266 is always false */
        if (abs_y >= -0x266) {
            goto out_of_bounds;
        }
    }

    {
        int dx;
        int dz;
        unsigned int cam_dist;
        int pitch_ang;
        int yaw_ang;
        int sin_v;
        int cos_v;

        SW(Scratchpad, 0x1FC) = S(ball, 0x02);
        dx = D_800EF934 - SW(Scratchpad, 0x1FC);
        SW(Scratchpad, 0x204) = S(ball, 0x06);
        dz = D_800EF938 - SW(Scratchpad, 0x204);
        cam_dist = func_800B1128((unsigned int)(dx * dx + dz * dz));

        if ((int)(cam_dist - 0x1600) < D_800F2B08) {
            S(Scratchpad, 0x2EA) = 0x1600;
        } else {
            S(Scratchpad, 0x2EA) = (signed short)(cam_dist - D_800F2B08);
        }

        pitch_ang = func_8005B61C(0, (signed short)(SW(Scratchpad, 0x200) - (signed short)D_800EF936), (signed short)cam_dist, (signed short)D_800EF936);
        yaw_ang = func_8005B61C(D_800EF934, D_800EF938, (signed short)SW(Scratchpad, 0x1FC), (signed short)SW(Scratchpad, 0x204));
        B(Scratchpad, 0x2E7) = (unsigned char)yaw_ang;

        sin_v = func_8001E5A4(pitch_ang & 0xFF, D_800F2B08);
        SW(Scratchpad, 0x1F4) = (SW(Scratchpad, 0x1F4) * 95 + sin_v * 5) / 100;

        cos_v = func_8001E694(B(Scratchpad, 0x2E7), S(Scratchpad, 0x2EA));
        SW(Scratchpad, 0x1F0) = (SW(Scratchpad, 0x1F0) * 95 + (D_800EF934 + cos_v) * 5) / 100;

        sin_v = func_8001E5A4(B(Scratchpad, 0x2E7), S(Scratchpad, 0x2EA));
        SW(Scratchpad, 0x1F8) = (SW(Scratchpad, 0x1F8) * 95 + (D_800EF938 + sin_v) * 5) / 100;

        S(Scratchpad, 0x294) = 0;
        goto check_end;
    }

out_of_bounds:
    {
        unsigned short oob_cnt = H(Scratchpad, 0x294);
        H(Scratchpad, 0x294) = (unsigned short)(oob_cnt + 1);
        if (oob_cnt >= 6) {
            func_800171DC();
            return;
        }
    }

check_end:
    if (S(Scratchpad, 0x288) == (int)D_8010C2A0) {
        func_800171DC();
        return;
    }

    S(Scratchpad, 0x288) = (signed short)((S(Scratchpad, 0x288) + 1) % 120);
}
