/*
 * func_800691A8
 *
 * Purpose:
 *   Adjusts the AI off-ball / tactical target coordinates in Scratchpad+0xF8/+0xFC
 *   according to the team's tactical level (Scratchpad+0x2FF[side]), attack
 *   direction, and active sub-mode (Scratchpad+0x30B = 0..5), adding positional
 *   jitter, offside-line clamping, and field-length bounds.
 *
 * Arguments:
 *   actor (u8*) - pointer to the 1000-byte player actor struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x0F8 (s16) target x coordinate (R/W)
 *     +0x0FC (s16) target z coordinate (R/W)
 *     +0x2A9 (u8)  ball owner ID (0 = free) (R)
 *     +0x2E1 (u8)  mode (5 suppresses offside-line pull in cases 2..3) (R)
 *     +0x2F4 (u8*) pointer to ball actor (R)
 *     +0x2FF (u8[2]) per-side tactic level (0, 1, 2) (R)
 *     +0x30B (u8)  AI positioning sub-mode (0..5) (R)
 *     +0x314 (s32[2]) per-attack-direction x line thresholds (R)
 *   Actor:
 *     +0x002 (s16) x position (R)
 *     +0x392 (u8)  team slot 0..10 (R)
 *     +0x398 (u8)  side 0/1 (R)
 *     +0x399 (u8)  attack direction flag (0 -> +1, 1 -> -1) (R)
 *     +0x3DA (u8)  tick/jitter phase counter (R)
 *     +0x3DC (s16) cached x offset jitter (W/R)
 *     +0x3DE (s16) cached z offset jitter (W/R)
 *   Owner actor (D_800FF748 + (ownerId % 24) * 1000):
 *     +0x006 (s16) z position (R)
 *     +0x391 (u8)  role/position code (1 or 2 flips target z away from owner side) (R)
 *   Ball (Scratchpad+0x2F4):
 *     +0x002 (s16) ball x position (R)
 *   Globals:
 *     D_800FF748 (u8[]) actor array (24 x 1000 bytes) (R)
 *     D_8010C328 (u32[]) player stat table indexed by side*264 + slot*12 (low nibble) (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x300 = 768 raw = 3.072 m; 0x500 = 1280 raw = 5.120 m
 *   0x600 = 1536 raw = 6.144 m; 0x700 = 1792 raw = 7.168 m
 *   0x800 = 2048 raw = 8.192 m (ball x lead offset in case 4)
 *   0x2E08 = 11784 raw = 47.136 m (outer x clamp in case 4)
 *   0x28E2 = 10466 raw = 41.864 m (inner x push-out threshold in cases 4..5)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010C328[];

extern int func_8001E6B8(unsigned int);
extern int func_8005A76C(int, int, int);

void func_800691A8(unsigned char* actor) {
    int dirSign;
    unsigned char sideTactic;
    unsigned char subMode;
    int deltaX;
    int xVal;

    dirSign = (B(actor, 0x399) == 0) ? 1 : -1;

    sideTactic = B(Scratchpad, 0x2FF + B(actor, 0x398));
    subMode = B(Scratchpad, 0x30B);
    if (sideTactic < 2) {
        if (subMode < 4) {
            deltaX = (short)dirSign * 3 * 256; /* 768 = 3.072 m */
            S(Scratchpad, 0xF8) += (short)deltaX;
        } else if (subMode < 6) {
            deltaX = (short)dirSign * 5 * 256; /* 1280 = 5.120 m */
            S(Scratchpad, 0xF8) += (short)deltaX;
        }
    } else if (sideTactic == 2) {
        if (subMode < 4) {
            deltaX = (short)dirSign * 3 * 512; /* 1536 = 6.144 m */
            S(Scratchpad, 0xF8) += (short)deltaX;
        } else if (subMode < 6) {
            deltaX = (short)dirSign * 7 * 256; /* 1792 = 7.168 m */
            S(Scratchpad, 0xF8) += (short)deltaX;
        }
    }

    switch (B(Scratchpad, 0x30B)) {
    case 0:
    case 1:
        H(actor, 0x3DE) = 0;
        H(actor, 0x3DC) = 0;
        return;

    case 2:
    case 3:
        if ((B(actor, 0x3DA) & 3) == 0) {
            S(actor, 0x3DC) = (short)(func_8001E6B8(4) * (dirSign << 8));
            S(actor, 0x3DE) = (short)((func_8001E6B8(7) << 8) - 0x300);
        }
        if (B(Scratchpad, 0x2E1) != 5) {
            unsigned int row = (unsigned int)B(actor, 0x392) * 12 + (unsigned int)B(actor, 0x398) * 264;
            unsigned int stat = W(D_8010C328, row) & 0xF;
            if (func_8001E6B8(20U - (stat << 1)) == 0) {
                int lineX = SW(Scratchpad, 0x314 + ((B(actor, 0x399) ^ 1) << 2));
                S(actor, 0x3DC) = (short)((lineX - (int)H(actor, 2)) + (dirSign << 10));
            }
        }
        S(Scratchpad, 0xF8) += S(actor, 0x3DC);
        S(Scratchpad, 0xFC) += S(actor, 0x3DE);
        return;

    case 4:
    {
        int absX;
        S(Scratchpad, 0xFC) = (short)(S(Scratchpad, 0xFC) / 2);
        if ((B(actor, 0x3DA) % 3U) == 0) {
            S(actor, 0x3DC) = (short)(func_8001E6B8(5) * (dirSign << 8));
            S(actor, 0x3DE) = (short)((func_8001E6B8(9) << 8) - 0x400);
        }
        S(Scratchpad, 0xF8) += S(actor, 0x3DC);
        S(Scratchpad, 0xFC) += S(actor, 0x3DE);

        xVal = func_8005A76C(S(Scratchpad, 0xF8), S(W(Scratchpad, 0x2F4), 2) + 0x800, 4);
        S(Scratchpad, 0xF8) = (short)xVal;
        xVal = S(Scratchpad, 0xF8);
        absX = (xVal < 0) ? -xVal : xVal;
        if (absX >= 0x2E09) {
            S(Scratchpad, 0xF8) = (xVal >= 0) ? 0x2E08 : -0x2E08;
        }
        xVal = S(Scratchpad, 0xF8);
        break;
    }

    case 5:
    {
        unsigned char ownerId;
        S(Scratchpad, 0xF8) = (short)func_8005A76C(S(Scratchpad, 0xF8), S(W(Scratchpad, 0x2F4), 2), 3);
        ownerId = B(Scratchpad, 0x2A9);
        if (ownerId != 0) {
            unsigned char* owner = D_800FF748 + (ownerId % 24) * 0x3E8;
            if ((unsigned int)(B(owner, 0x391) - 1) < 2U) {
                int zSign = (S(Scratchpad, 0xFC) >= 0) ? 1 : -1;
                int ownerZSign = (S(owner, 6) >= 0) ? 1 : -1;
                if (zSign == ownerZSign) {
                    S(Scratchpad, 0xFC) = (short)(-S(Scratchpad, 0xFC));
                }
            }
        }
        S(Scratchpad, 0xFC) = (short)((S(Scratchpad, 0xFC) * 2) / 3);
        xVal = S(Scratchpad, 0xF8);
        break;
    }

    default:
        return;
    }

    /* Shared post-clamp for cases 4 and 5: push target X outside (-0x28E2, +0x28E2) */
    {
        int absX = (xVal < 0) ? -xVal : xVal;
        if (absX < 0x28E2) {
            S(Scratchpad, 0xF8) = (xVal >= 0) ? 0x28E2 : -0x28E2;
        }
    }
}
