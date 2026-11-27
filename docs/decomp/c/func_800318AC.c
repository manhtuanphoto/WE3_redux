/*
 * func_800318AC
 *
 * Purpose:
 *   Updates a player actor's long-ball / cross / through-pass or clearance
 *   action state machine across substates (+0x397 = 0..3):
 *   - Substate 0: sets +0x3D1 = 4 and +0x3C6 = 3; if actor is last-touch
 *     (Scratchpad+0x2A8), transitions directly to substate 2 (setting running
 *     or idle animation), else transitions to substate 1; copies +0x3AA to
 *     +0x3A4 and updates animation via func_8005BD1C.
 *   - Substate 1: charges +0x3C6 by +2 while input mask (+0x3D4 & Scratchpad+0x1A[side])
 *     is held, steers via func_80047F6C(actor, 1), and transitions to substate 2
 *     when func_8005385C(actor, 5) succeeds.
 *   - Substate 2: decelerates/steers when moving (+0x3A0 != 0), charges +0x3C6 by +2
 *     up to 0x10 while input mask is held, otherwise runs func_80031B58(actor)
 *     and checks +0x3C2 (0xFE -> state 0xA substate 1; 0xFF -> state 9 substate 0
 *     with +0x3CC=1, +0x3C4=0, +0x3B1=0 and func_80034470).
 *   - Substate 3: turns +0x3AA toward +0x3AB (rate 10); at animation frame 0x1A,
 *     if func_80053B44(actor) != 0 calls func_8003257C(actor), then transitions
 *     to state 4 / substate 4 and syncs +0x3AB = +0x3AA.
 *   - Epilogue: calls func_8002A83C(actor, +0x3C6) and func_8005B908(actor).
 *
 * Arguments:
 *   actor (unsigned char *) - pointer to 1000-byte player actor struct
 *
 * Return value:
 *   None (void)
 *
 * Actor fields read/written:
 *   +0x38D (u8)  - actor ID
 *   +0x38E (u16) - current animation ID
 *   +0x390 (u8)  - current animation frame
 *   +0x396 (u8)  - actor state (set to 4, 9, or 0xA on transitions)
 *   +0x397 (u8)  - actor substate (0..3 -> 1, 2, 0, or 4)
 *   +0x398 (u8)  - team side (0/1)
 *   +0x3A0 (u32) - movement speed
 *   +0x3A4 (u8)  - move heading
 *   +0x3A7 (u8)  - set to 1
 *   +0x3AA (u8)  - body heading
 *   +0x3AB (u8)  - target heading
 *   +0x3B1 (u8)  - cleared to 0 on +0x3C2 == 0xFF
 *   +0x3C2 (s16) - action outcome code (0xFE or 0xFF checked)
 *   +0x3C4 (u16) - cleared to 0 on +0x3C2 == 0xFF
 *   +0x3C6 (s16) - charge gauge (init 3, incremented by 2 up to 0x10)
 *   +0x3CC (u16) - set to 1 on +0x3C2 == 0xFF
 *   +0x3D1 (u8)  - set to 4 in substate 0
 *   +0x3D4 (u16) - input bitmask tested against Scratchpad+0x1A[side]
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x1A[side] (u16) - per-side held/trigger button mask
 *   +0x2A8      (u8)  - last-touch actor ID
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SCRATCHPAD ((unsigned char *)0x1F800000)

extern void func_8005BA78(unsigned char *actor, unsigned int anim, unsigned int frame);
extern void func_8005BA30(unsigned char *actor, unsigned int anim, unsigned int frame);
extern void func_8005BD1C(unsigned char *actor);
extern void func_80047F6C(unsigned char *actor, int a1);
extern int func_8005385C(unsigned char *actor, int t);
extern void func_8004840C(unsigned char *actor);
extern void func_800479B8(unsigned char *actor, int a1);
extern void func_80031B58(unsigned char *actor);
extern void func_80034470(unsigned char *actor);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);
extern int func_80053B44(unsigned char *actor);
extern void func_8003257C(unsigned char *actor);
extern void func_8002A83C(unsigned char *actor, unsigned int charge);
extern void func_8005B908(unsigned char *actor);

void func_800318AC(unsigned char *actor)
{
    unsigned char *scratch = SCRATCHPAD;
    unsigned char substate;

    B(actor, 0x3A7) = 1;
    substate = B(actor, 0x397);

    switch (substate) {
    case 0:
        B(actor, 0x3D1) = 4;
        H(actor, 0x3C6) = 3;
        if (B(actor, 0x38D) == B(scratch, 0x2A8)) {
            if (W(actor, 0x3A0) != 0) {
                func_8005BA78(actor, 0x26, B(actor, 0x390));
            } else if (H(actor, 0x38E) != 1) {
                func_8005BA30(actor, 1, 0);
            }
            B(actor, 0x397) = 2;
        } else {
            B(actor, 0x397) = 1;
        }
        B(actor, 0x3A4) = B(actor, 0x3AA);
        func_8005BD1C(actor);
        break;

    case 1:
        func_8005BD1C(actor);
        if ((H(scratch, 0x1A + (unsigned int)B(actor, 0x398) * 2U) & H(actor, 0x3D4)) != 0) {
            H(actor, 0x3C6) = H(actor, 0x3C6) + 2;
        }
        func_80047F6C(actor, 1);
        if (func_8005385C(actor, 5) != 0) {
            B(actor, 0x397) = 2;
        }
        break;

    case 2:
        func_8005BD1C(actor);
        if (W(actor, 0x3A0) != 0) {
            func_8004840C(actor);
            func_800479B8(actor, 1);
        }
        if ((H(scratch, 0x1A + (unsigned int)B(actor, 0x398) * 2U) & H(actor, 0x3D4)) != 0 &&
            S(actor, 0x3C6) < 0x10) {
            H(actor, 0x3C6) = H(actor, 0x3C6) + 2;
        } else {
            signed short res;

            func_80031B58(actor);
            res = S(actor, 0x3C2);
            if (res == 0xFE) {
                B(actor, 0x396) = 0xA;
                B(actor, 0x397) = 1;
            } else if (res == 0xFF) {
                H(actor, 0x3CC) = 1;
                H(actor, 0x3C4) = 0;
                B(actor, 0x3B1) = 0;
                B(actor, 0x396) = 9;
                B(actor, 0x397) = 0;
                func_80034470(actor);
            }
        }
        break;

    case 3:
        func_8005BD1C(actor);
        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 10);
        if (B(actor, 0x390) == 0x1A) {
            if (func_80053B44(actor) != 0) {
                func_8003257C(actor);
            }
            B(actor, 0x396) = 4;
            B(actor, 0x397) = 4;
            B(actor, 0x3AB) = B(actor, 0x3AA);
        }
        break;

    default:
        break;
    }

    func_8002A83C(actor, B(actor, 0x3C6));
    func_8005B908(actor);
}
