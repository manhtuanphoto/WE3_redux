/*
 * func_8005EB48
 *
 * Purpose:
 *   AI decision selector for a ball-carrying/acting player: picks an AI intent
 *   pair in D_8010C1D8+0x14 / +0x15 based on tactical state (D_8010C1D8+0x2D),
 *   player shooting/attack stats near the opponent penalty box, and checks
 *   whether to trigger an automatic tackle/intercept mask when the ball enters
 *   the opponent half wide of center.
 *
 * Arguments:
 *   actor (u8 *): pointer to 1000-byte actor struct
 *
 * Return value:
 *   void
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x01A[2] (u16) per-side action mask
 *   +0x298 (u8) game state (3 checked)
 *   +0x2F4 (u32) pointer to ball actor
 *   +0x328 (u8) default mode byte stored into D_800E6CB2
 *
 * Actor fields read:
 *   +0x02 (s16) x, +0x06 (s16) z
 *   +0x392 (u8) team slot (0..10), +0x398 (u8) side (0/1)
 *   +0x399 (u8) attack direction flag, +0x39A (u8) control/AI flag
 *
 * Actor fields written:
 *   +0x3C2 (s16) cleared to 0, +0x3D6 (u16) set to Scratchpad+0x1A[side]
 *
 * Global fields read/written:
 *   D_800E6CB2 (u8) written
 *   D_8010C1D8+0x14 (u8) primary AI command, +0x15 (u8) sub-command, +0x2D (u8) tactical state
 *   D_8010C328 / D_8010C32C (u32[]) player stat table (stride 12 per slot, 264 per side)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x8B6 (2230) = 8.920 m; 0x9B6 (2486) = 9.944 m;
 *   0x12F5 (4853) = 19.412 m; 0x1781 (6017) = 24.068 m;
 *   0x18E3 (6371) = 25.484 m; 0x22E3 (8931) = 35.724 m.
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800E6CB2;
extern unsigned char D_8010C1D8[];
extern unsigned char D_8010C328[];
extern unsigned char D_8010C32C[];

extern int func_8001E6B8(int n);
extern int func_80067A88(unsigned char *actor);
extern void func_8005E634(int dir);
extern int func_80067E7C(unsigned char *actor);

void func_8005EB48(unsigned char *actor)
{
    int r;
    int az;
    signed short ax;
    int dirX;
    unsigned int row;
    unsigned char atk;
    unsigned char *ball;

    if (B(actor, 0x39A) == 2 || B(Scratchpad, 0x298) == 3) {
        D_800E6CB2 = 2;
    } else {
        D_800E6CB2 = B(Scratchpad, 0x328);
    }

    switch (B(D_8010C1D8, 0x2D)) {
    case 1:
        switch (func_8001E6B8(6)) {
        case 0:
            B(D_8010C1D8, 0x14) = 1;
            B(D_8010C1D8, 0x15) = 2;
            break;
        case 1:
        case 2:
        case 3:
            B(D_8010C1D8, 0x14) = 1;
            B(D_8010C1D8, 0x15) = 6;
            break;
        case 4:
        case 5:
            B(D_8010C1D8, 0x14) = 1;
            B(D_8010C1D8, 0x15) = 0;
            break;
        }
        break;

    case 2:
        switch (func_8001E6B8(6)) {
        case 0:
        case 1:
        case 4:
            B(D_8010C1D8, 0x14) = 1;
            B(D_8010C1D8, 0x15) = 2;
            break;
        case 2:
        case 3:
            B(D_8010C1D8, 0x14) = 1;
            B(D_8010C1D8, 0x15) = 6;
            break;
        case 5:
            B(D_8010C1D8, 0x14) = 1;
            B(D_8010C1D8, 0x15) = 0;
            break;
        }
        break;

    case 3:
        r = func_8001E6B8(6);
        if (r >= 0) {
            if (r < 3) {
                B(D_8010C1D8, 0x14) = 1;
                B(D_8010C1D8, 0x15) = 6;
            } else if (r < 6) {
                B(D_8010C1D8, 0x14) = 1;
                B(D_8010C1D8, 0x15) = 2;
            }
        }
        break;

    case 4:
        r = func_8001E6B8(6);
        if (r >= 0) {
            if (r < 4) {
                B(D_8010C1D8, 0x14) = 1;
                B(D_8010C1D8, 0x15) = 6;
            } else if (r < 6) {
                B(D_8010C1D8, 0x14) = 1;
                B(D_8010C1D8, 0x15) = 0;
            }
        }
        break;

    case 5:
        r = func_8001E6B8(6);
        if (r >= 0) {
            if (r < 2) {
                B(D_8010C1D8, 0x14) = 1;
                B(D_8010C1D8, 0x15) = 6;
            } else if (r < 6) {
                B(D_8010C1D8, 0x14) = 1;
                B(D_8010C1D8, 0x15) = 0;
            }
        }
        break;

    case 6:
        r = func_8001E6B8(6);
        if (r >= 0) {
            if (r < 4) {
                B(D_8010C1D8, 0x14) = 1;
                B(D_8010C1D8, 0x15) = 6;
            } else if (r < 6) {
                B(D_8010C1D8, 0x14) = 1;
                B(D_8010C1D8, 0x15) = 2;
            }
        }
        if (func_80067A88(actor) < 4) {
            B(D_8010C1D8, 0x14) = 1;
            B(D_8010C1D8, 0x15) = 0;
        }
        break;
    }

    func_8005E634(B(actor, 0x399) != 0 ? 3 : 2);
    func_8005E634(S(actor, 6) >= 0 ? 1 : 0);

    az = S(actor, 6);
    if (az < 0) {
        az = -az;
    }
    if (az < 0x8B6) {
        ax = S(actor, 2);
        dirX = (B(actor, 0x399) != 0) ? -ax : ax;
        if (dirX >= 0x18E3) {
            row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
            if (func_8001E6B8(100 - (W(D_8010C328, row) & 0xF) * 5) == 0) {
                B(D_8010C1D8, 0x14) = 4;
                B(D_8010C1D8, 0x15) = 0;
                goto check_wide_ball;
            }

            row = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
            if (func_8001E6B8(100 - (W(D_8010C32C, row) & 0xF) * 5) == 0) {
                B(D_8010C1D8, 0x14) = 4;
                B(D_8010C1D8, 0x15) = 0;
                goto check_wide_ball;
            }
        }
    }

    if (func_8001E6B8(0xA) == 0) {
        az = S(actor, 6);
        if (az < 0) {
            az = -az;
        }
        if (az < 0x9B6) {
            ax = S(actor, 2);
            dirX = (B(actor, 0x399) != 0) ? -ax : ax;
            if (dirX >= 0x22E3) {
                B(D_8010C1D8, 0x14) = 4;
                B(D_8010C1D8, 0x15) = 0;
                goto check_wide_ball;
            }
        }
    }

check_wide_ball:
    atk = B(actor, 0x399);
    if (atk == 0) {
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        if (S(ball, 2) >= 0x1781) {
            goto trigger_wide;
        }
    }
    if (atk == 1) {
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        if (S(ball, 2) < -0x1780) {
            goto trigger_wide;
        }
    }
    return;

trigger_wide:
    az = S(actor, 6);
    if (az < 0) {
        az = -az;
    }
    if (az >= 0x12F5 && func_80067E7C(actor) != 0) {
        S(actor, 0x3C2) = 0;
        H(actor, 0x3D6) = H(Scratchpad, 0x1A + B(actor, 0x398) * 2);
    }
}
