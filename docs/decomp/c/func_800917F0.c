/*
 * func_800917F0
 *
 * Purpose:
 *   Goalkeeper diving save / airborne catch state machine (substates 0 and 1 on
 *   actor+0x397, anim 0x6B). Evaluates ball trajectory cancellation before launch,
 *   computes dive velocity vector from Scratchpad+0x0F0..+0x0F4 toward target
 *   actor+0x3C2..+0x3C6 capped by GK stat, steps airborne arc & catch checks
 *   (func_80094EEC / func_80095E8C) across anim frames 1..13+, applies drag on
 *   landing, and triggers ball/body post-step adjustment (func_8009462C).
 *
 * Arguments:
 *   actor (unsigned char *) - pointer to 1000-byte goalkeeper actor struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Actor fields:
 *     +0x002 (s16) x, +0x004 (s16) y (height), +0x006 (s16) z (R/W)
 *     +0x008 (s32) vx, +0x00C (s32) vy, +0x010 (s32) vz (R/W)
 *     +0x026 (u16) visual rotation (W)
 *     +0x38D (u8)  actor id (R)
 *     +0x390 (u8)  anim frame (R)
 *     +0x392 (u8)  team slot (R)
 *     +0x396 (u8)  state (W: 0x96 abort, 0x84/0x83 post-dive)
 *     +0x397 (u8)  substate (R/W)
 *     +0x398 (u8)  side 0/1 (R)
 *     +0x3A0 (u32) speed (R)
 *     +0x3A4 (u8)  move heading (W)
 *     +0x3AA (u8)  body heading (R/W)
 *     +0x3AB (u8)  target heading (R)
 *     +0x3AC (u8)  mirror/side flag (R)
 *     +0x3B9 (u8)  GK reaction/skill stat*10 (R)
 *     +0x3C2/+0x3C4/+0x3C6 (s16) target intercept x/y/z (R)
 *     +0x3D0 (u8)  reaction/hold flag (R)
 *   Scratchpad (0x1F800000):
 *     +0x0F0/+0x0F2/+0x0F4 (s16) start/reference xyz (R)
 *     +0x2A8 (u8)  last-touch actor id (R)
 *     +0x2F4 (u8*) pointer to ball actor (R)
 *     +0x31C (s16) ball timer, frames since last touch (R/W)
 *   Globals:
 *     D_8010C32F (u8[]) GK dive distance stat nibble (side*264 + slot*12) (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x13 = 19 frames (~0.63 s) reaction window minus (stat / 10)
 *   0x40 = 64 angle units (90 deg) abort threshold vs ball heading
 *   0x50 = 80 raw (0.32 m) ball catch/touch radius in func_80094EEC
 *   0x26 / 0x2C = 38 / 44 raw max dive step speed base (0x30 = 48 speed threshold)
 *   94/100 (frames 10..12), 7/10 (frame 13), 88/100 (ground slide) horizontal velocity decay
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_8010C32F[];

extern void func_8006EC5C(unsigned char*);
extern unsigned int func_8005B36C(int, int, int, int);
extern void func_8005C2AC(unsigned char*, int, int, int, int);
extern unsigned int func_800B1128(int);
extern int func_8001E694(unsigned int, int);
extern int func_8001E5A4(unsigned int, int);
extern void func_8005BA30(unsigned char*, int, int);
extern unsigned int func_8005B7CC(unsigned int, unsigned int, unsigned int);
extern void func_8005BD1C(unsigned char*);
extern int func_80094EEC(unsigned char*, int);
extern int func_8001E6B8(int);
extern void func_80095E8C(unsigned char*);
extern void func_800954E8(unsigned char*);
extern int func_80095F40(void);
extern void func_8009462C(unsigned char*, int, unsigned int, int, int);

void func_800917F0(unsigned char* actor) {
    unsigned char substate = B(actor, 0x397);

    switch (substate) {
    case 0: {
        int threshold = 19 - (int)(B(actor, 0x3B9) / 10);
        int dx, dz;
        unsigned int dist;
        unsigned int stepSpd;
        unsigned int maxStep;
        unsigned int dir;
        unsigned int heading;
        int turn;

        if (S(Scratchpad, 0x31C) < threshold && B(actor, 0x3D0) != 0) {
            func_8006EC5C(actor);
            if (S(Scratchpad, 0x31C) != 1) {
                return;
            }
            dir = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(actor, 0x3C2), S(actor, 0x3C6));
            {
                unsigned char ballHeading = B((unsigned char*)W(Scratchpad, 0x2F4), 0x3A4);
                unsigned int diff = (dir - ballHeading) & 0xFF;
                if (diff < 0x81) {
                    if (diff >= 0x40) {
                        B(actor, 0x396) = 0x96;
                        B(actor, 0x397) = 0;
                        return;
                    }
                } else if (((ballHeading - dir) & 0xFF) >= 0x40) {
                    B(actor, 0x396) = 0x96;
                    B(actor, 0x397) = 0;
                    return;
                }
            }
        }

        func_8005C2AC(actor, 0x6B, 9, B(actor, 0x3AB), B(actor, 0x3AC));

        dx = S(actor, 0x3C2) - S(Scratchpad, 0x0F0);
        dz = S(actor, 0x3C6) - S(Scratchpad, 0x0F4);
        dist = func_800B1128(dx * dx + dz * dz);
        stepSpd = dist / 10;

        {
            int stat = B(D_8010C32F, B(actor, 0x398) * 264 + B(actor, 0x392) * 12) & 0xF;
            maxStep = (W(actor, 0x3A0) < 0x30) ? ((stat >> 1) + 0x26) : ((stat >> 1) + 0x2C);
            if (stepSpd > maxStep) {
                stepSpd = maxStep;
            }
        }

        dir = func_8005B36C(S(Scratchpad, 0x0F0), S(Scratchpad, 0x0F4), S(actor, 0x3C2), S(actor, 0x3C6));
        B(actor, 0x3A4) = (unsigned char)dir;
        SW(actor, 0x08) = func_8001E694(dir & 0xFF, (int)stepSpd);
        SW(actor, 0x0C) = (S(actor, 0x3C4) - S(Scratchpad, 0x0F2)) / 5;
        SW(actor, 0x10) = func_8001E5A4(dir & 0xFF, (int)stepSpd);

        func_8005BA30(actor, 0x6B, 0);
        heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 0x40);
        B(actor, 0x3AA) = (unsigned char)heading;
        turn = 0x1C0 - (int)(heading & 0xFF);
        H(actor, 0x26) = (unsigned short)((turn % 256) * 16);

        S(actor, 0x04) = 0;
        S(actor, 0x02) = (signed short)(S(actor, 0x02) + SW(actor, 0x08));
        S(actor, 0x06) = (signed short)(S(actor, 0x06) + SW(actor, 0x10));
        B(actor, 0x397) = B(actor, 0x397) + 1;
        break;
    }

    case 1: {
        unsigned int frameIdx;
        func_8005BD1C(actor);
        frameIdx = (unsigned int)(B(actor, 0x390) - 1);

        if (frameIdx < 13) {
            switch (frameIdx) {
            case 4:
            case 5:
            case 6:
            case 7:
                S(actor, 0x04) = (signed short)(S(actor, 0x04) + SW(actor, 0x0C));
                /* fall through */
            case 0:
            case 1:
            case 2:
            case 3: {
                unsigned int heading;
                int turn;
                S(actor, 0x02) = (signed short)(S(actor, 0x02) + SW(actor, 0x08));
                S(actor, 0x06) = (signed short)(S(actor, 0x06) + SW(actor, 0x10));
                heading = func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), 12);
                B(actor, 0x3AA) = (unsigned char)heading;
                turn = 0x1C0 - (int)(heading & 0xFF);
                H(actor, 0x26) = (unsigned short)((turn % 256) * 16);
                break;
            }

            case 8:
                S(actor, 0x02) = (signed short)(S(actor, 0x02) + SW(actor, 0x08));
                S(actor, 0x04) = (signed short)(S(actor, 0x04) + SW(actor, 0x0C));
                S(actor, 0x06) = (signed short)(S(actor, 0x06) + SW(actor, 0x10));
                if (func_80094EEC(actor, 0x50) != 0) {
                    int r = func_8001E6B8((B(actor, 0x3B9) / 10) + 4);
                    S(Scratchpad, 0x31C) = (signed short)(S(Scratchpad, 0x31C) + r);
                    func_80095E8C(actor);
                }
                break;

            case 9:
            case 10:
            case 11:
                if (func_80094EEC(actor, 0x50) != 0) {
                    int r = func_8001E6B8((B(actor, 0x3B9) / 10) + 4);
                    S(Scratchpad, 0x31C) = (signed short)(S(Scratchpad, 0x31C) + r);
                    func_80095E8C(actor);
                }
                S(actor, 0x02) = (signed short)(S(actor, 0x02) + SW(actor, 0x08));
                S(actor, 0x06) = (signed short)(S(actor, 0x06) + SW(actor, 0x10));
                S(actor, 0x04) = (signed short)(S(actor, 0x04) - SW(actor, 0x0C));
                SW(actor, 0x08) = (SW(actor, 0x08) * 94) / 100;
                SW(actor, 0x10) = (SW(actor, 0x10) * 94) / 100;
                break;

            case 12:
                SW(actor, 0x08) = (SW(actor, 0x08) * 7) / 10;
                SW(actor, 0x10) = (SW(actor, 0x10) * 7) / 10;
                goto ground_slide;
            }
        } else {
        ground_slide:
            S(actor, 0x04) = 0;
            S(actor, 0x02) = (signed short)(S(actor, 0x02) + SW(actor, 0x08));
            S(actor, 0x06) = (signed short)(S(actor, 0x06) + SW(actor, 0x10));
            SW(actor, 0x08) = (SW(actor, 0x08) * 88) / 100;
            SW(actor, 0x10) = (SW(actor, 0x10) * 88) / 100;
            if (B(actor, 0x390) == 0x16 && B(actor, 0x38D) == B(Scratchpad, 0x2A8)) {
                B(actor, 0x396) = 0x84;
                B(actor, 0x397) = 0;
            } else if (B(actor, 0x390) == 0x1B) {
                B(actor, 0x396) = 0x83;
                B(actor, 0x397) = 0;
            }
        }
        break;
    }

    default:
        break;
    }

    func_800954E8(actor);
    {
        int randAng = func_8001E6B8(0x24) + 0x20;
        int ang = B(actor, 0x3AC) ? (B(actor, 0x3AB) + randAng) : (B(actor, 0x3AB) - randAng);
        int arg1 = (B(actor, 0x390) == 9) ? 10 : 12;
        int arg3 = func_8001E6B8(0x10) + 8;
        int arg4 = func_80095F40();
        func_8009462C(actor, arg1, (unsigned int)(ang & 0xFF), arg3, arg4);
    }
}
