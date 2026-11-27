/*
 * func_80044E40
 *
 * Purpose:
 *   Player actor action/state update handler. Sets flags +0x3A7/+0x3D1, calls
 *   func_8005BD1C, delegates early when Scratchpad+0x31C < 17, otherwise checks
 *   ball/match/cursor conditions to either revert to state 1 or update facing,
 *   stamina/timer (Scratchpad+0xF2), movement (func_80038F18), and next state
 *   (func_80039A5C + jtbl_80011218), finishing with func_8005B908(actor).
 *
 * Arguments:
 *   actor (u8*): player actor struct pointer
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800C6D84 (u8[]) stamina/timer decrement table indexed by Scratchpad+0x328
 *   jtbl_80011218 (jump table in .rodata for next_state in 0x0D..0x29)
 *
 * Actor fields read/written:
 *   +0x38D (u8)  actor ID
 *   +0x396 (u8)  state (W)
 *   +0x397 (u8)  substate (W: 0)
 *   +0x399 (u8)  team side (0 or 1)
 *   +0x3A7 (u8)  flag (W: 1)
 *   +0x3AA (u8)  facing angle
 *   +0x3AC (u8)  turn/target param
 *   +0x3C2 (s16) timer/counter
 *   +0x3D1 (u8)  flag (W: 1)
 *
 * Scratchpad (0x1F800000) fields read/written:
 *   +0x04E (s16) ball/pitch condition checked >= -255
 *   +0x0F2 (u16) timer/stamina accumulator (R/W)
 *   +0x29A (u8)  match phase (checked == 1)
 *   +0x2A9 (u8)  ball possession actor ID (checked == 0)
 *   +0x2F4 (u32) ball pointer (reads +0x0C s32 speed/height)
 *   +0x2FC (u8)  flag (checked == 0)
 *   +0x31C (s16) threshold checked < 0x11
 *   +0x31E (u8[2*2]) active controlled actor ID per team (stride 2)
 *   +0x328 (u8)  difficulty/mode index into D_800C6D84
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800C6D84[];

extern void func_8005BD1C(unsigned char *actor);
extern void func_80049F90(unsigned char *actor);
extern unsigned int func_80052E68(unsigned char *actor);
extern void func_8005C2AC(unsigned char *actor, unsigned int a1, unsigned int a2,
                          unsigned int a3, unsigned int a4);
extern void func_80038F18(unsigned char *actor, signed short a1, unsigned int a2, unsigned int a3);
extern unsigned int func_80039A5C(unsigned char *actor);
extern void func_8005B908(unsigned char *actor);

void func_80044E40(unsigned char *actor)
{
    signed short a1_arg;
    unsigned int next_state;
    signed int ball_val;

    B(actor, 0x3A7) = 1;
    B(actor, 0x3D1) = 1;
    func_8005BD1C(actor);

    if (S(Scratchpad, 0x31C) < 0x11) {
        func_80049F90(actor);
        func_8005B908(actor);
        return;
    }

    if ((func_80052E68(actor) & 0xFFU) != 0 || B(Scratchpad, 0x2A9) != 0) {
        B(actor, 0x396) = 1;
        B(actor, 0x397) = 0;
        func_8005B908(actor);
        return;
    }

    if (S(Scratchpad, 0x4E) >= -0xFF) {
        unsigned char *ball = (unsigned char *)W(Scratchpad, 0x2F4);
        ball_val = SW(ball, 0xC);
        if (ball_val < 0) {
            ball_val = -ball_val;
        }
        if (ball_val < 0x1700) {
            B(actor, 0x396) = 1;
            B(actor, 0x397) = 0;
            func_8005B908(actor);
            return;
        }
    }

    if (B(Scratchpad, 0x29A) != 1 ||
        B(Scratchpad, 0x2FC) != 0 ||
        B(actor, 0x38D) != B(Scratchpad, 0x31E + ((unsigned int)B(actor, 0x399) << 1))) {
        B(actor, 0x396) = 1;
        B(actor, 0x397) = 0;
        func_8005B908(actor);
        return;
    }

    if (S(actor, 0x3C2) == 0) {
        func_8005C2AC(actor, 0x24, 0, B(actor, 0x3AA), B(actor, 0x3AC));
        a1_arg = 0;
        H(Scratchpad, 0xF2) = (unsigned short)(H(Scratchpad, 0xF2) - D_800C6D84[B(Scratchpad, 0x328)]);
    } else {
        func_8005C2AC(actor, 0x20, 0, B(actor, 0x3AA), B(actor, 0x3AC));
        a1_arg = (B(actor, 0x399) != 0) ? 0x180 : -0x180;
        H(Scratchpad, 0xF2) = (unsigned short)(H(Scratchpad, 0xF2) - 0x30);
    }

    func_80038F18(actor, a1_arg, 0, 0);
    next_state = func_80039A5C(actor) & 0xFFU;

    if ((unsigned int)(next_state - 0xDU) < 0x1DU) {
        switch (next_state - 0xDU) {
        case 0:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 26:
        case 28: /* .L8004503C */
            B(actor, 0x396) = (unsigned char)next_state;
            B(actor, 0x397) = 0;
            break;

        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 23:
        case 24:
        case 25:
        case 27: /* .L80045044 */
        default:
            break;
        }
    }

    func_8005B908(actor);
}
