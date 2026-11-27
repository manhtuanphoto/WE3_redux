/*
 * func_8001D6A8
 *
 * Purpose:
 *   Per-frame render-loop dispatcher: transforms and renders all active actors
 *   (with optional distance/horizontal culling during open-play broadcast camera),
 *   runs scene/stadium/ball render helpers, projects off-screen player indicator
 *   markers during match/replay states, updates OT/HUD sprite objects, and
 *   renders active 2D/3D overlay elements.
 *
 * Arguments:
 *   void
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Actor fields (24 x 1000-byte structs at D_800FF748):
 *     +0x000 (u8)  active flag (R)
 *     +0x002 (s16) x, +0x004 (s16) y, +0x006 (s16) z (R)
 *     +0x014 (u16) projected screen X, +0x016 (u16) projected screen Y (R)
 *     +0x024..+0x028 (u16[3]) rotation angles (R)
 *     +0x02C..+0x034 (u32[3]) translation/offset vector (R)
 *     +0x03C (u32) cleared to 0 before func_800B6A78 (W)
 *     +0x040..+0x05C (u32[8]) transformed matrix/pos cache (W)
 *     +0x3D2 (u8)  off-screen culled flag (W = 1 when culled)
 *   Scratchpad (0x1F800000):
 *     +0x104..+0x120 (matrix/vector workspace) (R/W)
 *     +0x124..+0x128 (u16[3]) rotation workspace (W)
 *     +0x13C..+0x144 (s32[3]) position workspace (W)
 *     +0x14C..+0x154 (u32[3]) scale/offset workspace (W)
 *     +0x1EC (u8)  staggered update flag (R)
 *     +0x1FC (s32) camera/reference X (R)
 *     +0x204 (s32) camera/reference Z (R)
 *     +0x240..+0x241 (u8[2]) tracked/selected actor IDs for HUD markers (R)
 *     +0x290 (u32) frame counter (R)
 *     +0x298 (u8)  game state (R)
 *     +0x29A (u8)  match phase (1 = open play) (R)
 *     +0x2A0 (u8), +0x2C0 (u8) overlay/radar flags (R)
 *     +0x2A4 (u32) saved pointer copied to D_800EF8A8 (R)
 *     +0x2E1 (u8)  mode (R)
 *     +0x2E4 (u8), +0x2E5 (u8) render/camera mode flags (R)
 *     +0x2E6 (u8)  camera preset index (R)
 *     +0x2F0 (u8)  camera type (R)
 *     +0x330 (u8)  extra render pass flag (R)
 *   Globals:
 *     D_800FF748: actor array (24 x 1000 bytes), plus HUD/overlay structs at
 *       +0x5DC0 (slot 0), +0x5DE8 (slots 1..57, stride 0x28), +0x65F0 (markers),
 *       +0x6642/+0x6645/+0x666A/+0x6692 (indicator state), +0xABE0 (u16 flag)
 *     D_800EF8A8: receives Scratchpad+0x2A4
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x800 = 2048 raw (8.192 m) base horizontal cull half-width
 *   0x64 / -0x60 = +100 / -96 screen Y clamp; 0xA8 / -0xA8 = +-168 screen X clamp
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned int D_800EF8A8;

extern void func_800B1488(unsigned char*, unsigned char*);
extern void func_800B1348(unsigned char*, unsigned char*);
extern void func_800B1318(unsigned char*, unsigned char*);
extern void func_800B6A78(unsigned char*, unsigned char*);
extern void func_8001BE00(unsigned char*);
extern void func_8001C5A8(void);
extern void func_8001CA00(void);
extern void func_8009C6E8(void);
extern void func_80089900(void);
extern void func_8001AC74(void);
extern unsigned char* func_8001D004(int);
extern int func_8001B894(int);
extern void func_8006F360(void);
extern void func_80019C34(unsigned char*);
extern void func_8001B0B8(void);

void func_8001D6A8(void) {
    int doCull;
    int cullHalfWidth = 0;
    int i;
    unsigned char* actor;
    unsigned char* actorTransform;
    unsigned int gameState;
    unsigned char* markerPos;
    unsigned char* markerFlag;
    unsigned char* objPtr;
    int retVal;

    if (B(Scratchpad, 0x29A) == 1 && B(Scratchpad, 0x2F0) == 2 && B(Scratchpad, 0x2E6) < 5) {
        int absCamX = SW(Scratchpad, 0x1FC);
        if (absCamX < 0) {
            absCamX = -absCamX;
        }
        doCull = 1;
        cullHalfWidth = ((absCamX << 8) / 13088) + 0x800;
    } else {
        doCull = 0;
    }

    actor = D_800FF748;
    actorTransform = D_800FF748 + 0x3C;
    for (i = 0; i < 24; i++) {
        if (B(actor, 0) != 0) {
            if (doCull) {
                int refZ = SW(Scratchpad, 0x204);
                int dx = SW(Scratchpad, 0x1FC) - S(actor, 2);
                int dz;
                if (dx < 0) {
                    dx = -dx;
                }
                dz = (signed short)(H(actor, 6) - refZ);
                if (cullHalfWidth + (dz >> 2) < dx) {
                    B(actor, 0x3D2) = 1;
                    goto next_actor;
                }
            }

            SW(Scratchpad, 0x13C) = S(actor, 2);
            SW(Scratchpad, 0x140) = S(actor, 4);
            SW(Scratchpad, 0x144) = S(actor, 6);
            H(Scratchpad, 0x124) = H(actor, 0x24);
            H(Scratchpad, 0x126) = H(actor, 0x26);
            H(Scratchpad, 0x128) = H(actor, 0x28);
            W(Scratchpad, 0x14C) = W(actor, 0x2C);
            W(Scratchpad, 0x150) = W(actor, 0x30);
            W(Scratchpad, 0x154) = W(actor, 0x34);

            func_800B1488(Scratchpad + 0x124, Scratchpad + 0x104);
            func_800B1348(Scratchpad + 0x104, Scratchpad + 0x14C);
            func_800B1318(Scratchpad + 0x104, Scratchpad + 0x13C);

            W(actorTransform, 0x04) = W(Scratchpad, 0x104);
            W(actorTransform, 0x08) = W(Scratchpad, 0x108);
            W(actorTransform, 0x0C) = W(Scratchpad, 0x10C);
            W(actorTransform, 0x10) = W(Scratchpad, 0x110);
            W(actorTransform, 0x14) = W(Scratchpad, 0x114);
            W(actorTransform, 0x18) = W(Scratchpad, 0x118);
            W(actorTransform, 0x1C) = W(Scratchpad, 0x11C);
            W(actorTransform, 0x20) = W(Scratchpad, 0x120);
            W(actorTransform, 0x00) = 0;

            func_800B6A78(actorTransform, Scratchpad + 0x104);
            func_8001BE00(actor);
        }
    next_actor:
        actor += 0x3E8;
        actorTransform += 0x3E8;
    }

    func_8001C5A8();
    func_8001CA00();

    if (H(D_800FF748, 0xABE0) == 0x100 && B(Scratchpad, 0x2E1) != 5) {
        func_8009C6E8();
    }

    gameState = B(Scratchpad, 0x298);
    if (gameState == 5 || gameState == 3 || gameState == 6 || gameState == 0x63) {
        if (B(Scratchpad, 0x2A0) == 0 && B(Scratchpad, 0x2C0) == 1) {
            func_80089900();
        }
        func_8001AC74();

        markerFlag = func_8001D004(0x25) + 3;
        markerPos = D_800FF748 + 0x65F0;
        for (i = 0; i < 2; i++) {
            unsigned int actorId = B(Scratchpad, 0x240 + i) % 24u;
            unsigned char* targetActor = D_800FF748 + actorId * 1000;

            B(markerFlag, 0) = 0x90;
            H(markerPos, 0) = H(targetActor, 0x14);
            H(markerPos, 2) = H(targetActor, 0x16);

            if (S(markerPos, 2) >= 0x64) {
                S(markerPos, 2) = 0x64;
                B(markerFlag, 0) = 0x90;
            }
            if (S(markerPos, 2) < -0x5F) {
                S(markerPos, 2) = -0x60;
                B(markerFlag, 0) = 0x80;
            }
            if (S(markerPos, 0) >= 0xA8) {
                S(markerPos, 0) = 0xA8;
                B(markerFlag, 0) = 0xB0;
            }
            if (S(markerPos, 0) < -0xA7) {
                S(markerPos, 0) = -0xA8;
                B(markerFlag, 0) = 0xA0;
            }

            markerPos += 0x28;
            markerFlag += 0xC;
        }

        retVal = func_8001B894(0);
        if (B(Scratchpad, 0x2E4) == 1) {
            S(D_800FF748, 0x6692) = (signed short)retVal;
            B(D_800FF748, 0x6645) = (unsigned char)((SW(Scratchpad, 0x290) % 5) - 6);
        } else if (B(Scratchpad, 0x2E5) == 0) {
            S(D_800FF748, 0x666A) = (signed short)(retVal - 0x80);
            S(D_800FF748, 0x6642) = (signed short)retVal;
            S(D_800FF748, 0x6692) = (signed short)(retVal - 0x100);
        } else {
            S(D_800FF748, 0x6642) = (signed short)retVal;
        }
    } else if (gameState == 6 || gameState == 0x63) {
        func_8001AC74();
    }

    D_800EF8A8 = W(Scratchpad, 0x2A4);
    if (B(Scratchpad, 0x330) == 1) {
        func_8006F360();
    }

    objPtr = D_800FF748 + 0x5DC0;
    if (B(D_800FF748, 0x5DC4) != 0 && W(D_800FF748, 0x5DCC) != 0) {
        func_80019C34(objPtr);
    }

    objPtr = D_800FF748 + 0x5DE8;
    if (B(Scratchpad, 0x1EC) == 0) {
        func_8001B0B8();
    }

    for (i = 1; i < 0x3A; i++) {
        if (B(objPtr, 4) != 0 && W(objPtr, 0xC) != 0) {
            func_80019C34(objPtr);
        }
        objPtr += 0x28;
    }
}
