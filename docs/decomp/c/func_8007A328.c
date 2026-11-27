/*
 * func_8007A328
 *
 * Purpose:
 *   Set-piece / dead-ball reset and transition controller: initializes all 22
 *   outfield/GK player states for the current match phase (Scratchpad+0x29A),
 *   updates AI/set-piece subsystems, checks replay/camera countdown
 *   (Scratchpad+0x294), and transitions into the next match phase once ready.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   void
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x294 (u16) set-piece transition countdown timer
 *   +0x29A (u8)  match phase (3, 4, 9, 0xC handled)
 *   +0x2AA (u8)  previous ball owner ID (1 or 12 checked)
 *   +0x2AD (u8)  kicker/actor index (% 24)
 *   +0x2B0 (u32) ball event code (4, 9, 10, 11, 13 checked)
 *   +0x2BC (s16) ball X coordinate at event
 *   +0x2BE (s16) ball Z coordinate at event
 *   +0x2C4 (u32) camera/cutscene lock flag
 *   +0x2C8 (s32) elapsed cutscene frames (< 0xB4 = 180 frames = 6.0 s)
 *   +0x2E1 (u8)  mode (5 = replay/penalty/demo mode)
 *
 * Scratchpad fields written:
 *   +0x1EC (u8)  cleared to 0 on phase 0xB transition
 *   +0x294 (u16) decremented while non-zero
 *   +0x2A1 (u8)  cleared to 0 on phase 0xB transition
 *   +0x2C2 (u8)  cleared to 0 on phase transition
 *
 * Global / Actor fields read/written:
 *   D_800FF748 (actor array, stride 0x3E8 = 1000 bytes, indices 1..22):
 *     +0x396 (u8) state (set to 0x2C, 0x2F, 0x2D, or cleared to 0),
 *     +0x397 (u8) substate (cleared to 0),
 *     +0x39A (u8) control/AI flag
 *   D_800FF748 + 0xAC76 (u8) cleared to 0 on transition
 *   D_800EF9C8 (u8) set-piece result/sub-mode (1, 2, or 3)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0xB4 (180 frames) = 6.0 s;
 *   0x8B7 (2231) = 8.924 m; 0x12F5 (4853) = 19.412 m; 0x1990 (6544) = 26.176 m.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_800EF9C8;

extern int func_800537D0(unsigned char *actor);
extern void func_8009F62C(int state);
extern void func_8002283C(void);
extern void func_8004A2D0(void);
extern void func_80051810(void);
extern void func_8004F25C(void);
extern void func_8005965C(void);
extern int func_80189B24(void);
extern int func_8004F790(void);
extern void func_8004ED68(int side);
extern void func_800171C4(int phase);
extern int func_8004FBC4(int arg0);
extern void func_800171DC(void);
extern void func_80050510(int arg0);

void func_8007A328(void)
{
    int i;
    unsigned char phase;
    unsigned char resultMode;

    for (i = 0; i < 0x16; i++) {
        unsigned char *actor = D_800FF748 + (i + 1) * 0x3E8;

        if (func_800537D0(actor) != 0) {
            switch (B(Scratchpad, 0x29A)) {
            case 3:
                B(actor, 0x396) = 0x2C;
                B(actor, 0x397) = 0;
                break;
            case 4:
                B(actor, 0x396) = 0x2F;
                B(actor, 0x397) = 0;
                break;
            case 9:
                B(actor, 0x396) = 0x2D;
                B(actor, 0x397) = 0;
                break;
            case 12:
                if (B(actor, 0x396) != 0) {
                    B(actor, 0x396) = 0;
                    B(actor, 0x397) = 0;
                }
                break;
            }
        }
    }

    switch (B(Scratchpad, 0x29A)) {
    case 3:
        func_8009F62C(3);
        break;
    case 4:
        func_8009F62C(1);
        break;
    case 9:
        func_8009F62C(4);
        break;
    case 12:
        func_8009F62C(0x20);
        break;
    }

    func_8002283C();
    func_8004A2D0();
    func_80051810();
    func_8004F25C();
    func_8005965C();

    if (B(Scratchpad, 0x2E1) == 5 && func_80189B24() != 0) {
        return;
    }

    if (func_8004F790() != 0) {
        func_8004ED68(0);
        func_8004ED68(1);
        return;
    }

    if (H(Scratchpad, 0x294) != 0) {
        if (W(Scratchpad, 0x2C4) == 0 && SW(Scratchpad, 0x2C8) < 0xB4) {
            phase = B(Scratchpad, 0x29A);
            if (phase == 9) {
                func_800171C4(0xA);
                return;
            }
            if (phase == 3) {
                int bx = S(Scratchpad, 0x2BC);
                if (bx < 0) {
                    bx = -bx;
                }
                if (bx < 0x1990) {
                    func_800171C4(0xA);
                    return;
                }
            }
        }

        if (H(Scratchpad, 0x294) != 0) {
            H(Scratchpad, 0x294) = (unsigned short)(H(Scratchpad, 0x294) - 1);
            return;
        }
    }

    if (func_8004FBC4(0) == 0) {
        return;
    }

    B(Scratchpad, 0x2C2) = 0;
    B(D_800FF748, 0xAC76) = 0;
    func_800171DC();

    phase = B(Scratchpad, 0x29A);
    if (phase == 3) {
        unsigned int slot = B(Scratchpad, 0x2AD) % 24U;
        unsigned char *kicker = D_800FF748 + slot * 0x3E8;

        func_80050510(B(kicker, 0x39A) == 0 ? 1 : 0);
        return;
    }

    if (phase == 9) {
        unsigned int ev = W(Scratchpad, 0x2B0);
        if (ev == 4 || ev == 0xD || (ev - 9U) < 2U) {
            int bz = S(Scratchpad, 0x2BE);
            if (bz < 0) {
                bz = -bz;
            }
            if (bz < 0x8B7) {
                resultMode = 2;
                goto enter_phase_0xB;
            }
        }
        return;
    }

    if (phase == 0xC) {
        resultMode = 1;
        goto enter_phase_0xB;
    }

    func_80050510(1);
    if (B(Scratchpad, 0x2E1) == 5) {
        return;
    }

    {
        int bz = S(Scratchpad, 0x2BE);
        if (bz < 0) {
            bz = -bz;
        }
        if (bz < 0x12F5 && W(Scratchpad, 0x2B0) == 0xB) {
            unsigned char prevOwner = B(Scratchpad, 0x2AA);
            if (prevOwner == 1 || prevOwner == 0xC) {
                resultMode = 3;
                goto enter_phase_0xB;
            }
        }
    }
    return;

enter_phase_0xB:
    B(Scratchpad, 0x2A1) = 0;
    B(Scratchpad, 0x2C2) = 0;
    B(Scratchpad, 0x1EC) = 0;
    D_800EF9C8 = resultMode;
    func_800171C4(0xB);
}
