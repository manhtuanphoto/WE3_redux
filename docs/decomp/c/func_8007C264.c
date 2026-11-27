/*
 * func_8007C264
 *
 * Purpose:
 *   Handles a foul / card / stoppage event for the offending actor indexed by
 *   Scratchpad+0x2AD (modulo 24). Increments per-team event counters, and either
 *   triggers penalty/free-kick commentary & call when Scratchpad+0x2AF == 1
 *   inside the penalty-box region, or scatters nearby players away from the
 *   offender, turns the offender toward the fouled actor (Scratchpad+0x2AE),
 *   spawns an effect/marker at 2.56 m in front, calls func_8005C8D0, and
 *   updates card/foul counters and Scratchpad+0x2E7/0x2F0.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Actor / global fields read:
 *   D_800FF748 (24 x 1000-byte actor array; players at indices 1..22):
 *     +0x002 (s16) x, +0x006 (s16) z, +0x38D (u8) actor id,
 *     +0x392 (u8) team slot (0..10), +0x398 (u8) side (0/1),
 *     +0x399 (u8) attack direction flag, +0x3AA (u8) body heading
 *   D_800FF748 + 0x88A5 + side*22 + team_slot (u8) per-player counter/card value
 *   D_800FF748 + 0x8ECC + side (u8) per-side event counter
 *   D_800FF748 + 0x8ECE / 0x8ED0 + side (u8) per-side card/foul counters
 *
 * Actor / global fields written:
 *   Nearby actors (indices 1..22): +0x002 (s16) x, +0x006 (s16) z
 *   Offending actor: +0x026 (s16) rotation, +0x3AA (u8) body heading
 *   D_800FF748 + 0x59D8 (u8) cleared to 0
 *   D_800EF9C8 (u8) set to 4
 *   D_800FF748 + 0x88A5 / 0x8ECC / 0x8ECE / 0x8ED0 counters incremented
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x2AD (u8) offending actor id (indexed % 24)
 *   +0x2AE (u8) fouled/target actor id (indexed % 24)
 *   +0x2AF (u8) event sub-type (1 = penalty/box check, 2 = yellow/sub-event, else other)
 *   +0x2BC (s16) event x position
 *   +0x2BE (s16) event z position
 *   +0x2F0 (u8) counter incremented
 *
 * Scratchpad fields written:
 *   +0x294 (u16) cleared to 0
 *   +0x2E7 (u8) set to offender body heading + 0x80 + jitter(+-0x30)
 *   +0x2F0 (u8) incremented by 1
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x12F4 (4852) = 19.408 m (abs(z) threshold)
 *   0x23E2 (9186) = 36.744 m (x threshold along attack direction)
 *   0x23FFFF -> distSq <= 2359295, sqrt = 1536 raw = 6.144 m scatter check radius
 *   0x600 (1536) = 6.144 m scatter placement radius
 *   0x280 (640) = 2.56 m forward offset for func_8001D14C
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_800EF9C8;

extern void func_800171C4(int arg0);
extern void func_800171DC(void);
extern void func_8001D14C(int a0, int a1, int a2, int x, int z, int heading, int s1, int s2, int s3, int flag);
extern int func_8001E5A4(int angle, int scale);
extern int func_8001E694(int angle, int scale);
extern int func_8001E6B8(int n);
extern int func_800597C8(int arg0);
extern int func_8005B36C(int x1, int z1, int x2, int z2);
extern int func_8005C7A0(int n);
extern void func_8005C8D0(unsigned char *actor, int val, int id);

void func_8007C264(void)
{
    unsigned char id2AD;
    unsigned char *offender;
    unsigned char *other;
    unsigned char *target;
    int i;
    int absZ;
    int eventX;
    int dx, dz, distSq;
    int randAngle;
    int angle;
    int turn;
    int offX, offZ;
    int slotOffset;
    unsigned char argVal;
    int jitter;

    id2AD = B(Scratchpad, 0x2AD);
    H(Scratchpad, 0x294) = 0;

    offender = D_800FF748 + ((id2AD % 24) & 0xFF) * 1000;
    B(D_800FF748, 0x8ECC + B(offender, 0x398)) += 1;

    if (B(Scratchpad, 0x2AF) == 1) {
        func_800171C4(2);
        absZ = S(Scratchpad, 0x2BE);
        if (absZ < 0) {
            absZ = -absZ;
        }
        if (absZ < 0x12F4) {
            eventX = S(Scratchpad, 0x2BC);
            if (B(offender, 0x399) != 0) {
                if (-eventX < -0x23E2) {
                    B(D_800FF748, 0x59D8) = 0;
                    D_800EF9C8 = 4;
                    func_800171C4(0xB);
                    return;
                }
            } else {
                if (eventX < -0x23E2) {
                    B(D_800FF748, 0x59D8) = 0;
                    D_800EF9C8 = 4;
                    func_800171C4(0xB);
                    return;
                }
            }
        }
        func_800597C8(0);
        return;
    }

    other = D_800FF748 + 0x3E8;
    for (i = 0; i < 22; i++, other += 1000) {
        unsigned char otherId = B(other, 0x38D);
        if (otherId == B(Scratchpad, 0x2AD) || otherId == B(Scratchpad, 0x2AE)) {
            continue;
        }
        dx = S(offender, 2) - S(other, 2);
        dz = S(offender, 6) - S(other, 6);
        distSq = dx * dx + dz * dz;
        if (distSq <= 0x23FFFF) {
            randAngle = func_8001E6B8(0x100) & 0xFF;
            S(other, 2) = (signed short)(H(offender, 2) + func_8001E694(randAngle, 0x600));
            S(other, 6) = (signed short)(H(offender, 6) + func_8001E5A4(randAngle, 0x600));
        }
    }

    target = D_800FF748 + ((B(Scratchpad, 0x2AE) % 24) & 0xFF) * 1000;
    angle = func_8005B36C(S(offender, 2), S(offender, 6), S(target, 2), S(target, 6));
    B(offender, 0x3AA) = (unsigned char)angle;
    turn = 0x1C0 - (angle & 0xFF);
    S(offender, 0x26) = (signed short)((turn % 256) << 4);

    offX = (signed short)(H(offender, 2) + func_8001E694(B(offender, 0x3AA), 0x280));
    offZ = (signed short)(H(offender, 6) + func_8001E5A4(B(offender, 0x3AA), 0x280));
    func_8001D14C(0x17, 0x41, 0x35, offX, offZ, (B(offender, 0x3AA) + 0x80) & 0xFF, 0x1000, 0x1000, 0x1000, 1);

    slotOffset = B(offender, 0x398) * 22 + B(offender, 0x392);
    argVal = B(D_800FF748, 0x88A5 + slotOffset);
    func_8005C8D0(D_800FF748 + ((B(Scratchpad, 0x2AD) % 24) & 0xFF) * 1000, argVal, 0x1A);

    jitter = func_8005C7A0(0x30) + 0x80;
    B(Scratchpad, 0x2E7) = (unsigned char)(B(offender, 0x3AA) + jitter);
    B(Scratchpad, 0x2F0) = B(Scratchpad, 0x2F0) + 1;

    if (B(Scratchpad, 0x2AF) == 2) {
        slotOffset = B(offender, 0x398) * 22 + B(offender, 0x392);
        B(D_800FF748, 0x88A5 + slotOffset) += 1;
        B(D_800FF748, 0x8ED0 + B(offender, 0x398)) += 1;
    } else {
        slotOffset = B(offender, 0x398) * 22 + B(offender, 0x392);
        B(D_800FF748, 0x88A5 + slotOffset) += 4;
        B(D_800FF748, 0x8ECE + B(offender, 0x398)) += 1;
    }
    func_800171DC();
}
