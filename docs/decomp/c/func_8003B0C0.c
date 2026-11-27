/*
 * func_8003B0C0 - Player jumping header / aerial strike action handler
 *
 * Purpose:
 *   Updates a player performing a jumping header / aerial ball strike across
 *   two sub-states (+0x397):
 *     - Sub-state 0 (takeoff/ascent): turns toward +0x3AB (step 0xC), moves by
 *       (+0x3CA, +0x3CC, +0x3CE), and at anim frame >= 5 transitions to sub-state 1,
 *       halving vertical velocity (+0x3CC) and testing ball contact via func_8003D274.
 *       If within vertical reach ([-255, +191] raw), snaps the ball to the contact
 *       point (Scratchpad+0x0F0/+0x04E/+0x0F4), launches it via func_8004B0F0,
 *       sets spin/timer/sound (0x507), and triggers func_8002B5A4(actor, 0xB).
 *     - Sub-state 1 (descent/landing): applies step velocities while subtracting
 *       +0x3CC from Y and damping +0x3CA/+0x3CE by 9/10 per frame; clamps Y to 0
 *       from frame 0xF onward and resets sub-state at frame 0x19.
 *   From anim frame 6 onward, also runs body deflection check func_8009462C and
 *   calls func_800498AC at the end.
 *
 * Arguments:
 *   actor (unsigned char *): pointer to 1000-byte player actor struct.
 *
 * Return value:
 *   void.
 *
 * Scratchpad (0x1F800000) reads:
 *   - 0x04E (s16, R): predicted ball Y at frame 0 (clamped to <= -0x28 on contact).
 *   - 0x0F0/0x0F2/0x0F4 (s16, R): header contact point X, Y, Z.
 *   - 0x2F4 (u8 *, R): pointer to ball actor (+0x02/+0x04/+0x06, +0x396, +0x3A4, +0x3C8, +0x3CA).
 *
 * Actor fields read/written:
 *   - +0x02/+0x04/+0x06 (s16, R/W): X, Y, Z position.
 *   - +0x26 (u16, W): model Y rotation = ((0x1C0 - heading) % 256) << 4.
 *   - +0x390 (u8, R): animation frame counter.
 *   - +0x396 (u8, W): actor state (cleared to 0 in sub-state 1).
 *   - +0x397 (u8, R/W): header sub-state (0 -> 1 at frame 5; 1 -> 0 at frame 0x19).
 *   - +0x3AA (u8, R/W): body heading.
 *   - +0x3AB (u8, R/W): target heading (updated to kick_dir on contact).
 *   - +0x3B2 (u8, R): aim parameter passed to func_8003D99C.
 *   - +0x3CA/+0x3CC/+0x3CE (s16, R/W): X/Y/Z per-frame step velocity.
 *
 * Magic numbers (250 raw = 1 m, 30 Hz):
 *   - 0x1BF (447): unsigned vertical tolerance (contactY - ballY + 255 < 447).
 *   - -0x37 / -0x28 (-55 / -40 raw): ball Y floor clamp on header contact.
 *   - 0x507: ball strike sound effect ID.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern void func_8005BD1C(unsigned char *actor);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int current, int step);
extern unsigned int func_8003D274(unsigned char *actor, int arg1, int arg2);
extern unsigned int func_8003D99C(unsigned char *actor, unsigned int arg1, int arg2);
extern int func_8001E6B8(int range);
extern void func_8004B0F0(unsigned char *actor, unsigned int dir, int speed, int lift);
extern void func_800AB620(int sfx_id);
extern void func_8002B5A4(unsigned char *actor, unsigned int arg1);
extern void func_8005707C(unsigned char *actor);
extern int func_8009462C(unsigned char *actor, int mode, int heading, int pitch, int lift);
extern void func_800498AC(unsigned char *actor);

void func_8003B0C0(unsigned char *actor)
{
    unsigned int substate = B(actor, 0x397);

    if (substate == 0u) {
        unsigned int heading;
        int rot_diff;

        func_8005BD1C(actor);
        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0xC);
        B(actor, 0x3AA) = (unsigned char)heading;

        rot_diff = 0x1C0 - (int)(heading & 0xFFu);
        H(actor, 0x26) = (unsigned short)((rot_diff % 256) << 4);

        H(actor, 0x2) = (unsigned short)(H(actor, 0x2) + H(actor, 0x3CA));
        H(actor, 0x4) = (unsigned short)(H(actor, 0x4) + H(actor, 0x3CC));
        H(actor, 0x6) = (unsigned short)(H(actor, 0x6) + H(actor, 0x3CE));

        if (B(actor, 0x390) >= 5) {
            B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
            S(actor, 0x3CC) = (signed short)(S(actor, 0x3CC) >> 1);

            if ((func_8003D274(actor, 0, 0) & 0xFFu) != 0u) {
                int y_diff = (int)S(Scratchpad, 0x0F2) - (int)S(Scratchpad, 0x04E) + 0xFF;
                if ((unsigned int)y_diff < 0x1BFu) {
                    unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
                    signed short by;
                    unsigned int kick_dir;
                    int speed;
                    int lift;
                    int r2;
                    unsigned int body_ang;
                    unsigned int diff1;
                    int spin;

                    H(ball, 0x2) = H(Scratchpad, 0x0F0);
                    by = S(Scratchpad, 0x04E);
                    ball = (unsigned char *)W(Scratchpad, 0x2F4);
                    H(ball, 0x4) = (by >= -0x37) ? (unsigned short)(-0x28) : (unsigned short)by;

                    ball = (unsigned char *)W(Scratchpad, 0x2F4);
                    H(ball, 0x6) = H(Scratchpad, 0x0F4);

                    kick_dir = func_8003D99C(actor, B(actor, 0x3B2), 1);
                    speed = func_8001E6B8(8) + 0x8C;
                    lift = func_8001E6B8(6) + 0x4;
                    func_8004B0F0(actor, kick_dir & 0xFFu, speed, lift);

                    ball = (unsigned char *)W(Scratchpad, 0x2F4);
                    B(ball, 0x396) = 4;

                    diff1 = (kick_dir - B(actor, 0x3AA)) & 0xFFu;
                    H(ball, 0x3CA) = (diff1 < 0x81u) ? 1 : 0;

                    ball = (unsigned char *)W(Scratchpad, 0x2F4);
                    r2 = func_8001E6B8(2);
                    body_ang = B(actor, 0x3AA);
                    {
                        unsigned int diff2 = (body_ang - kick_dir) & 0xFFu;
                        int ang_term;
                        if (diff2 < 0x81u) {
                            ang_term = (int)diff2 >> 6;
                        } else {
                            ang_term = (int)((kick_dir - body_ang) & 0xFFu) >> 6;
                        }
                        spin = (9 - r2) - ang_term;
                    }
                    H(ball, 0x3C8) = (unsigned short)spin;

                    func_800AB620(0x507);
                    B(actor, 0x3AB) = (unsigned char)kick_dir;
                    func_8002B5A4(actor, 0xB);
                    func_8005707C(actor);
                }
            }
        }
    } else if (substate == 1u) {
        signed short vx;
        signed short vz;

        func_8005BD1C(actor);
        vx = S(actor, 0x3CA);
        vz = S(actor, 0x3CE);
        H(actor, 0x2) = (unsigned short)(H(actor, 0x2) + (unsigned short)vx);
        H(actor, 0x4) = (unsigned short)(H(actor, 0x4) - H(actor, 0x3CC));
        H(actor, 0x6) = (unsigned short)(H(actor, 0x6) + (unsigned short)vz);

        S(actor, 0x3CA) = (signed short)((vx * 9) / 10);
        S(actor, 0x3CE) = (signed short)((vz * 9) / 10);

        if (B(actor, 0x390) >= 0xF) {
            H(actor, 0x4) = 0;
        }
        B(actor, 0x396) = 0;
        if (B(actor, 0x390) == 0x19) {
            B(actor, 0x397) = 0;
        }
    }

    if (B(actor, 0x390) >= 6) {
        int r = func_8001E6B8(4);
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        func_8009462C(actor, 0xC, B(ball, 0x3A4) + 0x80, 0, -4 - r);
    }
    func_800498AC(actor);
}
