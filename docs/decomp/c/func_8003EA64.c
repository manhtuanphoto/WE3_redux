/*
 * func_8003EA64 - Resolve player-vs-player physical contact / tackle duel / ball challenge
 *
 * Purpose:
 *   Checks physical contact between an actor and its target actor (actor+0x3C2 % 24)
 *   during open play (scratch 0x29A == 1, 0x2FC == 0) when contact timer +0x3C4 != 0.
 *   Mode 1 handles body/shoulder duel using +0x3BA stat comparison and stamina/timer
 *   decay (+0x3C6) plus limb contact tests. Mode 2 handles challenge/interception
 *   turning toward the ball and checking proximity thresholds before calling
 *   func_8003EF94 / func_8003F0D0.
 *
 * Arguments:
 *   actor - pointer to 1000-byte player actor struct
 *   mode  - u8 contact mode (1 = duel/push, 2 = challenge/tackle)
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Actor fields read & written:
 *   Scratch 0x36/66/4E (s16) - contact joint/foot x, z, y
 *   Scratch 0xF0/F4    (s16) - computed limb contact x, z (from func_8005C2AC)
 *   Scratch 0x29A      (u8)  - match phase (1 = open play)
 *   Scratch 0x2A9      (u8)  - ball owner (0 = free)
 *   Scratch 0x2AA      (u8)  - previous ball owner
 *   Scratch 0x2F4      (ptr) - ball actor pointer
 *   Scratch 0x2FC      (u8)  - pause/lock flag (must be 0)
 *   Actor +0x02/06     (s16) - x, z position
 *   Actor +0x38D       (u8)  - actor id
 *   Actor +0x38E       (u16/u8) - current anim id
 *   Actor +0x390       (u8)  - anim frame
 *   Actor +0x394       (u32) - packed flags/roster word (checked & 0xFFFF0000 == 0x04000000)
 *   Actor +0x396/397   (u8)  - state / substate (set to 0x10 / 0 on duel loss)
 *   Actor +0x3A0       (u32) - speed (scaled by 9/10 or 8/10 on target)
 *   Actor +0x3AA/3AB   (u8)  - body heading / target heading
 *   Actor +0x3AC       (u8)  - mirror/foot flag
 *   Actor +0x3B8       (u8)  - stat*10 (cleared on target in mode 2 condition)
 *   Actor +0x3BA       (u8)  - balance/body stat compared in mode 1
 *   Actor +0x3C2       (s16) - target actor index (% 24)
 *   Actor +0x3C4       (s16) - active contact check flag/timer (cleared on func_8003F0D0)
 *   Actor +0x3C6       (s16) - duel timer/counter (clamped >= 0x26 = 38)
 *   Actor +0x3C8       (s16) - contact submode / duel threshold
 *   Actor +0x3CA       (u16) - turn speed (set to 4 in mode 2)
 *   Actor +0x3D0       (u8)  - checked != 0 in mode 2
 *
 * Magic numbers:
 *   0x2400 (9216  = 96^2)  -> 0.384 m radius (mode 1 limb-to-target check)
 *   0x7900 (30976 = 176^2) -> 0.704 m radius (mode 1 actor-to-joint check)
 *   0x1000 (4096  = 64^2)  -> 0.256 m radius (mode 2 limb-to-target check)
 *   0x400  (1024  = 32^2)  -> 0.128 m radius (target anim 0x15/0x17/0x58)
 *   0x900  (2304  = 48^2)  -> 0.192 m radius (default mode 2 limb-to-joint)
 *   0x5100 (20736 = 144^2) -> 0.576 m radius (target anim 0x55, frame >= 8)
 *   0x3100 (12544 = 112^2) -> 0.448 m radius (mode 2 actor-to-joint check)
 *   -0x5F  (-95)           -> -0.38 m height cutoff in mode 2
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char D_800FF748[];

extern int func_8001E6B8(int n);
extern void func_8005C2AC(unsigned char *actor, unsigned int anim, unsigned int frame, unsigned int heading, unsigned int mirror);
extern int func_8003EF94(unsigned char *actor, unsigned char *target);
extern void func_8003F0D0(unsigned char *actor, unsigned char *target);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, unsigned int step);

void func_8003EA64(unsigned char *actor, unsigned char mode)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    int target_idx = S(actor, 0x3C2) % 24;
    unsigned char *target = D_800FF748 + target_idx * 1000;
    unsigned int mode_byte;

    if (S(actor, 0x3C4) == 0) {
        return;
    }
    if (B(scratch, 0x29A) != 1 || B(scratch, 0x2FC) != 0) {
        return;
    }

    mode_byte = mode & 0xFF;
    if (mode_byte == 1) {
        int c = S(actor, 0x3C8);

        if (c != 0) {
            int diff = (int)B(target, 0x3BA) - (int)B(actor, 0x3BA);

            if (diff == 0) {
                if (B(actor, 0x390) == 3) {
                    W(target, 0x3A0) = (W(target, 0x3A0) * 9) / 10;
                }
                if (func_8001E6B8(0x20) == 0) {
                    B(actor, 0x396) = 0x10;
                    B(actor, 0x397) = 0;
                    return;
                }
            } else if (diff > 0) {
                if (diff == 1) {
                    if (func_8001E6B8(0x18) == 0) {
                        B(actor, 0x396) = 0x10;
                        B(actor, 0x397) = 0;
                        return;
                    }
                    H(actor, 0x3C6) = (unsigned short)(H(actor, 0x3C6) - 1);
                } else {
                    if (func_8001E6B8(0xC) == 0) {
                        B(actor, 0x396) = 0x10;
                        B(actor, 0x397) = 0;
                        return;
                    }
                    H(actor, 0x3C6) = (unsigned short)(H(actor, 0x3C6) - 2);
                }
            } else {
                if (diff >= -9) {
                    if (B(actor, 0x390) == 3) {
                        W(target, 0x3A0) = (W(target, 0x3A0) << 3) / 10;
                    }
                } else {
                    if (func_8001E6B8(0xC) == 0) {
                        B(actor, 0x396) = 0x10;
                        B(actor, 0x397) = 0;
                        return;
                    }
                    H(actor, 0x3C6) = (unsigned short)(H(actor, 0x3C6) - 2);
                }
            }

            if (S(actor, 0x3C6) < 0x27) {
                H(actor, 0x3C6) = 0x26;
            }
        } else {
            int dx, dz;
            func_8005C2AC(actor, B(actor, 0x38E), B(actor, 0x390), B(actor, 0x3AA), B(actor, 0x3AC));
            dx = S(target, 2) - S(scratch, 0xF0);
            dz = S(target, 6) - S(scratch, 0xF4);
            if (dx * dx + dz * dz < 0x2400) {
                if (func_8003EF94(actor, target) != 0) {
                    return;
                }
            }
        }

        {
            int dx = S(actor, 2) - S(scratch, 0x36);
            int dz = S(actor, 6) - S(scratch, 0x66);
            if ((unsigned int)(dx * dx + dz * dz) < 0x7900) {
                func_8003F0D0(actor, target);
                H(actor, 0x3C4) = 0;
            }
        }
    } else if (mode_byte == 2) {
        unsigned char heading;
        unsigned int thresh;
        int dx, dz, limb_dx, limb_dz;

        if (B(actor, 0x390) >= 8 && B(actor, 0x38D) != B(scratch, 0x2AA)) {
            unsigned char *ball = (unsigned char *)W(scratch, 0x2F4);
            B(actor, 0x3AB) = (unsigned char)func_8005B36C(S(actor, 2), S(actor, 6), S(ball, 2), S(ball, 6));
            H(actor, 0x3CA) = 4;
        }

        heading = (unsigned char)func_8005B7CC(B(actor, 0x3AB), B(actor, 0x3AA), B(actor, 0x3CA));
        B(actor, 0x3AA) = heading;

        if (B(scratch, 0x2A9) == 0 || S(scratch, 0x4E) < -0x5F) {
            return;
        }

        func_8005C2AC(actor, B(actor, 0x38E), B(actor, 0x390), heading, B(actor, 0x3AC));

        if (S(actor, 0x3C8) == 0) {
            dx = S(target, 2) - S(scratch, 0xF0);
            dz = S(target, 6) - S(scratch, 0xF4);
            if (dx * dx + dz * dz < 0x1000) {
                if (func_8003EF94(actor, target) != 0) {
                    return;
                }
            }
        }

        {
            unsigned short t_anim = H(target, 0x38E);
            if (t_anim == 0x15 || t_anim == 0x17 || t_anim == 0x58) {
                thresh = 0x400;
            } else if (t_anim == 0x55) {
                thresh = (B(target, 0x390) < 8) ? 0x900 : 0x5100;
            } else {
                thresh = 0x900;
            }
        }

        limb_dx = S(scratch, 0xF0) - S(scratch, 0x36);
        limb_dz = S(scratch, 0xF4) - S(scratch, 0x66);
        if ((unsigned int)(limb_dx * limb_dx + limb_dz * limb_dz) < thresh) {
            goto do_contact;
        }

        dx = S(actor, 2) - S(scratch, 0x36);
        dz = S(actor, 6) - S(scratch, 0x66);
        if (dx * dx + dz * dz < 0x3100) {
do_contact:
            H(actor, 0x3C4) = 0;
            if (B(target, 0x3B8) != 0 &&
                (W(target, 0x394) & 0xFFFF0000) == 0x04000000 &&
                B(target, 0x3D0) != 0) {
                B(target, 0x3B8) = 0;
                return;
            }
            func_8003F0D0(actor, target);
        }
    }
}
