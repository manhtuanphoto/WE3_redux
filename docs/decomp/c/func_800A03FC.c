/*
 * func_800A03FC
 *
 * Purpose:
 *   Updates outfield AI marking/pressing/support role flags (actor+0x3E0 and
 *   actor+0x3E1) for the 10 outfield players of the active/non-possessing team.
 *   Clears +0x3E1 when players stop matching designated presser/marker IDs,
 *   early-exits if the ball is beyond +-3008 raw X (+-12.032 m) depending on
 *   attack direction, and assigns +0x3E0 (0/1/2) based on match phase, mode,
 *   tactical index (Scratchpad+0x30B, cases 0..5), player role (+0x391), and
 *   pitch zone boundaries.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Actor / global table fields read:
 *   D_801002B0 (u8) active team index,
 *   D_800FF748 + 0xB68 (u8) side index (actor 2 + 0x398),
 *   D_8010C1D8 + 0x04 (u8) designated player actor id (checked in open-play case 3),
 *   actor+0x00 (u8) active flag, actor+0x02 (s16) x, actor+0x06 (s16) z,
 *   actor+0x38D (u8) actor id, actor+0x391 (u8) tactical role/position (1..13),
 *   actor+0x396 (u8) state, actor+0x399 (u8) attack direction flag,
 *   actor+0x39A (u8) presser flag
 *
 * Actor fields written:
 *   actor+0x3E0 (u8) defensive/pressing role flag (0, 1, or 2),
 *   actor+0x3E1 (u8) cleared to 0 when actor is no longer designated
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x006, +0x007 (u8) designated support/marker actor ids,
 *   +0x240, +0x241 (u8) designated actor ids,
 *   +0x29A (u8) match phase (1 = open play), +0x2A9 (u8) ball owner,
 *   +0x2CF[side] (u8) possession flags, +0x2E1 (u8) mode,
 *   +0x2F4 (u8*) pointer to ball actor, +0x30B (u8) defensive tactic state (0..5),
 *   +0x31E[side] (u8) target/presser actor id
 *
 * Scratchpad fields written:
 *   None
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x7D0 = 2000 (team 0 first outfielder, slot 2), 0x32C8 = 13000 (team 1 first outfielder, slot 13),
 *   0xBC0 = 3008 (12.032 m X boundary), 0x2101 = 8449 (33.796 m Z pitch width),
 *   0x12F5 = 4853 (19.412 m Z), 0x12F4 = 4852 (19.408 m Z),
 *   0x22E3 = 8931 (35.724 m X), 0x1EE3 = 7907 (31.628 m X), 0x1AE3 = 6883 (27.532 m X),
 *   0xFF4 = 4084 (16.336 m Z), 0x10F4 = 4340 (17.360 m Z), 0xAB6 = 2742 (10.968 m Z)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_801002B0;
extern unsigned char D_8010C1D8[];

static int abs16(int v)
{
    return (v < 0) ? -v : v;
}

void func_800A03FC(void)
{
    unsigned char *teamBase;
    unsigned char *actor;
    int i;
    unsigned char atkDir;
    unsigned char *ball;

    if (B(Scratchpad, 0x2CF + D_801002B0) != 0) {
        teamBase = D_800FF748 + 0x7D0;
    } else {
        teamBase = D_800FF748 + 0x32C8;
    }

    atkDir = B(teamBase, 0x399);
    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    if (atkDir == 0 && S(ball, 0x02) >= 0xBC1) {
        return;
    }
    if (atkDir == 1 && S(ball, 0x02) < -0xBC0) {
        return;
    }

    actor = teamBase;
    for (i = 0; i < 10; i++, actor += 0x3E8) {
        if (B(actor, 0x39A) != 0) {
            unsigned char targetId = B(Scratchpad, 0x31E + (B(actor, 0x399) << 1));
            if (B(actor, 0x38D) != targetId) {
                B(actor, 0x3E1) = 0;
            }
        } else {
            if (B(actor, 0x38D) != B(Scratchpad, 0x240)) {
                B(actor, 0x3E1) = 0;
            }
            if (B(actor, 0x38D) != B(Scratchpad, 0x241)) {
                B(actor, 0x3E1) = 0;
            }
        }
    }

    if (B(Scratchpad, 0x29A) != 1) {
        if (B(Scratchpad, 0x2E1) == 3) {
            if (B(Scratchpad, 0x2CF + B(D_800FF748, 0xB68)) != 1) {
                actor = D_800FF748 + 0x7D0;
            } else {
                actor = D_800FF748 + 0x32C8;
            }
            for (i = 0; i < 10; i++, actor += 0x3E8) {
                B(actor, 0x3E0) = 0;
            }
            return;
        }

        if (B(Scratchpad, 0x2CF + B(D_800FF748, 0xB68)) != 0) {
            actor = D_800FF748 + 0x7D0;
        } else {
            actor = D_800FF748 + 0x32C8;
        }
        for (i = 0; i < 10; i++, actor += 0x3E8) {
            if (B(actor, 0x396) != 0x35) {
                B(actor, 0x3E1) = 0;
            }
        }

        if (B(Scratchpad, 0x2CF + B(D_800FF748, 0xB68)) != 1) {
            actor = D_800FF748 + 0x7D0;
        } else {
            actor = D_800FF748 + 0x32C8;
        }
        for (i = 0; i < 10; i++, actor += 0x3E8) {
            B(actor, 0x3E0) = 0;
            if (B(actor, 0x00) != 0 && B(actor, 0x38D) != B(Scratchpad, 0x2A9)) {
                if (abs16(S(actor, 0x06)) < 0x2101) {
                    unsigned char role = B(actor, 0x391);
                    if (role >= 1 && role <= 8) {
                        B(actor, 0x3E0) = 1;
                    } else {
                        if (role == 9 || role == 10 || role == 13) {
                            B(actor, 0x3E0) = 2;
                        }
                        if (abs16(S(actor, 0x06)) < 0x12F5) {
                            B(actor, 0x3E0) = 0;
                        }
                    }
                }
            }
        }
        return;
    }

    if (B(Scratchpad, 0x2CF + B(D_800FF748, 0xB68)) != 1) {
        actor = D_800FF748 + 0x7D0;
    } else {
        actor = D_800FF748 + 0x32C8;
    }

    switch (B(Scratchpad, 0x30B)) {
    case 0:
        for (i = 0; i < 10; i++, actor += 0x3E8) {
            B(actor, 0x3E0) = 0;
        }
        break;

    case 1:
        for (i = 0; i < 10; i++, actor += 0x3E8) {
            B(actor, 0x3E0) = 0;
            if (B(actor, 0x00) != 0 && B(actor, 0x38D) != B(Scratchpad, 0x2A9)) {
                if (B(actor, 0x391) < 6) {
                    B(actor, 0x3E0) = 1;
                }
            }
        }
        break;

    case 2:
        for (i = 0; i < 10; i++, actor += 0x3E8) {
            B(actor, 0x3E0) = 0;
            if (B(actor, 0x00) != 0 && B(actor, 0x38D) != B(Scratchpad, 0x2A9)) {
                unsigned char id = B(actor, 0x38D);
                if (B(actor, 0x391) < 6
                    || (abs16(S(actor, 0x02)) >= 0x1AE3 && abs16(S(actor, 0x06)) < 0xAB6)) {
                    B(actor, 0x3E0) = 1;
                } else if (id == B(Scratchpad, 0x07) || id == B(Scratchpad, 0x06)) {
                    B(actor, 0x3E0) = 2;
                }
            }
        }
        break;

    case 3:
        for (i = 0; i < 10; i++, actor += 0x3E8) {
            B(actor, 0x3E0) = 0;
            if (B(actor, 0x00) != 0) {
                unsigned char id = B(actor, 0x38D);
                if (id != B(Scratchpad, 0x2A9)) {
                    if (B(actor, 0x391) < 6
                        || id == B(D_8010C1D8, 0x04)
                        || (abs16(S(actor, 0x02)) >= 0x1AE3 && abs16(S(actor, 0x06)) < 0xAB6)
                        || id == B(Scratchpad, 0x07)
                        || id == B(Scratchpad, 0x06)) {
                        B(actor, 0x3E0) = 1;
                    }
                }
            }
        }
        break;

    case 4:
        for (i = 0; i < 10; i++, actor += 0x3E8) {
            B(actor, 0x3E0) = 0;
            if (B(actor, 0x00) != 0 && B(actor, 0x38D) != B(Scratchpad, 0x2A9)) {
                if ((B(actor, 0x391) < 6 && abs16(S(actor, 0x06)) < 0x12F4)
                    || (abs16(S(actor, 0x02)) >= 0x22E3 && abs16(S(actor, 0x06)) < 0xFF4)) {
                    B(actor, 0x3E0) = 1;
                } else if (abs16(S(actor, 0x02)) >= 0x1EE3 && abs16(S(actor, 0x06)) < 0x10F4) {
                    B(actor, 0x3E0) = 2;
                }
            }
        }
        break;

    case 5:
        for (i = 0; i < 10; i++, actor += 0x3E8) {
            B(actor, 0x3E0) = 0;
            if (B(actor, 0x00) != 0 && B(actor, 0x38D) != B(Scratchpad, 0x2A9)) {
                if (B(actor, 0x391) < 6
                    || (abs16(S(actor, 0x02)) >= 0x22E3 && abs16(S(actor, 0x06)) < 0xFF4)) {
                    B(actor, 0x3E0) = 1;
                } else if (abs16(S(actor, 0x02)) >= 0x1EE3 && abs16(S(actor, 0x06)) < 0x10F4) {
                    B(actor, 0x3E0) = 2;
                }
            }
        }
        break;
    }
}
