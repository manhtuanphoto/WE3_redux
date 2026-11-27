/*
 * func_8009462C
 *
 * Purpose:
 *   Tests whether a player actor's body/limbs (queried from D_800CA6A0/D_800CA6A1
 *   via func_8005C1CC) intersect the ball at either its midpoint or frame-0
 *   predicted position, and if hit, snaps the ball outside the contact sphere,
 *   computes deflection heading/speed/lift, and invokes func_800951C4.
 *
 * Arguments:
 *   actor   (unsigned char *): pointer to 1000-byte player actor struct.
 *   mode    (int): number of body-part entries to test (1-based count).
 *   heading (int): base deflection heading byte (0..255).
 *   pitch   (int): vertical/deflection pitch parameter.
 *   lift    (int): ball lift parameter (passed to func_800951C4).
 *
 * Return value:
 *   int: 1 if ball deflection occurred, 0 otherwise.
 *
 * Scratchpad (0x1F800000) reads/writes:
 *   - 0x036/0x04E/0x066 (s16, R): ball predicted xyz at frame 0.
 *   - 0x0F0/0x0F2/0x0F4 (s16, R): limb/joint world xyz set by func_8005C1CC.
 *   - 0x29A (u8, R): match phase (0xC disables deflection).
 *   - 0x2A8 (u8, R): last-touch actor ID (1 or 0xC disables deflection).
 *   - 0x2A9 (u8, R): current ball owner (0 = free).
 *   - 0x2AA (u8, R): previous ball owner.
 *   - 0x2AC (u8, R): shot/deflection actor ID check.
 *   - 0x2B0 (u32, R/W): ball event code (set to 0xB on deflection).
 *   - 0x2B4 (s16, W): set to 0x7FFF sentinel.
 *   - 0x2CF[2] (u8, R/W): team possession flags (set to 1 for actor's side).
 *   - 0x2F4 (unsigned char *, R): pointer to ball actor.
 *   - 0x31C (s16, R/W): ball timer (frames since last touch).
 *
 * Actor fields reads:
 *   - +0x38D (u8, R): actor ID.
 *   - +0x396 (u8, R): actor state (0xC = intercept/block).
 *   - +0x398 (u8, R): side (0/1).
 *   - +0x399 (u8, R): attack direction flag (0/1).
 *   - +0x3A4 (u8, R): move heading.
 *   - +0x3CC (s16, R): block sub-mode (2 triggers special heading blend).
 *
 * Ball actor (+0x2F4) reads/writes:
 *   - +0x02/+0x04/+0x06 (s16, R/W): ball xyz.
 *   - +0x0C (s32, R): ball vertical velocity vy (scaled by 736).
 *   - +0x3A0 (u32, R/W): ball speed.
 *   - +0x3A4 (u8, R): ball move heading.
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - -0x20 (-32): -0.128 m radius penalty if actor == previous ball owner (0x2AA).
 *   - 0x9 / 0x5 / 0x18: 9 frames (0.30 s), 5 frames (0.17 s), 24 frames (0.80 s) cooldowns.
 *   - 0x40 / 0xC0 (64 / 192): +/-90 deg lateral deflection targets in event 0xD.
 *   - 736: ball vy scale factor (from func_8004B0F0: vy = -lift * 736).
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800CA6A0[];
extern unsigned char D_800CA6A1[];

extern int func_8005CE14(int actor_id);
extern void func_8005C1CC(unsigned char *actor, int joint_id);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8001E694(int angle, int scale);
extern int func_8001E5A4(int angle, int scale);
extern int func_8005B7CC(int target, int cur, int step);
extern int func_8001E6B8(int n);
extern int func_8005C7A0(int n);
extern void func_800951C4(unsigned char *actor, int heading, int lift, int arg3);

int func_8009462C(unsigned char *actor, int mode, int heading, int pitch, int lift)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char last_touch;
    unsigned char actor_id;
    int radius_adj;
    int pass;
    int test_x, test_y, test_z;
    unsigned char *ball;
    int hit_dir;
    int joint_radius;
    int out_heading;
    int angle_diff;
    int blend_pitch;
    int jitter;
    int final_heading;
    unsigned int prev_event;

    if (B(scratch, 0x29A) == 0xC) {
        return 0;
    }

    last_touch = B(scratch, 0x2A8);
    if (last_touch == 1 || last_touch == 0xC) {
        return 0;
    }

    actor_id = B(actor, 0x38D);
    if (actor_id == B(scratch, 0x2AA) && S(scratch, 0x31C) < 9) {
        return 0;
    }

    if (B(scratch, 0x2A9) != 0 && B(scratch, 0x2CF + B(actor, 0x398)) == 0) {
        return 0;
    }

    if (W(scratch, 0x2B0) == 0xB && S(scratch, 0x31C) < 5) {
        return 0;
    }

    if (actor_id == (func_8005CE14(actor_id) & 0xFF) &&
        B(scratch, 0x2CF + B(actor, 0x398)) == 1 &&
        S(scratch, 0x31C) < 0x18 &&
        actor_id == B(scratch, 0x2AA)) {
        radius_adj = 0;
    } else {
        radius_adj = (B(actor, 0x38D) == B(scratch, 0x2AA)) ? -0x20 : 0;
    }

    ball = (unsigned char *)W(scratch, 0x2F4);
    test_x = (int)((unsigned int)(S(ball, 2) + S(scratch, 0x36)) >> 1);
    test_y = (int)((unsigned int)(S(ball, 4) + S(scratch, 0x4E)) >> 1);
    test_z = (int)((unsigned int)(S(ball, 6) + S(scratch, 0x66)) >> 1);

    for (pass = 0; pass < 2; pass++) {
        int idx = (mode & 0xFF) - 1;
        int sx = (signed short)test_x;
        int sz = (signed short)test_z;
        int sy = (signed short)test_y;
        int offset = idx * 2;

        while (offset >= 0) {
            int jx, jz, dx, dz, dy;
            int radius;

            func_8005C1CC(actor, D_800CA6A0[offset]);
            jx = S(scratch, 0xF0);
            jz = S(scratch, 0xF4);
            dx = jx - sx;
            dz = jz - sz;
            radius = D_800CA6A1[offset] + radius_adj;

            if (dx * dx + dz * dz < radius * radius) {
                dy = sy - S(scratch, 0xF2);
                if (dy < 0) dy = -dy;
                if (dy < radius) {
                    hit_dir = func_8005B36C(jx, jz, sx, sz);
                    joint_radius = D_800CA6A1[offset];
                    goto on_hit;
                }
            }
            offset -= 2;
        }

        test_x = H(scratch, 0x36);
        test_y = H(scratch, 0x4E);
        test_z = H(scratch, 0x66);
    }

    return 0;

on_hit:
    {
        unsigned char *b;
        int cx = func_8001E694(hit_dir & 0xFF, joint_radius);
        int cz;

        b = (unsigned char *)W(scratch, 0x2F4);
        S(b, 2) = S(scratch, 0xF0) + cx;
        b = (unsigned char *)W(scratch, 0x2F4);
        H(b, 4) = H(scratch, 0x4E);
        cz = func_8001E5A4(hit_dir & 0xFF, joint_radius);
        b = (unsigned char *)W(scratch, 0x2F4);
        S(b, 6) = S(scratch, 0xF4) + cz;
    }

    if (B(actor, 0x396) == 0xC && S(actor, 0x3CC) == 2) {
        unsigned char *b = (unsigned char *)W(scratch, 0x2F4);
        int ball_dir = B(b, 0x3A4);
        int delta = hit_dir - ball_dir - 0x80;
        int abs_d = ((delta & 0xFF) < 0x81) ? delta : -delta;
        int target_dir;
        int step;

        out_heading = hit_dir;
        if ((abs_d & 0xFF) >= 0x41) {
            abs_d = 0x40;
        }
        if (((out_heading - ball_dir - 0x80) & 0xFF) < 0x81) {
            target_dir = ball_dir - 0x10;
        } else {
            target_dir = ball_dir + 0x10;
        }
        step = (-0x80 - ((abs_d & 0xFF) << 1)) & 0xFE;
        out_heading = func_8005B7CC((ball_dir + 0x80) & 0xFF, target_dir & 0xFF, step);
        hit_dir = out_heading;
    } else {
        unsigned int event_code = W(scratch, 0x2B0);

        if (event_code == 0xB) {
            unsigned char *b = (unsigned char *)W(scratch, 0x2F4);
            W(b, 0x3A0) >>= 1;
        } else if (event_code == 9) {
            int step = func_8001E6B8(0x28) + 0x20;
            out_heading = func_8005B7CC(heading & 0xFF, hit_dir & 0xFF, step & 0xFF);
            goto after_event;
        } else if (event_code == 0xD) {
            unsigned char *b = (unsigned char *)W(scratch, 0x2F4);
            int atk_angle = B(actor, 0x399) << 7;
            int ball_dir = B(b, 0x3A4);
            int diff = (ball_dir - atk_angle) & 0xFF;
            int in_arc = (diff < 0x81) ? (diff < 0x29) : (((atk_angle - ball_dir) & 0xFF) < 0x29);

            if (diff < 0x81 && !in_arc) {
                out_heading = hit_dir;
            } else if (in_arc) {
                unsigned char *b2 = (unsigned char *)W(scratch, 0x2F4);
                int side_dir = 0xC0;
                int cur_dir = 0x80 - B(b2, 0x3A4);
                int rnd;
                unsigned char *b3;

                if (S(b2, 6) > 0) {
                    side_dir = 0x40;
                }
                rnd = func_8001E6B8(8);
                out_heading = func_8005B7CC(side_dir & 0xFF, cur_dir & 0xFF, (rnd + 8) & 0xFF);
                b3 = (unsigned char *)W(scratch, 0x2F4);
                W(b3, 0x3A0) = (W(b3, 0x3A0) << 2) / 10;
            } else {
                out_heading = hit_dir;
            }

            {
                int rnd = func_8001E6B8(8);
                unsigned char *b2 = (unsigned char *)W(scratch, 0x2F4);
                int vy_scaled = SW(b2, 0xC) / 736;
                if (vy_scaled < 0) vy_scaled = -vy_scaled;
                pitch = -0x20;
                lift = (vy_scaled * 6) / 10 + rnd;
            }
            goto after_event;
        } else if (event_code == 0xE) {
            out_heading = hit_dir;
            goto after_event;
        }

        if (B(actor, 0x38D) == B(scratch, 0x2AC)) {
            int rnd = func_8001E6B8(4);
            unsigned char *b = (unsigned char *)W(scratch, 0x2F4);
            int vy_scaled = SW(b, 0xC) / 736;
            if (vy_scaled < 0) vy_scaled = -vy_scaled;
            out_heading = hit_dir;
            pitch = 0;
            lift = -vy_scaled - rnd;
            W(b, 0x3A0) = (W(b, 0x3A0) * 7) / 10;
        } else {
            out_heading = heading;
        }
    }

after_event:
    blend_pitch = pitch * 48;
    angle_diff = (B(actor, 0x3A4) - out_heading) & 0xFF;
    if (angle_diff < 0x81) {
        pitch = blend_pitch / (angle_diff + 0x40);
    } else {
        pitch = blend_pitch / (((out_heading - B(actor, 0x3A4)) & 0xFF) + 0x40);
    }

    jitter = func_8005C7A0(8);
    final_heading = func_8005B7CC((out_heading + jitter) & 0xFF, hit_dir & 0xFF, 0x58);
    func_800951C4(actor, final_heading & 0xFF, lift, pitch);

    prev_event = W(scratch, 0x2B0);
    S(scratch, 0x2B4) = 0x7FFF;
    if (prev_event == 0xB) {
        S(scratch, 0x31C) = B(actor, 0x399) ^ 0xB;
    } else {
        S(scratch, 0x31C) = B(actor, 0x399) ^ 1;
    }
    W(scratch, 0x2B0) = 0xB;
    B(scratch, 0x2CF + B(actor, 0x398)) = 1;

    return 1;
}
