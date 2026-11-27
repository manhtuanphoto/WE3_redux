/*
 * func_80026D14
 *
 * Purpose:
 *   Updates a player actor during a slide/dive recovery or fall-and-get-up
 *   state machine (driven by actor+0x397: cases 0..2 and 16..18), applying
 *   deceleration, random recovery delay, movement step, and ball contact/kick
 *   handling.
 *
 * Arguments:
 *   actor (u8 *): pointer to 1000-byte actor struct
 *
 * Return value:
 *   void
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x240, +0x241 (u8) special/highlighted actor IDs
 *   +0x29A (u8) match phase (1 = open play, 7 = checked for early exit)
 *   +0x2A8 (u8) last-touch actor ID
 *   +0x2A9 (u8) ball owner actor ID
 *   +0x2AB (u8) previous contact actor ID
 *   +0x31C (s16) ball timer (frames since last touch)
 *
 * Scratchpad fields written:
 *   +0x2A9 (u8) cleared to 0 on ball contact
 *   +0x2AA (u8) set to opposing GK ID via func_8005CE30(actor+0x38D)
 *   +0x2AB (u8) set to actor+0x38D
 *   +0x2B0 (u32) ball event code set to 0xE on tackle/deflection contact
 *   +0x2CF[2] (u8) possession flag for actor's side set to 1
 *
 * Actor fields read:
 *   +0x38D (u8) actor ID, +0x390 (u8) anim frame, +0x397 (u8) substate
 *   +0x398 (u8) side (0/1), +0x3A0 (u32/u16) speed, +0x3AA (u8) body heading
 *   +0x3AC (u8) mirror/foot, +0x3AE (u16) stamina/timer, +0x3B0 (u8) delay timer
 *
 * Actor fields written:
 *   +0x26 (u16) model rotation, +0x396 (u8) state (set to 0x11 on finish),
 *   +0x397 (u8) substate, +0x39C (u8) set to 1, +0x3A0 (u32) speed,
 *   +0x3AA (u8) body heading, +0x3AD (u8) cleared to 0, +0x3AE (u16) -= 4,
 *   +0x3B0 (u8) delay timer, +0x3D1 (u8) cleared to 0
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x28 (40) min speed = 0.16 m/frame (4.8 m/s);
 *   0x48 (72) kick power in func_8004B0F0;
 *   0x10..0x17 (16..23 frames = 0.53..0.77 s) or 0x20..0x37 / 0x40..0x4F delay;
 *   0x11 (17 frames = 0.57 s) ball re-contact cooldown;
 *   0xC (12) touch radius, -0x18 (-24) height threshold in func_8009462C.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern void func_800512B8(unsigned char *actor, int mode);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_8005BD1C(unsigned char *actor);
extern int func_8001E6B8(int n);
extern void func_8005B864(unsigned char *actor, unsigned short speed);
extern void func_800498AC(unsigned char *actor);
extern int func_8005C7A0(int n);
extern void func_8004B0F0(unsigned char *actor, unsigned int heading, unsigned int power, unsigned int lift);
extern unsigned int func_8009462C(unsigned char *actor, int radius, unsigned int heading, int height, int arg4);
extern unsigned int func_8005CE30(unsigned int actorId);

void func_80026D14(unsigned char *actor)
{
    unsigned char id = B(actor, 0x38D);

    if (id == B(Scratchpad, 0x240) || id == B(Scratchpad, 0x241)) {
        func_800512B8(actor, 2);
    }

    B(actor, 0x39C) = 1;
    B(actor, 0x3D1) = 0;
    H(actor, 0x3AE) = (unsigned short)(H(actor, 0x3AE) - 4);

    switch (B(actor, 0x397)) {
    case 0:
        func_8005BA30(actor, 0x29, 0);
        if (W(actor, 0x3A0) < 0x28) {
            W(actor, 0x3A0) = 0x28;
        }
        B(actor, 0x397) = B(actor, 0x397) + 1;
        break;

    case 1:
        func_8005BD1C(actor);
        if (B(actor, 0x390) >= 0xA) {
            W(actor, 0x3A0) = (W(actor, 0x3A0) * 9U) / 10U;
        }
        if (B(actor, 0x390) == 0x1C) {
            if (B(Scratchpad, 0x29A) == 1) {
                B(actor, 0x3B0) = (unsigned char)(func_8001E6B8(8) + 0x10);
            } else {
                B(actor, 0x3B0) = (unsigned char)(func_8001E6B8(0x18) + 0x20);
            }
            W(actor, 0x3A0) = 0;
            B(actor, 0x397) = B(actor, 0x397) + 1;
        }
        break;

    case 2:
        B(actor, 0x3B0) = B(actor, 0x3B0) - 1;
        if (B(actor, 0x3B0) == 0 || B(Scratchpad, 0x29A) == 7) {
            B(actor, 0x396) = 0x11;
            B(actor, 0x397) = 0;
        }
        break;

    case 16:
        func_8005BA30(actor, 0x27, 0);
        if (W(actor, 0x3A0) < 0x28) {
            W(actor, 0x3A0) = 0x28;
        }
        B(actor, 0x397) = B(actor, 0x397) + 1;
        break;

    case 17:
        func_8005BD1C(actor);
        switch (B(actor, 0x390)) {
        case 15:
            W(actor, 0x3A0) >>= 1;
            break;
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
            W(actor, 0x3A0) = (W(actor, 0x3A0) * 244U) >> 8;
            break;
        case 40:
            W(actor, 0x3A0) = 0;
            break;
        case 53:
            if (B(Scratchpad, 0x29A) == 1) {
                B(actor, 0x3B0) = (unsigned char)(func_8001E6B8(8) + 0x10);
            } else {
                B(actor, 0x3B0) = (unsigned char)(func_8001E6B8(0x10) + 0x40);
            }
            B(actor, 0x397) = B(actor, 0x397) + 1;
            break;
        }
        break;

    case 18:
        B(actor, 0x3B0) = B(actor, 0x3B0) - 1;
        if (B(actor, 0x3B0) == 0 || B(Scratchpad, 0x29A) == 7) {
            unsigned char heading = B(actor, 0x3AA);
            int turn;

            if (B(actor, 0x3AC) == 0) {
                heading = (unsigned char)(heading + 0x40);
            } else {
                heading = (unsigned char)(heading - 0x40);
            }
            B(actor, 0x3AA) = heading;
            turn = 0x1C0 - heading;
            H(actor, 0x26) = (unsigned short)((turn % 256) * 16);
            func_8005BA30(actor, 0x21, 0);
            B(actor, 0x3AD) = 0;
            B(actor, 0x396) = 0x11;
            B(actor, 0x397) = 0;
        }
        break;
    }

    func_8005B864(actor, H(actor, 0x3A0));
    func_800498AC(actor);

    id = B(actor, 0x38D);
    if (id == B(Scratchpad, 0x2A8)) {
        int jitter = func_8005C7A0(0x20);

        func_8004B0F0(actor, (B(actor, 0x3AA) + jitter) & 0xFF, 0x48, 2);
        B(Scratchpad, 0x2CF + B(actor, 0x398)) = 1;
        B(Scratchpad, 0x2AA) = (unsigned char)func_8005CE30(B(actor, 0x38D));
        B(Scratchpad, 0x2AB) = B(actor, 0x38D);
    } else if (id == B(Scratchpad, 0x2A9)) {
        B(Scratchpad, 0x2A9) = 0;
        B(Scratchpad, 0x2CF + B(actor, 0x398)) = 1;
        B(Scratchpad, 0x2AA) = (unsigned char)func_8005CE30(B(actor, 0x38D));
        B(Scratchpad, 0x2AB) = B(actor, 0x38D);
    } else {
        if (B(Scratchpad, 0x2AB) == id && S(Scratchpad, 0x31C) < 0x11) {
            return;
        }
        if ((func_8009462C(actor, 0xC, B(actor, 0x3AA), -0x18, 2) & 0xFF) != 0) {
            W(Scratchpad, 0x2B0) = 0xE;
            B(Scratchpad, 0x2AA) = (unsigned char)func_8005CE30(B(actor, 0x38D));
            B(Scratchpad, 0x2AB) = B(actor, 0x38D);
        }
    }
}
