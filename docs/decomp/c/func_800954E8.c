/*
 * func_800954E8
 *
 * Purpose:
 *   Updates/clamps player position (+0x02 X, +0x06 Y) relative to the target
 *   player/entity at Scratchpad+0x2F4 against pitch/box bounds after calling
 *   func_8003DB08. If the player's actor index (+0x38D) does not match the active
 *   index at Scratchpad+0x2A8, delegates to func_800498AC(player) and returns.
 *
 * Arguments:
 *   player (unsigned char *): pointer to player struct
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x2A8 (u8): active actor index
 *     +0x2F4 (unsigned char *): reference/target actor pointer (+0x02 X, +0x06 Y)
 *   player:
 *     +0x002 (s16): X position (R/W)
 *     +0x006 (s16): Y position (R/W)
 *     +0x38D (u8): actor index (R)
 *     +0x399 (u8): side/flag (R)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern void func_8003DB08(void);
extern void func_800498AC(unsigned char *player);
extern unsigned int func_8009568C(void);

void func_800954E8(unsigned char *player)
{
    unsigned char *target;
    signed int tx;
    signed int ty;

    func_8003DB08();

    if (B(player, 0x38D) != B(Scratchpad, 0x2A8)) {
        func_800498AC(player);
        return;
    }

    target = *(unsigned char **)(Scratchpad + 0x2F4);
    tx = S(target, 0x02);

    if ((func_8009568C() & 0xFF) != 0) {
        if (tx >= -0x23E1) {
            H(player, 0x02) = (unsigned short)(H(player, 0x02) + 0x23E2 - tx);
        }
    } else if (B(player, 0x399) == 0) {
        if (tx >= -0x2461) {
            H(player, 0x02) = (unsigned short)(H(player, 0x02) - 0x2462 - tx);
        }
    } else {
        if (tx < 0x2462) {
            H(player, 0x02) = (unsigned short)(H(player, 0x02) + 0x2462 - tx);
        }
    }

    target = *(unsigned char **)(Scratchpad + 0x2F4);
    ty = S(target, 0x06);
    if (ty >= 0x12F5) {
        H(player, 0x06) = (unsigned short)(H(player, 0x06) + 0x12F4 - ty);
    } else if (ty < -0x12F4) {
        H(player, 0x06) = (unsigned short)(H(player, 0x06) - 0x12F4 - ty);
    }

    target = *(unsigned char **)(Scratchpad + 0x2F4);
    tx = S(target, 0x02);
    if (tx >= 0x32E0) {
        H(player, 0x02) = (unsigned short)(H(player, 0x02) + 0x32E0 - tx);
    } else if (tx < -0x32DF) {
        H(player, 0x02) = (unsigned short)(H(player, 0x02) - 0x32E0 - tx);
    }
}
