/*
 * func_8008C618
 * Purpose: Goalkeeper dive / shot-stopping decision check.
 *          Evaluates ball timer (< 24 frames), ball heading relative to goal and GK,
 *          predicted ball trajectory point (SCRATCH+0x40, 0x58, 0x70) against GK stats
 *          and distance, and triggers dive state 0x9B (high), 0x8B (close), or 0x89 (far).
 * Arguments:
 *   actor - pointer to 1000-byte goalkeeper actor struct
 * Return value:
 *   1 if GK initiates a dive/save, 0 otherwise
 * Scratchpad reads/writes:
 *   R: 0x40 (s16 predicted x), 0x58 (s16 predicted y/height), 0x70 (s16 predicted z),
 *      0x2F4 (ball actor ptr), 0x31C (s16 ball timer)
 *   W: none
 * Actor reads/writes:
 *   R: +0x02/+0x06 (x/z), +0x392 (team slot), +0x398 (side), +0x399 (attack dir),
 *      +0x3AA (body heading)
 *   W: +0x396 (state = 0x9B/0x8B/0x89), +0x397 (substate = 0), +0x3AB (target heading),
 *      +0x3AC (mirror/dive side), +0x3C2/+0x3C4/+0x3C6 (target x/y/z), +0x3D1 (= 0)
 * Constants:
 *   0x18 (24 frames = 0.8 s) max ball timer;
 *   0x39 (57 angle units = ~80 deg) away-from-goal heading filter;
 *   0x40 (64 angle units = 90 deg) away-from-GK heading filter;
 *   -0x33DF (-13279 raw = -53.116 m), -0x331F (-13087 raw = -52.348 m),
 *   +-0x3320 (+-13088 raw = +-52.352 m) goal-line X clamp;
 *   D_8010C330[side][slot] >> 11 & 0x7F + 100: GK stat scaling.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCH  ((unsigned char *)0x1F800000)

extern unsigned char D_8010C330[];

extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);

int func_8008C618(unsigned char *actor) {
    unsigned char *ball;
    unsigned int opp_dir;
    unsigned int ball_heading;
    unsigned int ang_to_ball;
    unsigned int ang_to_target;
    unsigned int body_heading;
    unsigned int diff;
    int stat;
    int max_height;
    int target_x;
    int target_z;
    int dir_x;
    int dx;
    int dz;
    unsigned int dist_sq;
    int scaled;
    unsigned int max_dist_sq;
    int state;

    if (S(SCRATCH, 0x31C) >= 0x18) { /* 24 frames = 0.8 s */
        return 0;
    }

    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    opp_dir = B(actor, 0x399) << 7;
    ball_heading = B(ball, 0x3A4);
    diff = (opp_dir - ball_heading) & 0xFF;
    if (diff < 0x81) {
        if (diff < 0x39) {
            return 0;
        }
    } else {
        if (((ball_heading - opp_dir) & 0xFF) < 0x39) {
            return 0;
        }
    }

    ang_to_ball = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    ball_heading = B(ball, 0x3A4);
    if (((ang_to_ball - ball_heading) & 0xFF) < 0x81) {
        ang_to_ball = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        if (((ang_to_ball - B(ball, 0x3A4)) & 0xFF) < 0x40) {
            return 0;
        }
    } else {
        ang_to_ball = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
        ball = (unsigned char *)W(SCRATCH, 0x2F4);
        if (((B(ball, 0x3A4) - ang_to_ball) & 0xFF) < 0x40) {
            return 0;
        }
    }

    stat = ((W(D_8010C330, B(actor, 0x398) * 0x108 + B(actor, 0x392) * 0xC) >> 11) & 0x7F) + 0x64;
    max_height = (-548 * stat) / 180;
    if (S(SCRATCH, 0x58) < max_height) {
        return 0;
    }

    target_x = S(SCRATCH, 0x40);
    dir_x = (B(actor, 0x399) == 0) ? target_x : -target_x;
    if (dir_x < -0x33DF) { /* -53.116 m */
        return 0;
    }
    if (dir_x < -0x331F) { /* -52.348 m */
        target_x = (B(actor, 0x399) != 0) ? 0x3320 : -0x3320; /* +-52.352 m */
    }

    dx = S(actor, 2) - (signed short)target_x;
    stat = ((W(D_8010C330, B(actor, 0x398) * 0x108 + B(actor, 0x392) * 0xC) >> 11) & 0x7F) + 0x64;
    target_z = S(SCRATCH, 0x70);
    dz = S(actor, 6) - target_z;
    dist_sq = (unsigned int)(dx * dx + dz * dz);

    scaled = stat * 576;
    max_dist_sq = (unsigned int)(((scaled / 180) * scaled) / 240);
    if (max_dist_sq < dist_sq) {
        return 0;
    }

    ang_to_target = func_8005B36C(S(actor, 2), S(actor, 6), (signed short)target_x, target_z);
    body_heading = B(actor, 0x3AA);
    diff = (body_heading - ang_to_target) & 0xFF;
    if (diff < 0x81) {
        if (diff >= 0x41) {
            return 0;
        }
    } else {
        if (((ang_to_target - body_heading) & 0xFF) < 0x41) {
            return 0;
        }
    }

    B(actor, 0x3D1) = 0;
    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    B(actor, 0x3AB) = (B(ball, 0x3A4) + 0x80) & 0xFF;

    ball = (unsigned char *)W(SCRATCH, 0x2F4);
    ang_to_ball = func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
    B(actor, 0x3AC) = ((ang_to_target - ang_to_ball) & 0xFF) < 0x81;

    S(actor, 0x3C2) = (signed short)target_x;
    S(actor, 0x3C6) = (signed short)target_z;
    S(actor, 0x3C4) = S(SCRATCH, 0x58);

    stat = ((W(D_8010C330, B(actor, 0x398) * 0x108 + B(actor, 0x392) * 0xC) >> 11) & 0x7F) + 0x64;
    if (S(SCRATCH, 0x58) < (-368 * stat) / 180) {
        state = 0x9B;
    } else {
        scaled = stat * 136;
        if (dist_sq < (unsigned int)(((scaled / 180) * scaled) / 180)) {
            state = 0x8B;
        } else {
            state = 0x89;
        }
    }

    B(actor, 0x396) = state;
    B(actor, 0x397) = 0;
    return 1;
}
