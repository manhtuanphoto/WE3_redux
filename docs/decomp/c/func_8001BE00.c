/*
 * func_8001BE00
 *
 * Purpose:
 *   Projects an actor to screen space, performs off-screen culling, sets up
 *   the actor's render/lighting context, and iterates over its skeletal limbs
 *   (actor+0x3A5 .. actor+0x3A6 - 1) to build mirrored/blended limb matrices
 *   (including root head/body ball-tracking on limb 0) and submit limb meshes
 *   to the renderer via func_800B2728.
 *
 * Arguments:
 *   actor (unsigned char *) - pointer to 1000-byte actor struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Actor fields:
 *     +0x002 (s16) x, +0x006 (s16) z (R)
 *     +0x014 (s16) projected screen X (W/R)
 *     +0x016 (s16) projected screen Y (W/R)
 *     +0x01C (u32) pointer to keyframe bone array (12 bytes per limb) (R)
 *     +0x03C (u8[]) actor local matrix/transform buffer (address stored to Scratchpad+0x384) (R)
 *     +0x08C, +0x0EC, +0x14C, +0x1AC (u32[24]) per-limb mesh/material pointers (R)
 *     +0x20C..+0x228 (32 bytes per limb) previous-frame limb transform cache for blending (R/W)
 *     +0x38D (u8)  actor id (R)
 *     +0x3A5 (u8)  first limb index (R)
 *     +0x3A6 (u8)  end limb index (exclusive) (R)
 *     +0x3A7 (u8)  ball-tracking enable flag on root limb (R/W, cleared after root)
 *     +0x3A8 (u8)  smoothed head/look yaw offset (R/W)
 *     +0x3A9 (u8)  head/look pitch toward ball (R/W)
 *     +0x3AA (u8)  body heading (R)
 *     +0x3AC (u8)  mirror/foot flag (R)
 *     +0x3AD (u8)  blend timer (averages current limb with cached limb when != 0, decremented) (R/W)
 *     +0x3D2 (u8)  off-screen culled flag (1 = culled, 0 = visible) (W)
 *     +0x3D3 (u8)  force-visible / skip-cull flag (R)
 *   Scratchpad (0x1F800000):
 *     +0x000 (u32) OT shift base (0xE - W(0)) (R)
 *     +0x104..+0x120 (transform/matrix workspace) (R/W)
 *     +0x118, +0x11C, +0x120 (s32) camera-space X, Y, Z (R)
 *     +0x134..+0x138 (s16[3]) Euler rotation input (W)
 *     +0x1EC (u8)  staggered look-at update flag (update every 11 frames by actor_id % 11) (R)
 *     +0x290 (u32) frame counter (R)
 *     +0x29D (u8)  display buffer index (R)
 *     +0x2A4 (u32) previous D_800F1018 pointer (R/W)
 *     +0x2E4 (u8)  render mode flag A (R)
 *     +0x2E5 (u8)  render mode flag B (R)
 *     +0x2F4 (u8*) pointer to ball actor (R)
 *     +0x338 (u8)  screen Y offset selector (0 -> -0x210, non-zero -> -0x2A0) (R)
 *     +0x33C..+0x35C (limb matrix + translation workspace) (R/W)
 *     +0x384 (u32) set to actor + 0x3C (W)
 *   Globals:
 *     D_800F1018 (u8[]) render/lighting table (20-byte stride per display buffer)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x200 / 0x201 = 512 / 513 min depth clamp
 *   [-0xD0, +0xC8] = [-208, +200] screen X cull bounds; [-0x98, +0x68] = [-152, +104] screen Y cull bounds
 *   0x1A0 = 416 raw (1.664 m) head height offset for ball pitch; -0x1BF = -447 raw (1.788 m) high-ball threshold
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800F1018[];

extern void func_8001A19C(unsigned char*);
extern void func_8001A544(unsigned char*);
extern void func_8001A814(unsigned char*);
extern void func_8001A968(unsigned char*);
extern unsigned int func_800B1128(int);
extern unsigned int func_8005B36C(int, int, int, int);
extern unsigned int func_8005B7CC(unsigned int, unsigned int, unsigned int);
extern void func_800B2388(unsigned char*, unsigned char*);
extern void func_800B1F78(unsigned char*, unsigned char*);
extern void func_800B6A78(unsigned char*, unsigned char*);
extern void func_800B4A14(unsigned char*);
extern void func_800B2728(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, int, unsigned int);

void func_8001BE00(unsigned char* actor) {
    int depth;
    int scrY;
    int limbIdx;
    int endLimb;
    int boneStep;
    unsigned char* cachePtr;
    unsigned char* arrPtr;

    depth = SW(Scratchpad, 0x120);
    if (depth < 0x201) {
        depth = 0x200;
    }
    S(actor, 0x14) = (signed short)((SW(Scratchpad, 0x118) << 10) / depth);

    if (B(Scratchpad, 0x338) != 0) {
        scrY = SW(Scratchpad, 0x11C) - 0x2A0;
    } else {
        scrY = SW(Scratchpad, 0x11C) - 0x210;
    }
    S(actor, 0x16) = (signed short)((scrY << 10) / depth);

    if (B(actor, 0x3D3) == 0) {
        if ((unsigned short)(H(actor, 0x14) + 0xD0) >= 0x199 ||
            (unsigned short)(H(actor, 0x16) + 0x98) >= 0x101) {
            B(actor, 0x3D2) = 1;
            return;
        }
    }
    B(actor, 0x3D2) = 0;

    if (B(actor, 0x38D) == 0) {
        func_8001A19C(actor);
    } else if (B(Scratchpad, 0x2E5) != 0) {
        func_8001A968(actor);
    } else if (B(Scratchpad, 0x2E4) != 0) {
        func_8001A814(actor);
    } else {
        func_8001A544(actor);
    }

    limbIdx = B(actor, 0x3A5);
    endLimb = B(actor, 0x3A6);
    boneStep = 0;
    cachePtr = actor;
    arrPtr = actor + limbIdx * 4;

    while (limbIdx < endLimb) {
        if (limbIdx != 0x14 && limbIdx != 0x17) {
            unsigned char* work = Scratchpad + 0x33C;
            unsigned char* keyPtr = (unsigned char*)W(actor, 0x1C) + boneStep * 12;
            unsigned int otBuf;
            unsigned int prevBuf;

            W(Scratchpad, 0x384) = (unsigned int)(actor + 0x3C);
            W(Scratchpad, 0x33C) = 0;

            if (boneStep == 0 && B(actor, 0x3A7) == 1) {
                int pitchRot;
                if (B(Scratchpad, 0x1EC) == 0 ||
                    (SW(Scratchpad, 0x290) % 11) == (int)(B(actor, 0x38D) % 11)) {
                    unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
                    int dx = S(actor, 0x02) - S(ball, 0x02);
                    int dz = S(actor, 0x06) - S(ball, 0x06);
                    unsigned int dist = func_800B1128(dx * dx + dz * dz);
                    int dy = S(ball, 0x04) + 0x1A0;
                    unsigned int pitch;
                    unsigned int ang;
                    unsigned int turnTarget;

                    if (dy < 0) {
                        dy = -dy;
                    }
                    pitch = ((unsigned int)dy << 5) / (dist + 0x10);
                    if ((pitch & 0xFF) >= 0x19) {
                        pitch = 0x18;
                    }
                    B(actor, 0x3A9) = (unsigned char)pitch;

                    ang = func_8005B36C(S(actor, 0x02), S(actor, 0x06), S(ball, 0x02), S(ball, 0x06));
                    turnTarget = func_8005B7CC((B(actor, 0x3AA) - ang) & 0xFF, 0, 0x34);
                    B(actor, 0x3A8) = (unsigned char)func_8005B7CC(turnTarget & 0xFF, B(actor, 0x3A8), 0x20);
                }

                if (S((unsigned char*)W(Scratchpad, 0x2F4), 0x04) < -0x1BF) {
                    pitchRot = 0x100 - B(actor, 0x3A9);
                } else {
                    pitchRot = ((signed int)(B(actor, 0x3A9) * 7)) / 10;
                }
                H(Scratchpad, 0x134) = (unsigned short)(pitchRot << 4);
                H(Scratchpad, 0x136) = (unsigned short)(B(actor, 0x3A8) << 4);
                H(Scratchpad, 0x138) = 0;
                func_800B2388(Scratchpad + 0x134, work + 4);

                SW(work, 0x18) = B(actor, 0x3AC) ? -S(keyPtr, 6) : S(keyPtr, 6);
                SW(work, 0x1C) = S(keyPtr, 8);
                SW(work, 0x20) = S(keyPtr, 10);
                B(actor, 0x3A7) = 0;
            } else {
                if (B(actor, 0x3AC) == 0) {
                    H(Scratchpad, 0x134) = H(keyPtr, 0);
                    H(Scratchpad, 0x136) = H(keyPtr, 2);
                    H(Scratchpad, 0x138) = H(keyPtr, 4);
                    func_800B1F78(Scratchpad + 0x134, work + 4);
                    SW(work, 0x18) = S(keyPtr, 6);
                } else {
                    unsigned int mirrorIdx = (unsigned int)(boneStep - 2);
                    if (mirrorIdx < 10) {
                        switch (mirrorIdx) {
                        case 0:
                        case 1:
                            keyPtr += 0x18;
                            break;
                        case 2:
                        case 3:
                            keyPtr -= 0x18;
                            break;
                        case 4:
                        case 5:
                        case 6:
                            keyPtr += 0x0C;
                            break;
                        case 7:
                        case 8:
                        case 9:
                            keyPtr -= 0x0C;
                            break;
                        }
                        H(Scratchpad, 0x134) = (unsigned short)(0x800 - H(keyPtr, 0));
                        H(Scratchpad, 0x136) = H(keyPtr, 2);
                        H(Scratchpad, 0x138) = (unsigned short)(0x800 - H(keyPtr, 4));
                    } else {
                        H(Scratchpad, 0x134) = H(keyPtr, 0);
                        H(Scratchpad, 0x136) = (unsigned short)(-H(keyPtr, 2));
                        H(Scratchpad, 0x138) = (unsigned short)(-H(keyPtr, 4));
                    }
                    func_800B1F78(Scratchpad + 0x134, work + 4);
                    SW(work, 0x18) = -S(keyPtr, 6);
                }
                SW(work, 0x1C) = S(keyPtr, 8);
                SW(work, 0x20) = S(keyPtr, 10);
            }

            if (B(actor, 0x3AD) != 0) {
                S(work, 0x04) = (S(work, 0x04) + S(cachePtr, 0x20C)) >> 1;
                S(work, 0x06) = (S(work, 0x06) + S(cachePtr, 0x20E)) >> 1;
                S(work, 0x08) = (S(work, 0x08) + S(cachePtr, 0x210)) >> 1;
                S(work, 0x0A) = (S(work, 0x0A) + S(cachePtr, 0x212)) >> 1;
                S(work, 0x0C) = (S(work, 0x0C) + S(cachePtr, 0x214)) >> 1;
                S(work, 0x0E) = (S(work, 0x0E) + S(cachePtr, 0x216)) >> 1;
                S(work, 0x10) = (S(work, 0x10) + S(cachePtr, 0x218)) >> 1;
                S(work, 0x12) = (S(work, 0x12) + S(cachePtr, 0x21A)) >> 1;
                S(work, 0x14) = (S(work, 0x14) + S(cachePtr, 0x21C)) >> 1;
                SW(work, 0x18) = (SW(work, 0x18) + SW(cachePtr, 0x220)) >> 1;
                SW(work, 0x1C) = (SW(work, 0x1C) + SW(cachePtr, 0x224)) >> 1;
                SW(work, 0x20) = (SW(work, 0x20) + SW(cachePtr, 0x228)) >> 1;
            }

            W(cachePtr, 0x20C) = W(work, 0x04);
            W(cachePtr, 0x210) = W(work, 0x08);
            W(cachePtr, 0x214) = W(work, 0x0C);
            W(cachePtr, 0x218) = W(work, 0x10);
            W(cachePtr, 0x21C) = W(work, 0x14);
            W(cachePtr, 0x220) = W(work, 0x18);
            W(cachePtr, 0x224) = W(work, 0x1C);
            W(cachePtr, 0x228) = W(work, 0x20);

            func_800B6A78(work, Scratchpad + 0x104);
            func_800B4A14(Scratchpad + 0x104);

            otBuf = (unsigned int)(D_800F1018 + B(Scratchpad, 0x29D) * 20);
            prevBuf = W(Scratchpad, 0x2A4);
            W(Scratchpad, 0x2A4) = otBuf;
            func_800B2728(W(arrPtr, 0x14C), W(arrPtr, 0x8C), W(arrPtr, 0xEC),
                          prevBuf, W(arrPtr, 0x1AC),
                          0x0E - W(Scratchpad, 0x000), otBuf);
        }

        arrPtr += 4;
        limbIdx++;
        cachePtr += 0x20;
        endLimb = B(actor, 0x3A6);
        boneStep++;
    }

    if (B(actor, 0x3AD) != 0) {
        B(actor, 0x3AD) = B(actor, 0x3AD) - 1;
    }
}
