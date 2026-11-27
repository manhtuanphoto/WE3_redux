/*
 * func_800A4D30
 *
 * Purpose:
 *   Computes an actor's target pitch position (*outX, *outZ), starting from
 *   the base formation position (func_8005A85C) and applying tactical role
 *   (+0x391), special positioning flag (+0x3E3 / +0x3E4), or set-piece
 *   marker (func_80067F78) adjustments before clamping to pitch bounds.
 *
 * Arguments:
 *   actor (u8 *)  : pointer to 1000-byte actor struct
 *   outX  (s16 *) : output target X position (raw units, 250 = 1 m)
 *   outZ  (s16 *) : output target Z position (raw units, 250 = 1 m)
 *
 * Return value:
 *   None (void)
 *
 * Actor fields read:
 *   +0x06 (s16) z position, +0x38D (u8) actor id,
 *   +0x391 (u8) tactical role index (1..25), +0x398 (u8) side (0/1),
 *   +0x399 (u8) attack direction flag (0 -> +1, nonzero -> -1),
 *   +0x3E3 (u8) special positioning mode (0xFD checked),
 *   +0x3E4 (u8) special positioning table index (0..6)
 *
 * Actor fields written:
 *   None
 *
 * Global / Scratchpad (0x1F800000) fields read:
 *   Scratchpad +0x2AD (u8) designated/set-piece actor id,
 *   Scratchpad +0x2F4 (u8 *) pointer to ball actor (reads +0x02 s16 x, +0x06 s16 z),
 *   Scratchpad +0x2FF[side] (u8) side tactical mode (1 or 2),
 *   Scratchpad +0x30C[side] (s16) per-side target X reference,
 *   D_800E7000 (u8) match/tactical flag (<4 / <5 checked),
 *   D_800CB654 / D_800CB656 (s16 pairs, stride 4 bytes) special positioning offsets
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   5264 raw = 21.056 m; 1280 raw = 5.12 m; 2304 raw = 9.216 m; -768 raw = -3.072 m;
 *   0x15E2 (5602 raw = 22.408 m), 0x1BE2 (7138 raw = 28.552 m),
 *   0x22E2 (8930 raw = 35.720 m), 0x25E2 (9698 raw = 38.792 m),
 *   384 raw = 1.536 m, 12960 raw = 51.840 m;
 *   Pitch clamps: X in [-0x32C0, +0x32C0] (+-12992 raw = +-51.968 m),
 *   Z in [-0x1F80, +0x2080] (-8064..+8320 raw = -32.256..+33.280 m;
 *   note clamp triggers at |z| >= 0x1F81).
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800E7000;
extern unsigned char D_800CB654[];
extern unsigned char D_800CB656[];

extern void func_8005A85C(unsigned char *actor, signed short *outX, signed short *outZ);
extern int func_8005A76C(int cur, int target, int step);
extern int func_80067F78(unsigned char id1, unsigned char id2);

void func_800A4D30(unsigned char *actor, signed short *outX, signed short *outZ)
{
    signed short tx;
    signed short tz;
    int dirSign;
    unsigned char *ball;
    int ballSign;
    int actorSign;
    int absVal;

    dirSign = -1;
    if (B(actor, 0x399) == 0) {
        dirSign = 1;
    }

    func_8005A85C(actor, &tx, &tz);

    if (func_80067F78(B(Scratchpad, 0x2AD), B(actor, 0x38D)) != 0) {
        switch (B(actor, 0x391)) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            tx = (S(ball, 2) >= 0) ? 0x15E2 : -0x15E2;
            break;

        case 6:
        case 7:
        case 8:
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            tx = (S(ball, 2) >= 0) ? 0x1BE2 : -0x1BE2;
            tz = (signed short)((int)tz + (-(int)tz * 40) / 100);
            break;

        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            tx = (S(ball, 2) >= 0) ? 0x22E2 : -0x22E2;
            tz = (signed short)((int)tz + (-(int)tz) / 2);
            break;

        case 14:
        case 15:
        case 16:
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            tx = (S(ball, 2) >= 0) ? 0x25E2 : -0x25E2;
            tz = (signed short)((int)tz + (-(int)tz * 70) / 100);
            break;

        case 17:
        case 18:
        case 19:
        case 20:
            tx = (signed short)((int)tx - (int)(signed short)dirSign * 384);
            tz = (signed short)((int)tz + (-(int)tz * 70) / 100);
            break;

        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
            tx = (signed short)(-((int)(signed short)dirSign * 12960));
            tz = (signed short)((int)tz + (-(int)tz * 70) / 100);
            break;

        default:
            break;
        }
    } else if (B(actor, 0x3E3) == 0xFD) {
        unsigned char idx = B(actor, 0x3E4);
        signed short valX = S(D_800CB654, (int)idx * 4);
        signed short valZ;

        if (S((unsigned char *)W(Scratchpad, 0x2F4), 2) < 0) {
            valX = -valX;
        }
        tx = valX;

        idx = B(actor, 0x3E4);
        valZ = S(D_800CB656, (int)idx * 4);
        if (S((unsigned char *)W(Scratchpad, 0x2F4), 6) < 0) {
            valZ = -valZ;
        }
        tz = valZ;

        switch (B(actor, 0x3E4)) {
        case 2:
        case 4:
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            tz = (signed short)func_8005A76C((int)tz, (-(int)S(ball, 6)) / 10, 1);
            break;

        case 5:
        case 6: {
            unsigned char side = B(actor, 0x398);
            unsigned char mode = B(Scratchpad + side, 0x2FF);
            if (mode == 1) {
                tx = (signed short)func_8005A76C((int)tx, (int)S(Scratchpad + side * 2, 0x30C), 2);
            } else if (mode == 2) {
                tx = (signed short)func_8005A76C((int)tx, (int)S(Scratchpad + side * 2, 0x30C), 7);
            }
            break;
        }

        case 0:
        case 1:
        case 3:
        default:
            break;
        }
    } else {
        switch (B(actor, 0x391)) {
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
            if (D_800E7000 >= 5) {
                tx = 0;
            }
            break;

        case 14:
        case 15:
        case 16:
            tx = (signed short)((int)(signed short)dirSign * 5264);
            tz = (signed short)((int)tz + (-(int)tz * 40) / 100);
            if (D_800E7000 >= 4) {
                tx = 0;
            }
            break;

        case 19:
        case 20:
            tx = (signed short)((int)(signed short)dirSign * 1280);
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            ballSign = (S(ball, 6) >= 0) ? 1 : -1;
            actorSign = (S(actor, 6) >= 0) ? 1 : -1;
            if (ballSign != actorSign) {
                tx = (signed short)((int)tx + (int)(signed short)dirSign * 2304);
            }
            break;

        case 17:
        case 18:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
            tx = 0;
            if (B(actor, 0x391) == 0x18) {
                tx = (signed short)(-((int)(signed short)dirSign * 768));
            }
            break;

        default:
            ball = (unsigned char *)W(Scratchpad, 0x2F4);
            ballSign = (S(ball, 6) >= 0) ? 1 : -1;
            actorSign = (S(actor, 6) >= 0) ? 1 : -1;
            if (ballSign == actorSign) {
                tz = (signed short)func_8005A76C((int)tz, 0, 4);
            }
            break;
        }
    }

    absVal = (tx >= 0) ? (int)tx : -(int)tx;
    if (absVal >= 0x32C1) {
        tx = (tx <= 0) ? -0x32C0 : 0x32C0;
    }

    absVal = (tz >= 0) ? (int)tz : -(int)tz;
    if (absVal >= 0x1F81) {
        tz = (tz <= 0) ? -0x1F80 : 0x2080;
    }

    *outX = tx;
    *outZ = tz;
}
