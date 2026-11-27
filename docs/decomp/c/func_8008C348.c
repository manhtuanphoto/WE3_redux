/*
 * func_8008C348 - Goalkeeper crouched/sidestep positioning and ball-action state machine
 *
 * Purpose:
 *   Updates a goalkeeper actor (substate Actor +0x397: 0 = initialize forward or
 *   reverse anim 0x69 based on |x| vs 0x28F1; 1 = advance forward to frame 0x2D;
 *   2 = play anim 0x69 in reverse down to frame 0). Transitions to state 0x81
 *   when the animation completes. For human-controlled GKs facing their attack
 *   direction, checks action buttons (0x1A -> state 0x94 if |x| >= 0x2563;
 *   0x12/0x16 -> state 0x92 if func_8008BD20 succeeds).
 *
 * Arguments:
 *   actor (u8*) - pointer to 1000-byte goalkeeper actor struct
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Actor fields read & written:
 *   Scratch 0x12[2] (u16) - mapped button mask per side (with 0x16 -> func_8008BD20 / state 0x92)
 *   Scratch 0x16[2] (u16) - mapped button mask per side (with 0x12 -> func_8008BD20 / state 0x92)
 *   Scratch 0x1A[2] (u16) - mapped button mask per side (state 0x94 if |x| >= 0x2563)
 *   Scratch 0x332   (u8)  - cleared to 0 at exit
 *   Actor +0x02     (s16) - x position
 *   Actor +0x26     (s16) - model rotation ((0x1C0 - heading) % 256) << 4
 *   Actor +0x390    (u8)  - animation frame
 *   Actor +0x396    (u8)  - state (0x81, 0x92, or 0x94)
 *   Actor +0x397    (u8)  - substate (0, 1, 2)
 *   Actor +0x398    (u8)  - side (0/1)
 *   Actor +0x399    (u8)  - attack direction flag
 *   Actor +0x39A    (u8)  - CPU control flag (0 = human)
 *   Actor +0x3A0    (u32) - speed (0x12 during motion, zeroed at exit)
 *   Actor +0x3A4    (u8)  - move heading
 *   Actor +0x3AA    (u8)  - body heading
 *   Actor +0x3B1    (u8)  - frame counter (incremented)
 *   Actor +0x3C2    (u16) - stores return value of func_8008BD20 when transitioning to 0x92
 *   Actor +0x3D1    (u8)  - cleared to 0 in substate 0
 *   Actor +0x3D6    (u16) - newly pressed button input mask
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define H(p, o) (*(unsigned short *)((p) + (o)))
#define S(p, o) (*(signed short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern void func_800512B8(unsigned char *actor, int arg1);
extern void func_80094FF4(unsigned char *actor);
extern void func_8005BA30(unsigned char *actor, int anim_id, int frame);
extern void func_8005BD1C(unsigned char *actor);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int current, unsigned int step);
extern unsigned int func_8008BD20(unsigned char *actor);
extern void func_8005B908(unsigned char *actor);
extern void func_800954E8(unsigned char *actor);

void func_8008C348(unsigned char *actor)
{
    unsigned char substate;

    func_800512B8(actor, 4);
    substate = B(actor, 0x397);

    if (substate == 0) {
        int x;
        int abs_x;

        B(actor, 0x3D1) = 0;
        func_80094FF4(actor);
        x = S(actor, 0x02);
        abs_x = (x >= 0) ? x : -x;
        W(actor, 0x3A0) = 0;

        if (abs_x >= 0x28F1) {
            func_8005BA30(actor, 0x69, 0);
            B(actor, 0x3A4) = (unsigned char)(B(actor, 0x399) << 7);
            B(actor, 0x397) = (unsigned char)(B(actor, 0x397) + 1);
        } else {
            func_8005BA30(actor, 0x69, 0x2D);
            B(actor, 0x397) = 2;
            B(actor, 0x3A4) = (unsigned char)((B(actor, 0x399) ^ 1) << 7);
        }
    } else if (substate == 1) {
        unsigned int body;
        int rot;

        func_8005BD1C(actor);
        body = func_8005B7CC((B(actor, 0x399) << 7) & 0x80, B(actor, 0x3AA), 0x10) & 0xFF;
        B(actor, 0x3AA) = (unsigned char)body;
        rot = (0x1C0 - (int)body) % 256;
        S(actor, 0x26) = (signed short)(rot << 4);

        if (B(actor, 0x390) < 0x26) {
            W(actor, 0x3A0) = 0x12;
        }
        if (B(actor, 0x390) == 0x2D) {
            B(actor, 0x396) = 0x81;
            B(actor, 0x397) = 0;
        }
    } else if (substate == 2) {
        unsigned int body;
        int rot;

        func_8005BA30(actor, 0x69, (B(actor, 0x390) - 1) & 0xFF);
        body = func_8005B7CC((B(actor, 0x399) << 7) & 0x80, B(actor, 0x3AA), 0x10) & 0xFF;
        B(actor, 0x3AA) = (unsigned char)body;
        rot = (0x1C0 - (int)body) % 256;
        S(actor, 0x26) = (signed short)(rot << 4);

        if (B(actor, 0x390) >= 8) {
            W(actor, 0x3A0) = 0x12;
        }
        if (B(actor, 0x390) == 0) {
            B(actor, 0x396) = 0x81;
            B(actor, 0x397) = 0;
        }
    }

    if (B(actor, 0x39A) == 0 && B(actor, 0x3AA) == (unsigned char)(B(actor, 0x399) << 7)) {
        unsigned char side = B(actor, 0x398);
        unsigned short pad = H(actor, 0x3D6);
        int triggered_94 = 0;

        if (H(Scratchpad, 0x1A + side * 2) & pad) {
            int x = S(actor, 0x02);
            int abs_x = (x >= 0) ? x : -x;
            if (abs_x >= 0x2563) {
                B(actor, 0x396) = 0x94;
                B(actor, 0x397) = 0;
                triggered_94 = 1;
            }
        }

        if (!triggered_94) {
            if ((H(Scratchpad, 0x12 + side * 2) | H(Scratchpad, 0x16 + side * 2)) & pad) {
                unsigned int res = func_8008BD20(actor) & 0xFF;
                if (res != 0) {
                    H(actor, 0x3C2) = (unsigned short)res;
                    B(actor, 0x396) = 0x92;
                    B(actor, 0x397) = 0;
                }
            }
        }
    }

    B(actor, 0x3B1) = (unsigned char)(B(actor, 0x3B1) + 1);
    func_8005B908(actor);
    W(actor, 0x3A0) = 0;
    func_800954E8(actor);
    B(Scratchpad, 0x332) = 0;
}
