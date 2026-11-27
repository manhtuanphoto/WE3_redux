/*
 * func_8007B41C
 *
 * Purpose:
 *   Initializes a corner kick set piece. Resets all 22 player actors into state
 *   0x2F / substate 0 (except active flagged players when Scratchpad+0x2E1 == 5),
 *   assigns possession flags to the corner taker's team, selects the taker's
 *   preferred foot and facing angle, places the ball at the appropriate corner
 *   flag (+-0x3280, -0x20, +-0x20C0), positions the taker and formation, and
 *   triggers corner kick UI/audio setup.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   void
 *
 * Actor fields read/written (players 0..21 at D_800FF748 + (i+1)*1000):
 *   +0x392 (u8) team slot, +0x396 (u8) state (set to 0x2F),
 *   +0x397 (u8) substate (set to 0, or 1 for taker),
 *   +0x398 (u8) side, +0x399 (u8) attack direction flag,
 *   +0x39A (u8) active/skip flag, +0x3AA (u8) body heading (taker),
 *   +0x3AB (u8) target heading (taker), +0x3AC (u8) mirror/foot (taker)
 *
 * Global fields written:
 *   D_800FF748[0x88A4] = 2;
 *   D_800FF748[0x6544] = D_800FF748[0x6454] = D_800FF748[0x647C] = 0 (when +0x2E1 == 5)
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x2AD (u8) corner-taker actor id,
 *   +0x2BC (s16) out-of-bounds x side (<0 left, >=0 right),
 *   +0x2BE (s16) out-of-bounds z side (>0 top/bottom),
 *   +0x2E1 (u8) mode, +0x2F4 (u32 -> u8 *) pointer to ball actor,
 *   +0x334 (u16) active flag triggering func_800729CC
 *
 * Scratchpad fields written:
 *   +0x1EC (u8) = 0, +0x294 (u16) = 0, +0x296 (u16) = 0,
 *   +0x2A8 (u8) = 0, +0x2B8 (u16) = 0x7FFF, +0x2CF[side] possession flags,
 *   ball+0x02/04/06 position, ball+0x3A4 heading, ball+0x396 = 0
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x3280 (12928 raw = 51.712 m) - corner x coordinate
 *   -0x20 (-32 raw = 0.128 m above ground) - ball y height
 *   0x20C0 (8384 raw = 33.536 m) - corner z coordinate
 *   0x2BF0 (11248 raw = 44.992 m) - target aim x toward goal line
 *   0x48 / 0x38 (72 / 56 angle units; -0x80 when z > 0) - taker initial body heading
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned int D_8010C330[];

extern void func_800729CC(void);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern void func_8005BA30(unsigned char *actor, int anim, int frame);
extern void func_80078CE4(unsigned char *actor, unsigned int heading);
extern void func_800A5810(void);
extern void func_80096050(void);
extern void func_80051810(void);
extern void func_80098428(void);
extern void func_80089BD8(void);
extern void func_800204D8(void);
extern void func_8004ED68(int side);
extern void func_8005980C(unsigned char *actor);
extern void func_800171DC(void);
extern void func_8001D470(int, int, int, int, int, int, int, int, int);

void func_8007B41C(void)
{
    int i;
    unsigned char *taker;
    unsigned char *ball;
    int heading;
    unsigned int foot;
    int target_x;
    unsigned int angle;

    if (H(Scratchpad, 0x334) != 0) {
        func_800729CC();
    }

    for (i = 0; i < 0x16; i++) {
        unsigned char *p = D_800FF748 + (i + 1) * 1000;
        if (B(Scratchpad, 0x2E1) != 5 || B(p, 0x39A) == 0) {
            B(p, 0x396) = 0x2F;
            B(p, 0x397) = 0;
        }
    }

    B(Scratchpad, 0x2A8) = 0;
    taker = D_800FF748 + ((unsigned int)B(Scratchpad, 0x2AD) % 24) * 1000;

    B(Scratchpad + B(taker, 0x398), 0x2CF) = 0;
    B(Scratchpad + (B(taker, 0x398) ^ 1), 0x2CF) = 1;

    {
        unsigned int off = B(taker, 0x392) * 12 + B(taker, 0x398) * 264;
        foot = (W((unsigned char *)D_8010C330, off) >> 4) & 1;
        B(taker, 0x3AC) = (unsigned char)foot;
        heading = (foot != 0) ? 0x48 : 0x38;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    if (S(Scratchpad, 0x2BC) < 0) {
        S(ball, 2) = -0x3280;
    } else {
        S(ball, 2) = 0x3280;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    S(ball, 4) = -0x20;

    if (S(Scratchpad, 0x2BE) > 0) {
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        S(ball, 6) = 0x20C0;
        heading -= 0x80;
    } else {
        ball = (unsigned char *)W(Scratchpad, 0x2F4);
        S(ball, 6) = -0x20C0;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    target_x = (B(taker, 0x399) != 0) ? -0x2BF0 : 0x2BF0;
    angle = func_8005B36C(S(ball, 2), S(ball, 6), target_x, 0);

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    B(ball, 0x3A4) = (unsigned char)angle;

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    B(taker, 0x3AB) = B(ball, 0x3A4);
    B(ball, 0x396) = 0;

    func_8005BA30(taker, 0x2F, 0);
    B(taker, 0x397) = 1;
    B(taker, 0x3AA) = (unsigned char)heading;
    func_80078CE4(taker, heading & 0xFF);

    func_800A5810();
    func_80096050();
    func_80051810();
    func_80098428();
    func_80089BD8();

    B(Scratchpad, 0x1EC) = 0;
    H(Scratchpad, 0x2B8) = 0x7FFF;
    H(Scratchpad, 0x296) = 0;
    B(D_800FF748, 0x88A4) = 2;

    func_800204D8();
    func_8004ED68(0);
    func_8004ED68(1);
    H(Scratchpad, 0x294) = 0;
    func_8005980C(taker);
    func_800171DC();

    if (B(Scratchpad, 0x2E1) == 5) {
        func_8001D470(0x11, 9, 0, 0, 0x40, 0x1000, 0x1000, 0x1000, 0);
        B(D_800FF748, 0x6544) = 0;
        B(D_800FF748, 0x6454) = 0;
        B(D_800FF748, 0x647C) = 0;
    }
}
