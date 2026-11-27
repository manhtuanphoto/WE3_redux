/*
 * func_80021A68 - Position and orient a player actor during set-piece / kickoff / formation reset
 *
 * Purpose:
 *   Places a player actor at a set-piece or formation position via func_8001D14C,
 *   updates ball ownership / set-piece scratchpad state when placing the kicker
 *   or receiver, and resets the actor's speed, rotation, state (0x2B or 0xFF),
 *   and facing toward the center of the pitch (0, 0).
 *
 * Arguments:
 *   actor     - pointer to 1000-byte player actor struct
 *   state_ptr - pointer to u8 step/counter byte (0 = kicker, 1 = partner, >=2 = formation)
 *   mode_flag - u8 flag; 0 = full kickoff/set-piece placement, nonzero = quick reset
 *   slot_id   - u8 formation slot index (>= 2 for outfielders; actor_id 1 or 12 = GK)
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Actor fields read & written:
 *   Scratch 0x2A8 (u8)  - last-touch actor id (written when *state_ptr == 0)
 *   Scratch 0x2A9 (u8)  - ball owner actor id (written when *state_ptr == 0)
 *   Scratch 0x2AD (u8)  - set-piece kicker actor id (written when *state_ptr == 0)
 *   Scratch 0x2AE (u8)  - set-piece secondary actor id (written when *state_ptr == 1)
 *   Scratch 0x2E1 (u8)  - mode (5 = hide actor off-pitch at x=0x7000, state=0xFF)
 *   Scratch 0x2FD[2] (u8) - formation id per side
 *   Actor +0x02/+0x06 (s16) - x, z position
 *   Actor +0x26       (s16) - model rotation
 *   Actor +0x2C/30/34 (s32) - model x/y/z scale
 *   Actor +0x38D      (u8)  - actor id
 *   Actor +0x396      (u8)  - state (0x2B, or 0xFF if scratch 0x2E1 == 5)
 *   Actor +0x397      (u8)  - substate (0x10, 0x20, or 0)
 *   Actor +0x398      (u8)  - side (0/1)
 *   Actor +0x399      (u8)  - attack direction flag (0/1)
 *   Actor +0x3A0      (u32) - speed (zeroed)
 *   Actor +0x3A4      (u8)  - move heading
 *   Actor +0x3AA      (u8)  - body heading
 *   Actor +0x3D1      (u8)  - set to 1 when *state_ptr == 1
 *   Actor +0x3E2/3E3  (u8)  - zeroed
 *
 * Magic numbers:
 *   0x42   (66)     = 0.264 m (kicker x offset)
 *   0x25B  (603)    = 2.412 m (kickoff partner z offset)
 *   0x3220 (12832)  = 51.328 m (goalkeeper x position)
 *   0x7000 (28672)  = 114.688 m (off-pitch x coordinate when mode == 5)
 *   0x30   (48)     = 1.6 s at 30 Hz (argument to func_8001D14C for kicker)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char D_800C7DF8[];
extern unsigned char D_800C7DFA[];

extern void func_8001D14C(unsigned int actor_id, int arg1, int arg2, int x, int z,
                          int angle, unsigned int sx, unsigned int sy, unsigned int sz, int arg9);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);

void func_80021A68(unsigned char *actor, unsigned char *state_ptr, unsigned char mode_flag, unsigned int slot_id)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char actor_id;
    unsigned char heading;
    int turn;

    if (mode_flag == 0) {
        unsigned char step = B(state_ptr, 0);

        if (step == 0) {
            unsigned char atk_dir = B(actor, 0x399);
            int off_x = atk_dir ? 0x42 : -0x42;
            int off_z = atk_dir ? 0x40 : -0x40;
            unsigned int angle = ((atk_dir << 7) + 0x20) & 0xFF;

            actor_id = B(actor, 0x38D);
            func_8001D14C(actor_id, 0, 0x30, off_x, off_z, angle,
                          W(actor, 0x2C), W(actor, 0x30), W(actor, 0x34), 1);

            B(actor, 0x3A4) = (unsigned char)(B(actor, 0x399) << 7);
            actor_id = B(actor, 0x38D);
            B(scratch, 0x2AD) = actor_id;
            B(scratch, 0x2A9) = actor_id;
            B(scratch, 0x2A8) = actor_id;
            B(actor, 0x397) = 0x10;
            B(state_ptr, 0) = (unsigned char)(B(state_ptr, 0) + 1);
        } else if (step == 1) {
            unsigned char atk_dir = B(actor, 0x399);
            int off_z = atk_dir ? -0x25B : 0x25B;
            unsigned int angle = (unsigned int)(-0x40 - (atk_dir << 7)) & 0xFF;

            actor_id = B(actor, 0x38D);
            func_8001D14C(actor_id, 0, 1, 0, off_z, angle,
                          W(actor, 0x2C), W(actor, 0x30), W(actor, 0x34), 1);

            B(scratch, 0x2AE) = B(actor, 0x38D);
            B(actor, 0x397) = 0x20;
            B(actor, 0x3D1) = 1;
            B(state_ptr, 0) = (unsigned char)(B(state_ptr, 0) + 1);
        } else {
            actor_id = B(actor, 0x38D);
            if (actor_id == 1 || actor_id == 0xC) {
                int gk_x = B(actor, 0x399) ? 0x3220 : -0x3220;
                func_8001D14C(actor_id, 0, 0, gk_x, 0, 0,
                              W(actor, 0x2C), W(actor, 0x30), W(actor, 0x34), 1);
            } else {
                unsigned int formation = B(scratch, 0x2FD + B(actor, 0x398));
                unsigned int tbl_off = ((slot_id & 0xFF) - 2) * 4 + formation * 40;
                int fx = S(D_800C7DF8, tbl_off);
                int fz = S(D_800C7DFA, tbl_off);

                if (B(actor, 0x399) != 0) {
                    fx = (signed short)(-fx);
                    fz = (signed short)(-fz);
                }
                func_8001D14C(actor_id, 0, 0, fx, fz, 0,
                              W(actor, 0x2C), W(actor, 0x30), W(actor, 0x34), 1);
            }
            B(actor, 0x397) = 0;
            B(actor, 0x3AA) = (unsigned char)func_8005B36C(S(actor, 2), S(actor, 6), 0, 0);
        }
    } else {
        actor_id = B(actor, 0x38D);
        if (actor_id == 1 || actor_id == 0xC) {
            int gk_x = B(actor, 0x399) ? 0x3220 : -0x3220;
            func_8001D14C(actor_id, 0, 1, gk_x, 0, 0,
                          W(actor, 0x2C), W(actor, 0x30), W(actor, 0x34), 1);
        } else {
            unsigned int formation = B(scratch, 0x2FD + B(actor, 0x398));
            unsigned int tbl_off = ((slot_id & 0xFF) - 2) * 4 + formation * 40;
            int fx = S(D_800C7DF8, tbl_off);
            int fz = S(D_800C7DFA, tbl_off);

            if (B(actor, 0x399) != 0) {
                fx = (signed short)(-fx);
                fz = (signed short)(-fz);
            }
            func_8001D14C(actor_id, 0, 1, fx, fz, 0,
                          W(actor, 0x2C), W(actor, 0x30), W(actor, 0x34), 1);
        }

        if (B(scratch, 0x2E1) == 5) {
            H(actor, 2) = 0x7000;
            H(actor, 6) = 0;
            B(actor, 0x396) = 0xFF;
            return;
        }

        B(actor, 0x3AA) = (unsigned char)func_8005B36C(S(actor, 2), S(actor, 6), 0, 0);
        B(actor, 0x397) = 0;
        if (B(state_ptr, 0) == 0) {
            B(state_ptr, 0) = 1;
        }
    }

    heading = B(actor, 0x3AA);
    W(actor, 0x3A0) = 0;
    turn = 0x1C0 - heading;
    S(actor, 0x26) = (signed short)((turn % 256) << 4);
    B(actor, 0x396) = 0x2B;
    B(actor, 0x3E2) = 0;
    B(actor, 0x3E3) = 0;
}
