/*
 * func_8007B700 - Set-piece kicker idle, aiming, and kick-trigger state machine
 *
 * Purpose:
 *   Handles a player actor waiting to take a set-piece (free kick / corner /
 *   goal kick, dispatched on Actor +0x397: 0 = setup/wait, 1 = aim & kick input).
 *   In substate 0, starts an idle/wait animation unless Scratchpad 0x29B is nonzero.
 *   In substate 1, waits until D_800E7658 >= 16, adjusts ball heading via
 *   Actor +0x3D4 bits (0x8000 / 0x2000), optionally waits up to 60 frames on
 *   Scratchpad 0x296 when D_800E6B86 is set, simulates CPU button input after
 *   31 frames, and transitions to state 0x30 (substate 2) with Scratchpad 0x2B0
 *   set to 3, 4, or 1 depending on the pressed action button.
 *
 * Arguments:
 *   actor (u8*) - pointer to 1000-byte player actor struct
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Actor fields read & written:
 *   Scratch 0x12[2] (u16) - mapped button mask per side (triggers mode 4)
 *   Scratch 0x16[2] (u16) - mapped button mask per side (triggers mode 1, clears +0x3C4)
 *   Scratch 0x1A[2] (u16) - mapped button mask per side (triggers mode 3)
 *   Scratch 0x294   (u16) - set-piece wait timer (incremented each frame, or zeroed)
 *   Scratch 0x296   (s16) - set-piece pre-ready timer (0..60, or 0xFF when done)
 *   Scratch 0x29B   (u8)  - flag resetting actor state/substate to 0 if nonzero in substate 0
 *   Scratch 0x2B0   (u32) - kick/action type (1, 3, or 4)
 *   Scratch 0x2F4   (u8*) - pointer to ball actor struct (updates Ball +0x3A4 heading)
 *   Actor +0x38E    (u16) - current animation ID (checked != 1 in substate 0)
 *   Actor +0x396    (u8)  - state (0 or 0x30)
 *   Actor +0x397    (u8)  - substate (0, 1, or 2)
 *   Actor +0x398    (u8)  - side (0/1)
 *   Actor +0x39A    (u8)  - CPU control flag (nonzero = AI)
 *   Actor +0x3A0    (u32) - speed (zeroed in substate 0)
 *   Actor +0x3A7    (u8)  - processed flag (set to 1)
 *   Actor +0x3AB    (u8)  - target heading
 *   Actor +0x3C4    (u16) - cleared on mode 1 kick
 *   Actor +0x3D4    (u16) - held directional/aim input mask (0x8000 = +1, 0x2000 = -1)
 *   Actor +0x3D6    (u16) - newly pressed button input mask
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define H(p, o) (*(unsigned short *)((p) + (o)))
#define S(p, o) (*(signed short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern int D_800E7658;
extern unsigned short D_800E6B86;

extern int func_8001E6B8(int range);
extern void func_8005BA30(unsigned char *actor, int anim_id, int frame);
extern void func_8005BD1C(unsigned char *actor);
extern void func_800512B8(unsigned char *actor, int arg1);
extern void func_8005C7EC(unsigned char *actor);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int current, unsigned int step);
extern void func_800AB620(int sfx_id);
extern void func_80050510(int arg0);
extern void func_8005B908(unsigned char *actor);

void func_8007B700(unsigned char *actor)
{
    unsigned char substate = B(actor, 0x397);

    B(actor, 0x3A7) = 1;

    if (substate == 0) {
        if (B(Scratchpad, 0x29B) == 0) {
            if (H(actor, 0x38E) != 1) {
                int frame = func_8001E6B8(0x15) & 0xFF;
                func_8005BA30(actor, 1, frame);
            }
            func_8005BD1C(actor);
            W(actor, 0x3A0) = 0;
        } else {
            B(actor, 0x396) = 0;
            B(actor, 0x397) = 0;
            W(actor, 0x3A0) = 0;
        }
    } else if (substate == 1) {
        func_800512B8(actor, 6);

        if (D_800E7658 >= 0x10) {
            unsigned char *ball;
            unsigned char side;
            unsigned short pad;

            H(Scratchpad, 0x294) = (unsigned short)(H(Scratchpad, 0x294) + 1);
            func_8005C7EC(actor);

            if (H(actor, 0x3D4) & 0x8000) {
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                B(ball, 0x3A4) = (unsigned char)(B(ball, 0x3A4) + 1);
            }
            if (H(actor, 0x3D4) & 0x2000) {
                ball = (unsigned char *)W(Scratchpad, 0x2F4);
                B(ball, 0x3A4) = (unsigned char)(B(ball, 0x3A4) - 1);
            }

            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            B(ball, 0x3A4) = (unsigned char)func_8005B7CC(B(ball, 0x3A4), B(actor, 0x3AB), 8);

            if (D_800E6B86 != 0 && S(Scratchpad, 0x296) < 0x3C) {
                S(Scratchpad, 0x296) = (signed short)(S(Scratchpad, 0x296) + 1);
                H(Scratchpad, 0x294) = 0;
                return;
            }

            if (S(Scratchpad, 0x296) != 0xFF) {
                func_800AB620(0x517);
                H(Scratchpad, 0x296) = 0xFF;
            }

            if (B(actor, 0x39A) != 0 && H(Scratchpad, 0x294) >= 0x1F) {
                side = B(actor, 0x398);
                if (func_8001E6B8(4) != 0) {
                    H(actor, 0x3D6) = H(Scratchpad, 0x1A + side * 2);
                } else {
                    H(actor, 0x3D6) = H(Scratchpad, 0x16 + side * 2);
                }
            }

            side = B(actor, 0x398);
            pad = H(actor, 0x3D6);

            if (H(Scratchpad, 0x1A + side * 2) & pad) {
                func_80050510(0);
                W(Scratchpad, 0x2B0) = 3;
                B(actor, 0x396) = 0x30;
                B(actor, 0x397) = 2;
            } else if (H(Scratchpad, 0x12 + side * 2) & pad) {
                W(Scratchpad, 0x2B0) = 4;
                B(actor, 0x396) = 0x30;
                B(actor, 0x397) = 2;
                func_80050510(0);
            } else if (H(Scratchpad, 0x16 + side * 2) & pad) {
                W(Scratchpad, 0x2B0) = 1;
                B(actor, 0x396) = 0x30;
                H(actor, 0x3C4) = 0;
                B(actor, 0x397) = 2;
                func_80050510(0);
            }
        }
    }

    func_8005B908(actor);
}
