/*
 * func_80056764
 *
 * Purpose:
 *   Triggers context-sensitive commentary / audio cues (via func_800AB620)
 *   during open play when the ball speed exceeds 0x1800 (24.0 raw/frame << 8)
 *   and Scratchpad+0x332 is in range 2..18. Evaluates ball/actor field
 *   positions, shot/clearance checks (func_80057AE4), actor animation id, and
 *   player stat bits in D_8010C328, then resets Scratchpad+0x332 to 0x20.
 *
 * Arguments:
 *   actor (u8 *) - pointer to 1000-byte actor struct
 *
 * Return value:
 *   void
 *
 * Actor fields read:
 *   +0x02 (s16) x, +0x06 (s16) z, +0x38E (u16) current anim id,
 *   +0x392 (u8) team slot, +0x398 (u8) side, +0x399 (u8) attack direction flag
 *
 * Global fields read:
 *   D_8010A392 (u8) commentary disable/suppress flag,
 *   D_8010C328 (u32[]) player stats table (bits 20..23 checked for >= 7)
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x36 (s16) ball x, +0x29A (u8) match phase (1 = open play),
 *   +0x2F4 (u32 -> u8 *) pointer to ball actor (reads ball+0x3A0 speed),
 *   +0x332 (u8) commentary trigger event state (2..18)
 *
 * Scratchpad fields written:
 *   +0x332 (u8) set to 0x20 after triggering commentary or func_80057AE4
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x1800 (6144 = 24<<8) - minimum ball speed threshold
 *   0x23E3 (9187 raw = 36.748 m) - deep attacking x threshold (case 2)
 *   0x12F4 (4852 raw = 19.408 m) - |z| threshold (case 2)
 *   0x110B (4363 raw = 17.452 m) - midfield/attacking x threshold (cases 5,6,8,12,18)
 *   0x13F5 (5109 raw = 20.436 m) - |z| threshold (cases 5,8,12)
 *   0x20E3 (8419 raw = 33.676 m) - attacking x threshold (cases 6,18)
 *   0x1FE3 (8163 raw = 32.652 m) - attacking x threshold (case 13)
 *   Commentary IDs: 0x10, 0xD67..0xD6A, 0xD72, 0xD74, 0xD75, 0xD77, 0xD78, 0xD7A, 0xDE0
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_8010A392;
extern unsigned int D_8010C328[];

extern int func_8001E6B8(int n);
extern int func_80057AE4(unsigned char *actor, int arg1, int arg2);
extern void func_800AB620(int id);

void func_80056764(unsigned char *actor)
{
    unsigned char *ball;
    signed short ball_x;
    signed int rel_ball_x;
    unsigned int clip_id;

    if (D_8010A392 != 0) {
        return;
    }
    if (B(Scratchpad, 0x29A) != 1) {
        return;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    if (W(ball, 0x3A0) < 0x1801U) {
        return;
    }

    ball_x = S(Scratchpad, 0x36);
    if (B(actor, 0x399) != 0) {
        rel_ball_x = -ball_x;
    } else {
        rel_ball_x = ball_x;
    }

    switch (B(Scratchpad, 0x332)) {
    case 2:
        if (func_8001E6B8(2) != 0) {
            return;
        }
        if ((signed short)rel_ball_x < 0x23E3) {
            return;
        }
        if (func_8001E6B8(2) == 0) {
            if (func_80057AE4(actor, 0, 0) != 0) {
                B(Scratchpad, 0x332) = 0x20;
                return;
            }
        }
        {
            int az = S(actor, 6);
            if (az < 0) {
                az = -az;
            }
            if (az < 0x12F4) {
                clip_id = 0xD78;
            } else {
                clip_id = 0xDE0;
            }
        }
        func_800AB620((int)clip_id);
        B(Scratchpad, 0x332) = 0x20;
        break;

    case 5:
    case 8:
    case 12:
    {
        int x = S(actor, 2);
        int fwd = (B(actor, 0x399) != 0) ? (-x >= 0x110B) : (x >= 0x110B);

        if (fwd) {
            unsigned int r = (unsigned int)func_8001E6B8(0x18);
            switch (r) {
            case 0:
            case 4:
            case 12:
                func_800AB620(0xD67);
                B(Scratchpad, 0x332) = 0x20;
                break;

            case 16:
            case 20:
            case 21:
                func_800AB620(0xD68);
                B(Scratchpad, 0x332) = 0x20;
                break;

            default:
                if (func_8001E6B8(3) == 0) {
                    if (func_80057AE4(actor, 0, 0) != 0) {
                        B(Scratchpad, 0x332) = 0x20;
                        return;
                    }
                }
                if (func_8001E6B8(3) != 0) {
                    return;
                }
                {
                    int az = S(actor, 6);
                    if (az < 0) {
                        az = -az;
                    }
                    if (az < 0x13F5) {
                        return;
                    }
                }
                if (H(actor, 0x38E) != 4) {
                    return;
                }
                if (func_8001E6B8(2) == 0) {
                    clip_id = 0xD69;
                } else {
                    clip_id = 0xD7A;
                }
                func_800AB620((int)clip_id);
                B(Scratchpad, 0x332) = 0x20;
                break;
            }
        } else {
            int r = func_8001E6B8(0x30);
            if (r == 0 || r == 0xC) {
                func_800AB620(0xD74);
                B(Scratchpad, 0x332) = 0x20;
            }
        }
        break;
    }

    case 6:
    case 18:
    {
        signed short rx = (signed short)rel_ball_x;
        if (rx >= 0x20E3) {
            func_800AB620(0x10);
            func_800AB620(0xD78);
            B(Scratchpad, 0x332) = 0x20;
        } else if (rx >= 0x110B) {
            if (func_8001E6B8(2) != 0) {
                func_800AB620(0x10);
                func_800AB620(0xD77);
                B(Scratchpad, 0x332) = 0x20;
            } else if (H(actor, 0x38E) == 4) {
                unsigned int off = B(actor, 0x392) * 12 + B(actor, 0x398) * 264;
                unsigned int stat = (W((unsigned char *)D_8010C328, off) >> 20) & 0xF;
                if ((int)stat >= 7 && func_8001E6B8(3) != 0) {
                    func_800AB620(0x10);
                    func_800AB620(0xD6A);
                    B(Scratchpad, 0x332) = 0x20;
                }
            }
        }
        break;
    }

    case 13:
        if ((signed short)rel_ball_x >= 0x1FE3) {
            func_800AB620(0xD72);
            B(Scratchpad, 0x332) = 0x20;
        } else if (func_8001E6B8(8) == 0) {
            func_800AB620(0xD75);
            B(Scratchpad, 0x332) = 0x20;
        }
        break;

    default:
        break;
    }
}
