/*
 * func_80075C3C - Replay / attract-mode camera & ball frame setup
 *
 * Purpose:
 *   Two-step state machine driven by D_8010A322. Step 0 initializes replay/demo
 *   playback state; step 1 resets all 22 player actors, loads the ball's x/y/z
 *   from the selected replay sequence table (func_80188F78[scratch+0x2E8 % 5]),
 *   chooses a randomized camera angle/distance/height, computes camera position
 *   and look target in scratchpad, and calls func_800171DC.
 *
 * Arguments:
 *   None (void).
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Actor fields read & written:
 *   D_8010A322     (u8)  - state (0=init, 1=setup frame)
 *   Scratch 0x198  (u32) - cleared to 0 in state 0
 *   Scratch 0x1E8  (u8)  - set to 1 in state 0
 *   Scratch 0x2E8  (s16) - sequence index (0 in state 0; % 5 in state 1)
 *   Scratch 0x288  (u16) - frame index read from sequence->0x30
 *   Scratch 0x1F0/1F4/1F8 (s32) - camera eye x/y/z
 *   Scratch 0x1FC/200/204 (s32) - camera look-at target x/y/z
 *   Scratch 0x2E7  (u8)  - camera yaw angle byte
 *   Scratch 0x2EA  (s16) - horizontal camera distance
 *   Scratch 0x2F0  (u8)  - counter passed to func_800171DC after increment
 *   Scratch 0x2F4  (ptr) - ball actor pointer (+2/+4/+6 s16 x/y/z)
 *   Actors 1..22         - +0=0, +0x3A5=12, +0x3A6=24, +0x3D3=0
 *   D_800FF748 +0        - set to 0
 *   D_8010C2A0     (s16) - frame_idx ? frame_idx - 1 : 119 (0x77)
 *   D_800F0EE0     (u16) - copy of scratch+0x288
 *   D_800EF934/36/38 (s16) - camera reference x/y/z
 *   D_800F2B08     (s16) - camera zoom/distance parameter
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   -0x120 (-288)  = -1.152 m (camera look-at height y)
 *   +/-0x4200 (16896) = +/-67.584 m (touchline camera z)
 *   +/-0x5535 (21813) = +/-87.252 m
 *   +/-0x3320 (13088) = +/-52.352 m (goal line x)
 *   +/-0x3FE8 (16360), +/-0x39C0 (14784) = +/-65.44 m, +/-59.136 m
 *   0x1600 (5632)  = 22.528 m (minimum horizontal camera distance)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char D_8010A322;
extern unsigned char D_800FF748[];
extern unsigned char *func_80188F78[];
extern signed short D_8010C2A0;
extern unsigned short D_800F0EE0;
extern signed short D_800EF934;
extern signed short D_800EF936;
extern signed short D_800EF938;
extern signed short D_800F2B08;

extern void func_80098428(void);
extern void func_80096050(void);
extern int func_8001E6B8(int n);
extern unsigned int func_800B1128(unsigned int val);
extern unsigned int func_8005B61C(int x1, int z1, int x2, int z2);
extern int func_8001E694(unsigned int angle, int scale);
extern int func_8001E5A4(unsigned int angle, int scale);
extern void func_800171DC(unsigned int count);

void func_80075C3C(void)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char state = D_8010A322;

    if (state == 0) {
        W(scratch, 0x198) = 0;
        func_80098428();
        func_80096050();
        B(scratch, 0x1E8) = 1;
        H(scratch, 0x2E8) = 0;
        D_8010A322 = (unsigned char)(D_8010A322 + 1);
    } else if (state == 1) {
        int i;
        unsigned char *actor;
        unsigned char *seq;
        unsigned int frame_idx;
        unsigned char *ball;
        int ball_x;
        int mode;
        signed short ref_x;
        signed short ref_z;
        int cam_dist;
        int cam_y;
        int dx;
        int dz;
        unsigned int horiz_dist;
        signed short cam_horiz;
        unsigned int pitch;
        unsigned int yaw;
        unsigned char cnt;

        B(D_800FF748, 0) = 0;
        actor = D_800FF748 + 1000;
        for (i = 0; i < 22; i++) {
            B(actor, 0x3A5) = 12;
            B(actor, 0x3A6) = 24;
            B(actor, 0x3D3) = 0;
            B(actor, 0) = 0;
            actor += 1000;
        }

        seq = func_80188F78[S(scratch, 0x2E8) % 5];
        frame_idx = B((unsigned char *)W(seq, 0x30), 0);
        H(scratch, 0x288) = (unsigned short)frame_idx;
        D_8010C2A0 = (frame_idx == 0) ? 0x77 : (signed short)(frame_idx - 1);

        D_800F0EE0 = H(scratch, 0x288);
        ball = (unsigned char *)W(scratch, 0x2F4);
        S(ball, 2) = S((unsigned char *)W(seq, 0x1C), (int)S(scratch, 0x288) * 2);
        S(ball, 4) = S((unsigned char *)W(seq, 0x20), (int)S(scratch, 0x288) * 2);
        S(ball, 6) = S((unsigned char *)W(seq, 0x24), (int)S(scratch, 0x288) * 2);

        ball = (unsigned char *)W(scratch, 0x2F4);
        SW(scratch, 0x200) = -0x120;
        SW(scratch, 0x1FC) = S(ball, 2);
        SW(scratch, 0x204) = S(ball, 6);

        mode = func_8001E6B8(4);
        switch (mode) {
        case 0:
            D_800EF934 = 0;
            ref_z = (func_8001E6B8(2) == 0) ? -0x4200 : 0x4200;
            D_800EF938 = ref_z;
            break;
        case 1:
            ball_x = S((unsigned char *)W(scratch, 0x2F4), 2);
            D_800EF934 = (ball_x <= 0) ? -0x5535 : 0x5535;
            D_800EF938 = 0;
            break;
        case 2:
            ball_x = S((unsigned char *)W(scratch, 0x2F4), 2);
            D_800EF934 = (ball_x <= 0) ? 0x3320 : -0x3320;
            D_800EF938 = 0;
            break;
        case 3:
            ball_x = S((unsigned char *)W(scratch, 0x2F4), 2);
            ref_x = (ball_x <= 0) ? -0x3FE8 : 0x3FE8;
            D_800EF934 = ref_x;
            ball_x = S((unsigned char *)W(scratch, 0x2F4), 2);
            ref_z = (ball_x <= 0) ? -0x39C0 : 0x39C0;
            D_800EF938 = ref_z;
            break;
        }

        if (D_800EF938 != 0) {
            cam_dist = (func_8001E6B8(16) << 8) + 0x1080;
        } else {
            ball_x = S((unsigned char *)W(scratch, 0x2F4), 2);
            if (ball_x * (int)D_800EF934 > 0) {
                cam_dist = (func_8001E6B8(16) << 8) + 0x1480;
            } else {
                cam_dist = (func_8001E6B8(16) << 8) + 0xD33;
            }
        }
        D_800F2B08 = (signed short)cam_dist;

        if (D_800EF938 == 0 && S((unsigned char *)W(scratch, 0x2F4), 2) * (int)D_800EF934 < 0) {
            int step = (D_800F2B08 - 0xD33) / 256;
            cam_y = -0x2E0 - (step << 4);
        } else {
            int r = func_8001E6B8(2);
            int step = (D_800F2B08 - 0xD33) / 256;
            cam_y = -0x2C0 - step * (r << 4) - (step << 4);
        }
        D_800EF936 = (signed short)cam_y;

        dx = D_800EF934 - SW(scratch, 0x1FC);
        dz = D_800EF938 - SW(scratch, 0x204);
        horiz_dist = func_800B1128((unsigned int)(dx * dx + dz * dz));

        if ((int)(horiz_dist - 0x1600) < (int)D_800F2B08) {
            cam_horiz = 0x1600;
        } else {
            cam_horiz = (signed short)(horiz_dist - (unsigned int)D_800F2B08);
        }
        S(scratch, 0x2EA) = cam_horiz;

        pitch = func_8005B61C(0, (signed short)(SW(scratch, 0x200) - D_800EF936),
                              (signed short)horiz_dist, D_800EF936);
        SW(scratch, 0x1F4) = func_8001E5A4(pitch & 0xFF, D_800F2B08);

        yaw = func_8005B61C(D_800EF934, D_800EF938,
                            (signed short)SW(scratch, 0x1FC), (signed short)SW(scratch, 0x204));
        B(scratch, 0x2E7) = (unsigned char)yaw;

        SW(scratch, 0x1F0) = D_800EF934 + func_8001E694(B(scratch, 0x2E7), S(scratch, 0x2EA));
        SW(scratch, 0x1F8) = D_800EF938 + func_8001E5A4(B(scratch, 0x2E7), S(scratch, 0x2EA));

        cnt = (unsigned char)(B(scratch, 0x2F0) + 1);
        B(scratch, 0x2F0) = cnt;
        func_800171DC(cnt);
    }
}
