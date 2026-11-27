/*
 * func_8009DCC0
 *
 * Purpose:
 *   Defending/off-ball AI controller for a player without team possession:
 *   sets off-ball defensive role flags (+0x3E2/+0x3E3) when outside the own
 *   penalty area or when the ball is wide/deep, dispatches the primary ball
 *   chaser (or Scratchpad+0x241) to func_8005E510 / func_8005E104, checks
 *   secondary marker (+0x31F) engagement against ball/opponent position, and
 *   staggers tactical position updates via (actor_id % 22 == frame % 22).
 *
 * Arguments:
 *   actor (unsigned char*) - pointer to 1000-byte player actor struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Actor fields:
 *     +0x002 (s16) x, +0x006 (s16) z (R)
 *     +0x38D (u8)  actor id (R)
 *     +0x396 (u8)  state (R)
 *     +0x398 (u8)  side (0/1) (R)
 *     +0x399 (u8)  attack direction flag (R)
 *     +0x39C (u8)  tactical update flag (W)
 *     +0x3BC (s16), +0x3BE (s16) target/anchor x, z (W)
 *     +0x3D4 (u16), +0x3D6 (u16), +0x3D8 (u16) tactical/formation fields (R/W)
 *     +0x3E1 (u8)  cleared to 0 before pressing/chasing (W)
 *     +0x3E2 (u8), +0x3E3 (u8) defensive role/mode bytes (W)
 *   Scratchpad (0x1F800000):
 *     +0x03E (s16) line/reference X (R)
 *     +0x04E (s16) predicted ball height Y (R)
 *     +0x0FE (s16), +0x100 (s16) predicted ball X, Z (R)
 *     +0x241 (u8)  designated chaser actor id (R)
 *     +0x28C (u8)  set to actor+0x3D4 >> 12 (W)
 *     +0x290 (u32) frame counter (R)
 *     +0x2A9 (u8)  ball owner (0 = free) (R)
 *     +0x2B0 (u32) ball event code (R)
 *     +0x2CF (u8[2]) per-side possession flags (R)
 *     +0x2F4 (u8*) pointer to ball actor (+0x02 x, +0x06 z, +0x0C vel y) (R)
 *     +0x31E (u8[2*2]) primary chaser (+0) and secondary marker (+1) per atkDir (R)
 *     +0x3D0 (u8)  bitmask per atkDir forcing secondary marker press (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x22E3 = 8931 raw (35.724 m) penalty-box X threshold
 *   0x0FF4 = 4084 raw (16.336 m) penalty-box Z half-width
 *   0x23E2 = 9186 raw (36.744 m) deep ball X threshold
 *   -0x0FE2 / -0x17E2 = -4066 / -6114 raw (-16.264 m / -24.456 m) defensive line X
 *   0x12F4 = 4852 raw (19.408 m), 0x14F4 = 5364 raw (21.456 m) Z corridor thresholds
 *   0x3FFFF = 262143 -> sqrt(262144) = 512 raw (2.048 m) close engagement radius
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];

extern void func_8009E244(unsigned char*);
extern void func_8005E510(unsigned char*);
extern void func_8005E104(unsigned char*);
extern int func_8005346C(unsigned char*);
extern void func_8009F1BC(unsigned char*);

void func_8009DCC0(unsigned char* actor) {
    unsigned char atkDir;
    unsigned char actorId;
    unsigned char primaryId;

    if (B(Scratchpad, 0x2CF + B(actor, 0x398)) == 0) {
        actorId = B(actor, 0x38D);
        if (actorId == B(Scratchpad, 0x2A9)) {
            B(actor, 0x3E2) = 0xD;
            B(actor, 0x3E3) = 0;
            func_8009E244(actor);
            return;
        }
        if (actorId == B(Scratchpad, 0x31E + B(actor, 0x399) * 2)) {
            B(actor, 0x3E2) = 0xB;
            B(actor, 0x3E3) = 0;
        }

        {
            signed short x = S(actor, 2);
            int absX = (x >= 0) ? x : -x;
            if (absX >= 0x22E3) {
                signed short z = S(actor, 6);
                int absZ = (z >= 0) ? z : -z;
                if (absZ < 0xFF4) {
                    if (B(actor, 0x399) != 0) {
                        if (x <= 0) {
                            goto set_role_b;
                        }
                    } else if (x >= 0) {
                        goto set_role_b;
                    }

                    atkDir = B(actor, 0x399);
                    if (atkDir == 0) {
                        unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
                        if (S(ball, 2) < -0x23E2) {
                            goto after_role;
                        }
                    }
                    if (atkDir == 1) {
                        unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
                        if (S(ball, 2) >= 0x23E3) {
                            goto after_role;
                        }
                    }
                }
            }
        set_role_b:
            B(actor, 0x3E2) = 0xB;
            B(actor, 0x3E3) = 0;
        }
    }

after_role:
    atkDir = B(actor, 0x399);
    actorId = B(actor, 0x38D);
    primaryId = B(Scratchpad, 0x31E + atkDir * 2);

    if (actorId == primaryId || actorId == B(Scratchpad, 0x241)) {
        if (B(Scratchpad, 0x2A9) == 0 && S(Scratchpad, 0x04E) < -0xFF) {
            int velY = SW(W(Scratchpad, 0x2F4), 0xC);
            if (velY < 0) {
                velY = -velY;
            }
            if (velY >= 0x1700) {
                func_8005E104(actor);
                goto store_anchor_ret;
            }
        }
        func_8005E510(actor);
    store_anchor_ret:
        B(actor, 0x3E1) = 0;
        H(actor, 0x3BC) = H(actor, 2);
        H(actor, 0x3BE) = H(actor, 6);
        return;
    }

    if (actorId == B(Scratchpad, 0x31F + atkDir * 2)) {
        unsigned char* primaryActor = D_800FF748 + ((unsigned int)primaryId % 24u) * 1000;
        signed short refX = S(Scratchpad, 0x03E);
        int primX = S(primaryActor, 2);
        int threshX;
        signed short lineX;
        unsigned char oppId;
        unsigned char* oppActor;

        if (atkDir != 0) {
            primX = -primX;
        }
        threshX = primX - 0x200;
        if (B(actor, 0x399) != 0) {
            lineX = (-refX < threshX) ? -0xFE2 : -0x17E2;
        } else {
            lineX = (refX < threshX) ? -0xFE2 : -0x17E2;
        }

        atkDir = B(actor, 0x399);
        oppId = B(Scratchpad, 0x31E + (atkDir ^ 1) * 2);
        oppActor = D_800FF748 + ((unsigned int)oppId % 24u) * 1000;

        if (B(Scratchpad, 0x2A9) == 0) {
            if (W(Scratchpad, 0x2B0) == 1) {
                unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
                int dx = S(actor, 2) - S(ball, 2);
                int dz = S(actor, 6) - S(ball, 6);
                if (dx * dx + dz * dz <= 0x3FFFF) {
                    goto press_now;
                }
                {
                    signed short predZ = S(Scratchpad, 0x100);
                    int absPredZ = (predZ >= 0) ? predZ : -predZ;
                    if (absPredZ < 0x12F4) {
                        signed short predX = S(Scratchpad, 0x0FE);
                        if (atkDir != 0) {
                            if (-predX < lineX) {
                                goto press_now;
                            }
                        } else if (predX < lineX) {
                            goto press_now;
                        }
                    }
                }
            } else if (func_8005346C(actor) != 0) {
                B(actor, 0x3E1) = 0;
                H(actor, 0x3BC) = H(actor, 2);
                H(actor, 0x3BE) = H(actor, 6);
                return;
            }
        } else {
            int dx = S(actor, 2) - S(oppActor, 2);
            int dz = S(actor, 6) - S(oppActor, 6);
            if (dx * dx + dz * dz <= 0x3FFFF) {
                goto press_now;
            }
            if ((B(Scratchpad, 0x3D0) >> atkDir) & 1) {
                goto press_now;
            }
            {
                unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
                signed short bz = S(ball, 6);
                int absBz = (bz >= 0) ? bz : -bz;
                if (absBz < 0x14F4) {
                    signed short bx = S(ball, 2);
                    if (atkDir != 0) {
                        if (-bx < lineX) {
                            goto press_now;
                        }
                    } else if (bx < lineX) {
                        goto press_now;
                    }
                }
            }
        }

        if ((B(actor, 0x38D) % 22u) == (unsigned int)(SW(Scratchpad, 0x290) % 22)) {
            func_8009F1BC(actor);
            B(actor, 0x39C) = 1;
            H(actor, 0x3D8) = H(actor, 0x3D4);
        } else {
            H(actor, 0x3D6) = 0;
            B(actor, 0x39C) = 0;
            if (B(actor, 0x396) != 7) {
                H(actor, 0x3BC) = H(actor, 2);
                H(actor, 0x3BE) = H(actor, 6);
            }
        }
        B(Scratchpad, 0x28C) = (unsigned char)(H(actor, 0x3D4) >> 12);
        return;

    press_now:
        B(actor, 0x3E1) = 0;
        func_8005E510(actor);
        H(actor, 0x3BC) = H(actor, 2);
        H(actor, 0x3BE) = H(actor, 6);
        return;
    }

    func_8009F1BC(actor);
}
