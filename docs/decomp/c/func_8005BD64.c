/*
 * func_8005BD64
 *
 * Purpose:
 *   Updates player-to-player collision separation and cooldowns across all 22
 *   match players (actors 1..22). First decrements each player's collision
 *   cooldown timer (+0x39B). Then, for every active pair (i, j) within 160 raw
 *   units (0.64 m) in XZ, applies a brief cooldown if on opposite attack
 *   directions, pushes both players apart along the line between them, nudges
 *   ground-bound moving pairs apart by +-8 raw units (0.032 m) on the dominant
 *   axis, or damps airborne outfield players' XZ velocities to 80%.
 *
 * Arguments:
 *   void
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   D_800FF748 (actor array, stride 0x3E8 = 1000 bytes, indices 1..22):
 *     +0x000 (u8)  active flag (R)
 *     +0x002 (s16) x position (R/W)
 *     +0x004 (s16) y height (R)
 *     +0x006 (s16) z position (R/W)
 *     +0x008 (s32) vx velocity (R/W)
 *     +0x010 (s32) vz velocity (R/W)
 *     +0x38D (u8)  actor id (1..22; 1 and 12 are goalkeepers) (R)
 *     +0x399 (u8)  attack direction flag (R)
 *     +0x39B (u8)  collision cooldown timer (R/W)
 *     +0x3A0 (u32) speed (R)
 *     +0x3BA (u8)  balance/strength stat or priority byte (R)
 *     +0x3D2 (u8)  skip-collision flag (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x3E8  = 1000 bytes per actor struct
 *   0x6400 = 25600 = 160^2 (collision radius 160 raw = 0.64 m)
 *   0xA0   = 160 raw (0.64 m) separation range; push = ((160 - dist) >> 1) + 2
 *   8      = 8 raw units (0.032 m) extra ground separation step
 *   3      = 3-frame (0.10 s) collision cooldown stored in +0x39B
 *   8 / 10 = 80% XZ velocity retention when colliding while airborne
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];

extern unsigned int func_800B1128(unsigned int sqDist);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8001E694(unsigned int angle, int scale);
extern int func_8001E5A4(unsigned int angle, int scale);

void func_8005BD64(void) {
    int i;
    int j;
    unsigned char* actor1;
    unsigned char* actor2;

    /* Decrement collision cooldown (+0x39B) for actors 1..22. */
    actor1 = D_800FF748 + 0x3E8;
    for (i = 1; i < 23; i++) {
        if (B(actor1, 0x39B) != 0) {
            B(actor1, 0x39B)--;
        }
        actor1 += 0x3E8;
    }

    actor1 = D_800FF748 + 0x3E8;
    for (i = 1; i < 22; i++) {
        if (B(actor1, 0x0) != 0 && B(actor1, 0x3D2) == 0) {
            actor2 = D_800FF748 + (B(actor1, 0x38D) * 0x3E8) + 0x3E8;
            j = B(actor2, 0x38D);
            if (j < 23) {
                do {
                    if (B(actor2, 0x0) != 0 && B(actor2, 0x3D2) == 0) {
                        int dx = S(actor1, 0x2) - S(actor2, 0x2);
                        int dz = S(actor1, 0x6) - S(actor2, 0x6);
                        unsigned int distSq = (unsigned int)(dx * dx + dz * dz);

                        if (distSq < 0x6401U) {
                            unsigned int dist;
                            unsigned int angle;
                            unsigned int oppAngle;
                            unsigned int push;
                            int x1, x2, z1, z2;

                            if (B(actor1, 0x399) != B(actor2, 0x399)) {
                                unsigned char stat1 = B(actor1, 0x3BA);
                                unsigned char stat2 = B(actor2, 0x3BA);
                                if ((int)stat2 >= (int)stat1 - 2) {
                                    B(actor1, 0x39B) = 3;
                                } else if ((int)stat1 >= (int)stat2 - 2) {
                                    B(actor2, 0x39B) = 3;
                                }
                            }

                            dist = func_800B1128(distSq);
                            angle = func_8005B36C(S(actor1, 0x2), S(actor1, 0x6), S(actor2, 0x2), S(actor2, 0x6));
                            push = ((0xA0U - dist) >> 1) + 2U;
                            oppAngle = (angle + 0x80U) & 0xFFU;

                            S(actor1, 0x2) += func_8001E694(oppAngle, (int)push);
                            S(actor1, 0x6) += func_8001E5A4(oppAngle, (int)push);
                            angle &= 0xFFU;
                            S(actor2, 0x2) += func_8001E694(angle, (int)push);
                            z2 = S(actor2, 0x6) + func_8001E5A4(angle, (int)push);
                            S(actor2, 0x6) = (signed short)z2;

                            if (S(actor1, 0x4) == 0 && S(actor2, 0x4) == 0) {
                                if (W(actor1, 0x3A0) + W(actor2, 0x3A0) != 0) {
                                    int adx;
                                    int adz;
                                    x1 = S(actor1, 0x2);
                                    x2 = S(actor2, 0x2);
                                    z1 = S(actor1, 0x6);
                                    z2 = (signed short)z2;

                                    adx = x1 - x2;
                                    if (adx < 0) {
                                        adx = -adx;
                                    }
                                    adz = z1 - z2;
                                    if (adz < 0) {
                                        adz = -adz;
                                    }

                                    if (adz < adx) {
                                        if (z2 < z1) {
                                            S(actor1, 0x6) = (signed short)(z1 + 8);
                                            S(actor2, 0x6) = (signed short)(S(actor2, 0x6) - 8);
                                        } else {
                                            S(actor1, 0x6) = (signed short)(z1 - 8);
                                            S(actor2, 0x6) = (signed short)(S(actor2, 0x6) + 8);
                                        }
                                    } else {
                                        if (SW(actor2, 0x8) < SW(actor1, 0x8)) {
                                            S(actor1, 0x2) = (signed short)(x1 + 8);
                                            S(actor2, 0x2) = (signed short)(S(actor2, 0x2) - 8);
                                        } else {
                                            S(actor1, 0x2) = (signed short)(x1 - 8);
                                            S(actor2, 0x2) = (signed short)(S(actor2, 0x2) + 8);
                                        }
                                    }
                                }
                            } else {
                                if (B(actor1, 0x38D) != 1 && B(actor1, 0x38D) != 12) {
                                    SW(actor1, 0x8) = (SW(actor1, 0x8) * 8) / 10;
                                    SW(actor1, 0x10) = (SW(actor1, 0x10) * 8) / 10;
                                }
                                if (B(actor2, 0x38D) != 1 && B(actor2, 0x38D) != 12) {
                                    SW(actor2, 0x8) = (SW(actor2, 0x8) * 8) / 10;
                                    SW(actor2, 0x10) = (SW(actor2, 0x10) * 8) / 10;
                                }
                            }
                        }
                    }
                    j++;
                    actor2 += 0x3E8;
                } while (j < 23);
            }
        }
        actor1 += 0x3E8;
    }
}
