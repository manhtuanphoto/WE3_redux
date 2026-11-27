/*
 * func_80088460
 *
 * Purpose:
 *   Replay playback and interactive replay camera/scrubbing controller.
 *   Restores recorded positions, headings, animations, and contact/mirror
 *   flags for all 22 active field actors and the ball from the 120-frame
 *   replay ring buffers at the current replay frame (Scratchpad+0x288),
 *   then updates playback speed, camera angle/zoom, focus actor, and exit
 *   conditions according to replay mode (D_800EF9C8) and controller input.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Actor fields read:
 *   +0x00 (u8) active flag (for actors 1..22), +0x398 (u8) side (actor id % 24),
 *   +0x3AA (u8) body heading, +0x3D2 (u8) flag clearing +0x3AD
 *   Ball (+0x02 s16 x)
 *
 * Actor fields written (actors 1..22):
 *   +0x02 (s16) x, +0x04 (s16) y, +0x06 (s16) z, +0x26 (u16) rotation,
 *   +0x38E (u16) current anim id, +0x390 (u8) anim frame,
 *   +0x3AA (u8) body heading, +0x3AC (u8) mirror/foot, +0x3AD (u8) cleared if +0x3D2 != 0
 *   Ball: +0x02/+0x04/+0x06 (s16/u16) xyz, +0x24/+0x28 (u32) 8-byte rotation/state
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x230[pad*8] (u16) held buttons, +0x234[pad*8] (u16) newly pressed buttons,
 *   +0x236[pad*8] (u16) button edge/repeat state, +0x288 (s16) replay frame index,
 *   +0x290 (u32) global frame counter, +0x2AD (u8) target actor id (mod 24),
 *   +0x2AE (u8) replay camera focus actor (0..22), +0x2E2 (u8) replay/camera submode,
 *   +0x2E7 (u8) camera yaw/angle, +0x2EA (s16) camera zoom/distance,
 *   +0x2F2 (u8) camera mode (0..2 or 0xFF), +0x2F4 (ptr) ball actor
 *
 * Scratchpad fields written:
 *   +0x288 (s16) replay frame index (0..119), +0x2AE (u8) focus actor (0..22),
 *   +0x2E7 (u8) camera yaw/angle, +0x2EA (s16) camera zoom/distance,
 *   +0x2F2 (u8) camera mode (0..2)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   120 (0x78) frames = 4.0 s replay buffer length (indices 0..119 = 0x77)
 *   240 (0xF0) frames = 8.0 s OSD/UI auto-hide timer (D_800FB5A8)
 *   0x58 (88) frames = 2.93 s, 0x39 (57) frames = 1.90 s,
 *   0x68 (104) frames = 3.47 s, 0x48 (72) frames = 2.40 s
 *   0x3421 (13345) = 53.38 m, 0x3320 (13088) = 52.352 m (goal line),
 *   0x101 (257) = 1.028 m threshold around goal line
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800E6D68;
extern unsigned char D_800E6D69;
extern unsigned char D_800E7DF8[];
extern unsigned char D_800EF9C8;
extern unsigned char D_800EF9D0[];
extern unsigned char D_800F0450[];
extern unsigned short D_800F0EE0;
extern unsigned char D_800F20B8[];
extern unsigned char D_800F2B10[];
extern unsigned char D_800FB5A8;
extern unsigned char D_800FB5D0[];
extern unsigned char D_800FB6C0[];
extern unsigned char D_800FCB60[];
extern unsigned char D_800FCC50[];
extern unsigned char D_800FE0F0[];
extern unsigned char D_800FE1E0[];
extern unsigned char D_800FF748[];
extern unsigned char D_8010556D;
extern unsigned char D_8010A323;
extern unsigned short D_8010C2A0;

extern void func_800171DC(void);
extern unsigned char *func_8001D004(int id);
extern void func_800477A0(unsigned char *actor, int arg1);
extern void func_8004FB68(int arg0);
extern int func_80051810(void);
extern int func_800525A4(void);
extern void func_8005BA30(unsigned char *actor, unsigned int anim, unsigned int frame);
extern void func_800882D8(unsigned int buttonMask);
extern void func_80089C08(unsigned char *actor);
extern void func_800989C8(unsigned char *actor, int arg1);
extern void func_800AB620(int sfxId);

void func_80088460(void)
{
    int padIdx;
    int i;
    int frame;
    unsigned char *actor;
    unsigned char *ball;

    D_8010556D = ((SW(Scratchpad, 0x290) >> 3) & 1) ? 0x5B : 0x3B;

    padIdx = 0;
    if (B(Scratchpad, 0x2E2) == 1) {
        int actorIdx = B(Scratchpad, 0x2AD) % 24;
        unsigned char *targetActor = D_800FF748 + actorIdx * 1000;
        padIdx = B(targetActor, 0x398) ^ D_8010A323;
    }

    func_8004FB68(0);
    func_800989C8((unsigned char *)W(Scratchpad, 0x2F4), 1);

    /* Restore all 22 field actors (indices 1..22) for the current replay frame */
    actor = D_800FF748 + 0x3E8;
    for (i = 0; i < 22; i++, actor += 1000) {
        if (B(actor, 0x00) != 0) {
            int slotIdx;
            int rot;

            frame = S(Scratchpad, 0x288);
            slotIdx = frame * 22 + i;
            H(actor, 0x02) = H(D_800FB6C0, slotIdx * 2);
            H(actor, 0x04) = H(D_800FCC50, slotIdx * 2);
            H(actor, 0x06) = H(D_800FE1E0, slotIdx * 2);

            frame = S(Scratchpad, 0x288);
            slotIdx = frame * 22 + i;
            B(actor, 0x3AA) = B(D_800F20B8, slotIdx);

            frame = S(Scratchpad, 0x288);
            slotIdx = frame * 22 + i;
            H(actor, 0x38E) = B(D_800F2B10, slotIdx);

            frame = S(Scratchpad, 0x288);
            slotIdx = frame * 22 + i;
            B(actor, 0x390) = B(D_800F0450, slotIdx);

            frame = S(Scratchpad, 0x288);
            slotIdx = frame * 22 + i;
            B(actor, 0x3AC) = B(D_800EF9D0, slotIdx);

            func_8005BA30(actor, B(actor, 0x38E), B(actor, 0x390));

            rot = 0x1C0 - (int)B(actor, 0x3AA);
            H(actor, 0x26) = (unsigned short)((rot % 256) << 4);
            func_80089C08(actor);

            if (B(actor, 0x3D2) != 0) {
                B(actor, 0x3AD) = 0;
            }
        }
    }

    /* Restore ball position and 8-byte orientation/state */
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    H(ball, 0x02) = H(D_800FB5D0, S(Scratchpad, 0x288) * 2);
    H(ball, 0x04) = H(D_800FCB60, S(Scratchpad, 0x288) * 2);
    H(ball, 0x06) = H(D_800FE0F0, S(Scratchpad, 0x288) * 2);

    frame = S(Scratchpad, 0x288);
    /* Original asm uses lwl/lwr + swl/swr for 8-byte copy from D_800E7DF8 */
    W(ball, 0x24) = W(D_800E7DF8, frame * 8);
    W(ball, 0x28) = W(D_800E7DF8, frame * 8 + 4);

    func_8005BA30(ball, 0, 0);
    func_800477A0(ball, 2);
    func_80051810();

    switch (D_800EF9C8) {
    case 0: {
        int padOff = padIdx * 8;
        unsigned char *uiObj;
        int j;
        int curFrame;
        int camMode;

        if ((H(Scratchpad, 0x234 + padOff) & 0x2000) == 0x2000) {
            D_800E6D69 = 0;
        }
        curFrame = S(Scratchpad, 0x288);
        if ((H(Scratchpad, 0x236 + padOff) & 0x2000) == 0x2000 ||
            curFrame == (int)D_800F0EE0 ||
            curFrame == (int)D_8010C2A0) {
            uiObj = func_8001D004(0x3E);
            for (j = 0; j < 6; j++) {
                B(uiObj, j * 12 + 0xA) = 0x16;
            }
        }

        if ((H(Scratchpad, 0x234 + padOff) & 0x8000) == 0x8000) {
            D_800E6D69 = 0;
        }
        curFrame = S(Scratchpad, 0x288);
        if ((H(Scratchpad, 0x236 + padOff) & 0x8000) == 0x8000 ||
            curFrame == (int)D_800F0EE0 ||
            curFrame == (int)D_8010C2A0) {
            uiObj = func_8001D004(0x3E);
            for (j = 0; j < 6; j++) {
                B(uiObj, j * 12 + 0xA) = 0x16;
            }
        }

        /* 15 Hz forward/reverse playback when button held or auto-play enabled */
        if ((W(Scratchpad, 0x290) & 1) != 0) {
            if (S(Scratchpad, 0x288) != (int)D_8010C2A0) {
                if ((H(Scratchpad, 0x230 + padOff) & 0x2000) != 0 || D_800E6D68 != 0) {
                    func_800882D8(0x2000);
                    S(Scratchpad, 0x288) = (S(Scratchpad, 0x288) + 1) % 120;
                    D_800FB5A8 = 0xF0;
                    if (S(Scratchpad, 0x288) == (int)D_8010C2A0 &&
                        D_800E6D68 != 0 &&
                        B(Scratchpad, 0x2E2) == 4) {
                        func_800171DC();
                        return;
                    }
                }
            }
            if (S(Scratchpad, 0x288) != (int)D_800F0EE0 &&
                (H(Scratchpad, 0x230 + padOff) & 0x8000) != 0) {
                D_800E6D68 = 0;
                func_800882D8(0x8000);
                if (H(Scratchpad, 0x288) != 0) {
                    H(Scratchpad, 0x288) = H(Scratchpad, 0x288) - 1;
                } else {
                    H(Scratchpad, 0x288) = 119;
                }
                D_800FB5A8 = 0xF0;
            }
        }

        /* Cycle camera mode (0..2) */
        if ((H(Scratchpad, 0x234 + padOff) & 0x100) != 0) {
            int nextCam = (B(Scratchpad, 0x2F2) + 1) % 3;
            B(Scratchpad, 0x2F2) = (unsigned char)nextCam;
            if (nextCam != 0 && nextCam < 3) {
                if (S(Scratchpad, 0x2EA) < 0x1C00) {
                    S(Scratchpad, 0x2EA) = 0x1C00;
                }
            }
        }

        /* Single-frame step forward */
        if ((H(Scratchpad, 0x234 + padOff) & 0x20) == 0x20) {
            D_800E6D69 = 0;
        }
        curFrame = S(Scratchpad, 0x288);
        if ((H(Scratchpad, 0x236 + padOff) & 0x20) == 0x20 ||
            curFrame == (int)D_800F0EE0 ||
            curFrame == (int)D_8010C2A0) {
            uiObj = func_8001D004(0x3E);
            for (j = 0; j < 6; j++) {
                B(uiObj, j * 12 + 0xA) = 0x16;
            }
        }
        if (S(Scratchpad, 0x288) != (int)D_8010C2A0 &&
            (H(Scratchpad, 0x234 + padOff) & 0x20) != 0) {
            func_800882D8(0x20);
            D_800E6D68 = 0;
            D_800FB5A8 = 0xF0;
            S(Scratchpad, 0x288) = (S(Scratchpad, 0x288) + 1) % 120;
        }

        /* Single-frame step backward */
        if ((H(Scratchpad, 0x234 + padOff) & 0x80) == 0x80) {
            D_800E6D69 = 0;
        }
        curFrame = S(Scratchpad, 0x288);
        if ((H(Scratchpad, 0x236 + padOff) & 0x80) == 0x80 ||
            curFrame == (int)D_800F0EE0 ||
            curFrame == (int)D_8010C2A0) {
            uiObj = func_8001D004(0x3E);
            for (j = 0; j < 6; j++) {
                B(uiObj, j * 12 + 0xA) = 0x16;
            }
        }
        if (S(Scratchpad, 0x288) != (int)D_800F0EE0 &&
            (H(Scratchpad, 0x234 + padOff) & 0x80) != 0) {
            func_800882D8(0x80);
            D_800E6D68 = 0;
            if (H(Scratchpad, 0x288) != 0) {
                H(Scratchpad, 0x288) = H(Scratchpad, 0x288) - 1;
            } else {
                H(Scratchpad, 0x288) = 119;
            }
            D_800FB5A8 = 0xF0;
        }

        /* Rotate camera angle */
        if ((H(Scratchpad, 0x230 + padOff) & 0x8) != 0) {
            D_800FB5A8 = 0xF0;
            B(Scratchpad, 0x2E7) += 2;
        }
        if ((H(Scratchpad, 0x230 + padOff) & 0x4) != 0) {
            D_800FB5A8 = 0xF0;
            B(Scratchpad, 0x2E7) -= 2;
        }

        /* Adjust camera zoom depending on camera mode */
        camMode = B(Scratchpad, 0x2F2);
        if (camMode == 0) {
            if ((H(Scratchpad, 0x230 + padOff) & 0x1000) != 0 && S(Scratchpad, 0x2EA) >= 0x1201) {
                S(Scratchpad, 0x2EA) -= 0x100;
                D_800FB5A8 = 0xF0;
            }
            if ((H(Scratchpad, 0x230 + padOff) & 0x4000) != 0 && S(Scratchpad, 0x2EA) < 0x3200) {
                S(Scratchpad, 0x2EA) += 0x100;
                D_800FB5A8 = 0xF0;
            }
        } else if (camMode < 3) {
            if ((H(Scratchpad, 0x230 + padOff) & 0x1000) != 0 && S(Scratchpad, 0x2EA) >= 0x1C01) {
                S(Scratchpad, 0x2EA) -= 0x100;
                D_800FB5A8 = 0xF0;
            }
            if ((H(Scratchpad, 0x230 + padOff) & 0x4000) != 0 && S(Scratchpad, 0x2EA) < 0x3400) {
                S(Scratchpad, 0x2EA) += 0x100;
                D_800FB5A8 = 0xF0;
            }
        }

        /* Auto-play toggle, jump-to-start/end, or cycle focus actor (0..22) */
        if ((H(Scratchpad, 0x230 + padOff) & 0x10) != 0) {
            D_800E6D68 = 1;
            D_800FB5A8 = 0xF0;
        }
        if ((H(Scratchpad, 0x230 + padOff) & 0x40) != 0) {
            unsigned short pressed = H(Scratchpad, 0x234 + padOff);
            D_800E6D68 = 0;
            D_800FB5A8 = 0xF0;
            if ((pressed & 0x1) != 0) {
                H(Scratchpad, 0x288) = D_800F0EE0;
                return;
            }
            if ((pressed & 0x2) != 0) {
                H(Scratchpad, 0x288) = D_8010C2A0;
                return;
            }
        } else if ((H(Scratchpad, 0x234 + padOff) & 0x2) != 0) {
            D_800FB5A8 = 0xF0;
            B(Scratchpad, 0x2AE) = (B(Scratchpad, 0x2AE) + 1) % 23;
        }

        if ((H(Scratchpad, 0x234 + padOff) & 0x1) != 0) {
            D_800FB5A8 = 0xF0;
            B(Scratchpad, 0x2AE) = (B(Scratchpad, 0x2AE) + 22) % 23;
        }

        D_800FB5A8--;
        if ((H(Scratchpad, 0x234 + padOff) & 0x800) != 0) {
            func_800AB620(0x528);
            func_800171DC();
            return;
        }
        break;
    }

    case 1: {
        int mark;
        int diff;
        if (func_800525A4() != 0 || S(Scratchpad, 0x288) == ((int)D_800F0EE0 + 0x58) % 120) {
            func_800171DC();
        }
        mark = ((int)D_800F0EE0 + 0x39) % 120;
        diff = S(Scratchpad, 0x288) - mark;
        if (diff < 0) {
            diff = -diff;
        }
        if (diff < 4 && (W(Scratchpad, 0x290) & 1) != 0) {
            return;
        }
        S(Scratchpad, 0x288) = (S(Scratchpad, 0x288) + 1) % 120;
        break;
    }

    case 2:
    case 3: {
        int ballX;
        int absX;
        int distToGoalLine;

        if (func_800525A4() != 0 || S(Scratchpad, 0x288) == ((int)D_800F0EE0 + 0x48) % 120) {
            func_800171DC();
        }
        ballX = S((unsigned char *)W(Scratchpad, 0x2F4), 0x02);
        absX = (ballX >= 0) ? ballX : -ballX;

        if (absX < 0x3421 && B(Scratchpad, 0x2F2) != 0xFF) {
            distToGoalLine = (ballX >= 0) ? (ballX - 0x3320) : (ballX + 0x3320);
            if (distToGoalLine < 0) {
                distToGoalLine = -distToGoalLine;
            }
            if ((signed short)distToGoalLine < 0x101) {
                if ((W(Scratchpad, 0x290) & 1) != 0) {
                    return;
                }
            }
        } else {
            if ((W(Scratchpad, 0x290) & 1) != 0) {
                return;
            }
        }
        S(Scratchpad, 0x288) = (S(Scratchpad, 0x288) + 1) % 120;
        break;
    }

    case 4: {
        int mark;
        int diff;
        if (func_800525A4() != 0 || S(Scratchpad, 0x288) == (int)D_8010C2A0) {
            func_800171DC();
        }
        mark = ((int)D_800F0EE0 + 0x68) % 120;
        diff = S(Scratchpad, 0x288) - mark;
        if (diff < 0) {
            diff = -diff;
        }
        if (diff < 16 && (W(Scratchpad, 0x290) & 1) != 0) {
            return;
        }
        S(Scratchpad, 0x288) = (S(Scratchpad, 0x288) + 1) % 120;
        break;
    }

    default:
        if (D_800FB5A8 == 0) {
            func_800171DC();
        }
        break;
    }
}
