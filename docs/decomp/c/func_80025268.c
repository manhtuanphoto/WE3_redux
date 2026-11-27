/*
 * func_80025268
 *
 * Purpose:
 *   Per-frame player dribbling, touch/knock-on, and turn/trap action state
 *   machine (dispatches on actor+0x397: 0x00, 0x10, 0x30, 0x50, 0x51, 0x60, 0x70).
 *   Handles acceleration and turning while dribbling, ball-knock/pass/trap
 *   execution on specific animation frames, and state transitions.
 *
 * Arguments:
 *   actor (unsigned char *) - pointer to 1000-byte player actor struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Actor fields:
 *     +0x38D (u8)  actor id (R)
 *     +0x390 (u8)  anim frame (R)
 *     +0x392 (u8)  team slot 0..10 (R)
 *     +0x396 (u8)  state (W)
 *     +0x397 (u8)  substate (R/W)
 *     +0x398 (u8)  side 0/1 (R)
 *     +0x399 (u8)  attack direction flag (R)
 *     +0x39B (u8)  sprint/dash flag (R)
 *     +0x3A0 (u32) speed (R/W)
 *     +0x3A4 (u8)  move heading (W)
 *     +0x3A7 (u8)  set to 1 on entry (W)
 *     +0x3AA (u8)  body heading (R/W)
 *     +0x3AB (u8)  target heading (R)
 *     +0x3AC (u8)  mirror/foot flag (R)
 *     +0x3AE (u16) timer/counter decremented by 2 on entry (R/W)
 *     +0x3B0 (u8)  dribble touch countdown (R/W)
 *     +0x3B3 (u8)  dribble/move max speed cap (R)
 *     +0x3C6 (s16) queued next state (0xFF -> state 1, substate 2) (R)
 *     +0x3C8 (u8)  turn step rate (R)
 *     +0x3CA (s16/u16) touch power / timer offset in substate 0x10 (R)
 *     +0x3CC (s16) target end anim frame in substates 0x10 & 0x30 (R)
 *     +0x3D0 (u8)  post-kick cooldown timer (W)
 *     +0x3D1 (u8)  cleared on ball kick in substates 0x10, 0x30, 0x50 (W)
 *   Scratchpad (0x1F800000):
 *     +0x2A8 (u8)  last-touch actor id (R)
 *     +0x2A9 (u8)  ball owner (W)
 *     +0x2F4 (u8*) pointer to ball actor (R) -> writes ball+0x3B0 = 4 in substate 0x00
 *     +0x31C (s16) ball timer / frames since last touch (W)
 *     +0x332 (u8)  touch/dribble counter incremented in substate 0x00 (R/W)
 *   Globals:
 *     D_8010C32A (u16[]) player stat entry (indexed by side*264 + slot*12, low nibble 0..15) (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x22 / 0x23 = 34 / 35 raw speed cap during early anim frames (< 3)
 *   0x80 = 128 frames (~4.27 s) ball timer value set on state finish
 *   0x0A = 10 raw min speed, 0x1B = 27 raw deceleration threshold in substate 0x50
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_8010C32A[];

extern void func_8005BD1C(unsigned char*);
extern unsigned int func_8005B7CC(unsigned int, unsigned int, unsigned int);
extern unsigned int func_80045504(unsigned char*, int);
extern void func_8005BA30(unsigned char*, int, int);
extern unsigned int func_80049828(unsigned char*, int);
extern int func_8001E6B8(int);
extern void func_8004B0F0(unsigned char*, int, int, int);
extern void func_8005B908(unsigned char*);

void func_80025268(unsigned char* actor) {
    unsigned char substate;
    int kickHeading;

    substate = B(actor, 0x397);
    B(actor, 0x3A7) = 1;
    H(actor, 0x3AE) = H(actor, 0x3AE) - 2;

    switch (substate) {
    case 0x00: {
        unsigned int heading;
        unsigned int maxSpd;
        unsigned int spd;

        func_8005BD1C(actor);
        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), B(actor, 0x3C8));
        B(actor, 0x3AA) = (unsigned char)heading;
        B(actor, 0x3A4) = (unsigned char)heading;
        S(Scratchpad, 0x31C) = (signed short)((B(actor, 0x399) ^ 1) + 2);

        if (B(actor, 0x39B) != 0) {
            unsigned char cap = B(actor, 0x3B3);
            spd = W(actor, 0x3A0) + 4 + cap;
            maxSpd = (unsigned int)(cap - 3);
        } else {
            unsigned char cap = B(actor, 0x3B3);
            spd = W(actor, 0x3A0) + ((cap / 6) & 0xFF);
            maxSpd = cap;
        }
        W(actor, 0x3A0) = spd;
        if (maxSpd < spd) {
            W(actor, 0x3A0) = maxSpd;
        }

        if ((func_80045504(actor, 2) & 0xFF) == 0) {
            B(actor, 0x3B0) = B(actor, 0x3B0) - 1;
            if (B(actor, 0x3B0) == 0 || B(actor, 0x3AA) == B(actor, 0x3AB)) {
                int stat;
                B(Scratchpad, 0x332) = B(Scratchpad, 0x332) + 1;
                func_8005BA30(actor, 0x34, B(actor, 0x390));
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 0;

                stat = H(D_8010C32A, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) & 0xF;
                if (stat < 7) {
                    int arg1 = ((int)(W(actor, 0x3A0) + B(actor, 0x3B3) * 3) >> 2) - stat + 0x10;
                    int power = func_80049828(actor, arg1 & 0xFFFF);
                    int lift = func_8001E6B8(3);
                    func_8004B0F0(actor, B(actor, 0x3AB), power & 0xFF, lift);
                    B(Scratchpad, 0x2A9) = B(actor, 0x38D);
                    stat = H(D_8010C32A, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) & 0xF;
                    B(actor, 0x3D0) = (unsigned char)(10 - stat);
                    B((unsigned char*)W(Scratchpad, 0x2F4), 0x3B0) = 4;
                    B(actor, 0x397) = 2;
                }
                S(Scratchpad, 0x31C) = 0x80; /* 128 frames */
            }
        }
        break;
    }

    case 0x10:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 3 && B(actor, 0x38D) == B(Scratchpad, 0x2A8)) {
            int power;
            kickHeading = B(actor, 0x3AC) ? (B(actor, 0x3AB) + 2) : (B(actor, 0x3AB) - 2);
            power = func_80049828(actor, 0x34);
            func_8004B0F0(actor, kickHeading & 0xFF, (power & 0xFF) + S(actor, 0x3CA), 0);
            B(Scratchpad, 0x2A9) = B(actor, 0x38D);
            S(Scratchpad, 0x31C) = (signed short)((B(actor, 0x399) ^ 1) - H(actor, 0x3CA));
            B(actor, 0x3D1) = 0;
        }
        if (B(actor, 0x390) >= 3) {
            unsigned int heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 10);
            signed int cap = (signed int)B(actor, 0x3B3) - 8;
            unsigned int spd = W(actor, 0x3A0) + (cap / 6);
            B(actor, 0x3AA) = (unsigned char)heading;
            B(actor, 0x3A4) = (unsigned char)heading;
            W(actor, 0x3A0) = spd;
            if ((unsigned int)cap < spd) {
                W(actor, 0x3A0) = (unsigned int)cap;
            }
        } else {
            unsigned int spd = W(actor, 0x3A0) + 2;
            W(actor, 0x3A0) = spd;
            if (spd >= 0x23) {
                W(actor, 0x3A0) = 0x22;
            }
        }
        if ((func_80045504(actor, 3) & 0xFF) == 0 && B(actor, 0x390) == S(actor, 0x3CC)) {
            func_8005BA30(actor, 0x34, 8);
            S(Scratchpad, 0x31C) = 0x80;
            if (S(actor, 0x3C6) == 0xFF) {
                B(actor, 0x396) = 1;
                B(actor, 0x3D0) = 0;
                B(actor, 0x397) = 2;
            } else {
                B(actor, 0x396) = (unsigned char)S(actor, 0x3C6);
                B(actor, 0x397) = 0;
            }
        }
        break;

    case 0x30:
        func_8005BD1C(actor);
        if (B(actor, 0x390) == 3 && B(actor, 0x38D) == B(Scratchpad, 0x2A8)) {
            int power;
            kickHeading = B(actor, 0x3AC) ? (B(actor, 0x3AB) - 4) : (B(actor, 0x3AB) + 4);
            power = func_80049828(actor, 0x2E);
            func_8004B0F0(actor, kickHeading & 0xFF, power & 0xFF, 0);
            B(Scratchpad, 0x2A9) = B(actor, 0x38D);
            S(Scratchpad, 0x31C) = (signed short)(B(actor, 0x399) ^ 1);
            B(actor, 0x3D1) = 0;
        }
        if (B(actor, 0x390) >= 3) {
            unsigned int heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 12);
            signed int cap = (signed int)B(actor, 0x3B3) - 12;
            unsigned int spd = W(actor, 0x3A0) + (cap / 6);
            B(actor, 0x3AA) = (unsigned char)heading;
            B(actor, 0x3A4) = (unsigned char)heading;
            W(actor, 0x3A0) = spd;
            if ((unsigned int)cap < spd) {
                W(actor, 0x3A0) = (unsigned int)cap;
            }
        } else {
            unsigned int spd = W(actor, 0x3A0) + 2;
            W(actor, 0x3A0) = spd;
            if (spd >= 0x23) {
                W(actor, 0x3A0) = 0x22;
            }
        }
        if ((func_80045504(actor, 3) & 0xFF) == 0 && B(actor, 0x390) == S(actor, 0x3CC)) {
            func_8005BA30(actor, 0x26, 8);
            S(Scratchpad, 0x31C) = 0x80;
            if (S(actor, 0x3C6) == 0xFF) {
                B(actor, 0x396) = 1;
                B(actor, 0x3D0) = 0;
                B(actor, 0x397) = 2;
            } else {
                B(actor, 0x396) = (unsigned char)S(actor, 0x3C6);
                B(actor, 0x397) = 0;
            }
        }
        break;

    case 0x50: {
        unsigned int spd;
        func_8005BD1C(actor);
        if (B(actor, 0x390) >= 3) {
            B(actor, 0x3AA) = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 14);
        }
        spd = W(actor, 0x3A0);
        W(actor, 0x3A0) = (spd < 0x1B) ? 0x0A : (spd - 0x10);
        if (B(actor, 0x390) == 4) {
            if (B(actor, 0x38D) == B(Scratchpad, 0x2A8)) {
                int power = func_80049828(actor, 0x26);
                func_8004B0F0(actor, B(actor, 0x3AB), power & 0xFF, 0);
                B(Scratchpad, 0x2A9) = B(actor, 0x38D);
                S(Scratchpad, 0x31C) = (signed short)((B(actor, 0x399) ^ 1) - 4);
                B(actor, 0x3D1) = 0;
            }
            B(actor, 0x397) = B(actor, 0x397) + 1;
            B(actor, 0x3A4) = B(actor, 0x3AB);
        } else {
            B(actor, 0x3A4) = B(actor, 0x3AA);
        }
        func_80045504(actor, 3);
        break;
    }

    case 0x51:
        func_8005BD1C(actor);
        B(actor, 0x3AA) = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 12);
        if ((func_80045504(actor, 3) & 0xFF) == 0 && B(actor, 0x390) == 0) {
            func_8005BA30(actor, 0x34, 9);
            S(Scratchpad, 0x31C) = 0x80;
            if (S(actor, 0x3C6) == 0xFF) {
                B(actor, 0x3D0) = 0;
                B(actor, 0x396) = 1;
                B(actor, 0x397) = 2;
            } else {
                B(actor, 0x396) = (unsigned char)S(actor, 0x3C6);
                B(actor, 0x397) = 0;
            }
        }
        break;

    case 0x60:
    case 0x70: {
        unsigned int heading;
        func_8005BD1C(actor);
        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), B(actor, 0x3C8));
        B(actor, 0x3AA) = (unsigned char)heading;
        B(actor, 0x3A4) = (unsigned char)heading;
        if (B(actor, 0x390) == 9) {
            B(actor, 0x396) = 0;
            B(actor, 0x397) = 0;
            B(actor, 0x3AA) = B(actor, 0x3AB);
        }
        func_80045504(actor, 2);
        break;
    }

    default:
        break;
    }

    func_8005B908(actor);
}
