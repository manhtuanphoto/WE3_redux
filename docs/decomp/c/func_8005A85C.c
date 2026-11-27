/*
 * func_8005A85C
 *
 * Purpose:
 *   Computes an AI player's tactical formation home/target (x, z) position on
 *   the pitch by looking up the player's formation zone cells in D_800E6C84
 *   and interpolating between own-half, midfield, and attacking zone bounds
 *   based on the ball (or target) position and possession state.
 *
 * Arguments:
 *   actor (unsigned char*) - pointer to 1000-byte player actor struct
 *   outX  (signed short*)  - output X coordinate (clamped to [-0x32A0, 0x32A0])
 *   outZ  (signed short*)  - output Z coordinate (clamped to [-0x2080, 0x2080])
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Actor fields:
 *     +0x38D (u8) actor id (1..22) (R)
 *     +0x391 (u8) formation slot index (R)
 *     +0x398 (u8) side (0/1) (R)
 *     +0x399 (u8) attack direction flag (R)
 *   Scratchpad (0x1F800000):
 *     +0x04A (s16), +0x07A (s16) ball predicted/future X, Z fallback (R)
 *     +0x0FE (s16), +0x100 (s16) open-play reference X, Z (R)
 *     +0x29A (u8)  match phase (1 = open play) (R)
 *     +0x2B0 (u32) ball event code (R)
 *     +0x2B8 (s16), +0x2BA (s16) ball target X, Z (0x7FFF = invalid) (R)
 *     +0x2CF (u8[2]) per-side possession flags (R)
 *     +0x2E0 (u8)  formation side XOR flag (R)
 *     +0x2F4 (u8*) pointer to ball actor (+0x02 s16 x, +0x06 s16 z) (R)
 *   Globals:
 *     D_800FF748 + 0xABE3 + 26 * (side ^ Scratchpad[0x2E0]) + actor->slot
 *     D_800E6C84 (and -0x14, -0x0A): tactical zone cell tables (stride 30 per side)
 *     D_800C7DB8, D_800C7DC8, D_800C7DD8, D_800C7DE8: u16 coordinate lookup tables
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x600  = 1536 raw (6.144 m) possession push-up offset / wing zone half-width
 *   0x32A0 = 12960 raw (51.84 m) pitch half-length X clamp
 *   0x2080 = 8320 raw (33.28 m) pitch half-width Z clamp
 *   13088 / 20 = 654.4 raw (2.618 m) per X interpolation unit (-20..+20)
 *   8448 / 25  = 337.92 raw (1.352 m) per Z interpolation unit (-25..+25)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_800E6C84[];
extern unsigned short D_800C7DB8[];
extern unsigned short D_800C7DC8[];
extern unsigned short D_800C7DD8[];
extern unsigned short D_800C7DE8[];

void func_8005A85C(unsigned char* actor, signed short* outX, signed short* outZ) {
    int teamIdx;
    int formIdx;
    unsigned char cellIdx;
    unsigned char atkDir;
    int dirSign;
    unsigned char side;
    unsigned int c0, c1, c2;
    int baseZ, baseZPlus, baseZMinus;
    int xLow, xHigh;
    unsigned short refX_u, refZ_u;
    signed short refX, refZ;
    int xScale, zScale;
    int sumX, sumZ;
    int finalX, finalZ;

    teamIdx = (int)(B(actor, 0x38D) - 1) / 11;
    formIdx = (signed short)(B(Scratchpad, 0x2E0) ^ teamIdx);
    cellIdx = (unsigned char)(B(D_800FF748 + 0xABE3 + formIdx * 26 + B(actor, 0x391), 0) - 1);

    atkDir = B(actor, 0x399);
    dirSign = (atkDir != 0) ? -1 : 1;
    side = B(actor, 0x398);

    c0 = B(D_800E6C84, side * 30 + cellIdx);
    c1 = B(D_800E6C84 - 0x14, side * 30 + cellIdx);
    c2 = B(D_800E6C84 - 0x0A, side * 30 + cellIdx);

    baseZ = D_800C7DD8[c0 / 7u] + D_800C7DE8[c0 % 7u];
    baseZPlus = baseZ + 0x600;
    baseZMinus = baseZ - 0x600;

    xLow = D_800C7DB8[c1 / 7u] + D_800C7DC8[c1 % 7u];
    xHigh = D_800C7DB8[c2 / 7u] + D_800C7DC8[c2 % 7u];

    if (B(Scratchpad, 0x29A) != 1) {
        unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
        refX_u = H(ball, 2);
        refZ_u = H(ball, 6);
    } else if (W(Scratchpad, 0x2B0) == 1) {
        if (B(Scratchpad, 0x2CF + side) == 1) {
            refX_u = (unsigned short)(H(Scratchpad, 0x0FE) - ((signed short)dirSign * 0x600));
            refZ_u = H(Scratchpad, 0x100);
        } else {
            refX_u = H(Scratchpad, 0x0FE);
            refZ_u = H(Scratchpad, 0x100);
        }
    } else if (S(Scratchpad, 0x2B8) != 0x7FFF) {
        refX_u = H(Scratchpad, 0x2B8);
        refZ_u = H(Scratchpad, 0x2BA);
    } else {
        refX_u = H(Scratchpad, 0x04A);
        refZ_u = H(Scratchpad, 0x07A);
    }

    refX = (signed short)refX_u;
    refZ = (signed short)refZ_u;

    if (atkDir != 0) {
        xScale = (signed short)(-((refX * 20) / 13088));
        sumX = -(((signed short)xLow * (20 - xScale)) + ((signed short)xHigh * (20 + xScale)));
        zScale = (signed short)((refZ * 25) / 8448);
        sumZ = -(((signed short)baseZMinus * (25 + zScale)) + ((signed short)baseZPlus * (25 - zScale)));
    } else {
        xScale = (signed short)((refX * 20) / 13088);
        sumX = ((signed short)xLow * (20 - xScale)) + ((signed short)xHigh * (20 + xScale));
        zScale = (signed short)((-(refZ * 25)) / 8448);
        sumZ = ((signed short)baseZMinus * (25 + zScale)) + ((signed short)baseZPlus * (25 - zScale));
    }

    finalX = sumX / 40;
    finalZ = sumZ / 50;

    if ((signed short)finalX < -0x32A0) {
        finalX = -0x32A0;
    } else if ((signed short)finalX >= 0x32A1) {
        finalX = 0x32A0;
    }

    if ((signed short)finalZ >= 0x2081) {
        finalZ = 0x2080;
    } else if ((signed short)finalZ < -0x2080) {
        finalZ = -0x2080;
    }

    *outX = (signed short)finalX;
    *outZ = (signed short)finalZ;
}
