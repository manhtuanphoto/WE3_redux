/*
 * func_800A5340
 *
 * Purpose:
 *   Selects the corner-kick taker from priority slot tables D_800CB670 /
 *   D_800CB680 (14 slots) and positions them at the corner flag (+/-10722,
 *   +/-6644 = +/-0x29E2, +/-0x19F4), recording their actor ID in
 *   Scratchpad+0x2AE.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800FF748 (actor table base, 1000-byte stride)
 *   D_800FFAE1 (D_800FF748 + 0x399: actor team side byte)
 *   D_800CB670 / D_800CB680 (u8[14]) priority formation-slot tables
 *
 * Actor fields read:
 *   +0x000 (u8) active flag
 *   +0x38D (u8) actor ID
 *   +0x391 (u8) formation / role slot
 *
 * Scratchpad (0x1F800000) fields read/written:
 *   +0x2AD (u8)  excluded/primary actor ID
 *   +0x2AE (u8)  selected corner-kick taker actor ID (W)
 *   +0x2F4 (u32) ball pointer (reads +0x06 Z coordinate sign)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_800FFAE1[];
extern unsigned char D_800CB670[];
extern unsigned char D_800CB680[];

extern unsigned int func_8005CE14(unsigned char actor_id);
extern void func_8009F554(unsigned char *actor, signed short x, signed short z);
extern void func_8005BA30(unsigned char *actor, unsigned int anim, unsigned int param2);

void func_800A5340(void)
{
    unsigned int mod_id = (unsigned int)B(Scratchpad, 0x2AD) % 24U;
    signed short x_sign = -1;
    signed short z_sign;
    unsigned char *slot_table;
    unsigned char *ball;
    unsigned int slot_idx;

    if (D_800FFAE1[mod_id * 1000U] == 0) {
        x_sign = 1;
    }

    ball = (unsigned char *)W(Scratchpad, 0x2F4);
    z_sign = (S(ball, 6) >= 0) ? 1 : -1;

    slot_table = (x_sign == z_sign) ? D_800CB670 : D_800CB680;

    B(Scratchpad, 0x2AE) = 0;

    for (slot_idx = 0; slot_idx < 0xE; slot_idx++) {
        unsigned int team_idx = func_8005CE14(B(Scratchpad, 0x2AD)) & 0xFFU;
        unsigned char *actor = D_800FF748 + 0x3E8 + team_idx * 1000U;
        unsigned int p;

        for (p = 0; p < 0xA; p++, actor += 1000) {
            if (B(actor, 0x38D) != B(Scratchpad, 0x2AD) &&
                B(actor, 0) != 0 &&
                B(actor, 0x391) == slot_table[slot_idx]) {
                func_8009F554(actor,
                              (signed short)(x_sign * 0x29E2),
                              (signed short)(z_sign * 0x19F4));
                func_8005BA30(actor, 1, 0);
                B(Scratchpad, 0x2AE) = B(actor, 0x38D);
                break;
            }
        }

        if (B(Scratchpad, 0x2AE) != 0) {
            break;
        }
    }
}
