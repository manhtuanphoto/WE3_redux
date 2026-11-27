/*
 * func_80070008
 *
 * Purpose:
 *   Dispatches overlay/pause/menu sub-states via D_8010A320 (0..6). When the
 *   sub-state finishes (non-zero return), clears active UI entry flags, and if
 *   in match mode (Scratchpad+0x2E1 == 5) with D_8010A5DE != 0, restores match
 *   actors and transitions back into match phase (kickoff/set-piece/replay/etc.)
 *   according to D_800FF748+0xABDE (0..2).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   u8 (unsigned int masked to 0xFF) - completion flag from the dispatched sub-state (0 if still active)
 *
 * Global / actor fields read/written:
 *   D_8010A320 (u8) sub-mode selector (0..6)
 *   D_8010A5DE (u8) resume-to-match flag
 *   D_8010A5A8 + 0x0A (u8) option/HUD flags (bits 0x04 and 0x20)
 *   D_800FF748 + 0x5DC0.. (26 entries, stride 0x28): +0x04 (u8) cleared to 0
 *   D_800FF748 + 0x3E8.. (22 actors, stride 0x3E8): +0x00 (u8) set to 1 when match phase == 0xD
 *   D_800FF748 + 0x64CC / +0x64F4 / +0x656C (u8) HUD visibility flags set from (D_8010A5A8[0xA] & 4) >> 2
 *   D_800FF748 + 0xABDE (u8) resume target type (0, 1, 2)
 *   D_800FF748 + 0xABDF (u8) resume side/variant flags (bits 0x01, >> 1)
 *   func_8001D004(0) sprite table: +0x26, +0x28, +0x32, +0x34 (u8) UI coords
 *
 * Scratchpad (0x1F800000) fields read/written:
 *   +0x29A (u8) match phase (checked against 2, 4, 0xD)
 *   +0x2AD (u8) set-piece actor id written from func_8007BA04 in case 2
 *   +0x2BC (s16) restart x written to 0x3320 in case 2
 *   +0x2BE (s16) restart z written to +-0x2120 in case 2
 *   +0x2C8 (u32) cleared to 0 in case 2
 *   +0x2E1 (u8) game mode (checked == 5)
 *   +0x2F2 (u8) cleared to 0 in case 1
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x3320 (13088 raw = 52.352 m) goal line x
 *   +-0x2120 (+-8480 raw = +-33.92 m) touchline / corner z
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_8010A320;
extern unsigned char D_8010A5DE;
extern unsigned char D_8010A5A8[];
extern unsigned char D_800FF748[];

extern unsigned int func_80070348(void);
extern unsigned int func_80072098(int a0, int a1);
extern void func_8011D82C(void);
extern void func_80138018(void);
extern void func_80050738(unsigned char *ptr);
extern void func_8004FAC0(int a0, int a1);
extern void func_8001722C(int a0);
extern void func_800171C4(int a0);
extern unsigned char *func_8001D004(int a0);
extern unsigned int func_8007BA04(unsigned char *actor_base);
extern void func_800AB620(int a0);

unsigned int func_80070008(void)
{
    unsigned char result = 0;
    unsigned char *slot;
    int i;

    switch (D_8010A320) {
    case 0:
        result = (unsigned char)func_80070348();
        break;
    case 1:
        break;
    case 2:
        result = (unsigned char)func_80072098(5, 5);
        break;
    case 3:
        func_8011D82C();
        break;
    case 4:
        func_80138018();
        break;
    case 5:
    case 6:
        result = (unsigned char)func_80072098(5, 3);
        break;
    }

    if (result == 0) {
        return result;
    }

    func_80050738(D_800FF748 + 0x5DC0);
    slot = D_800FF748 + 0x5DC0;
    for (i = 0; i < 0x1A; i++) {
        B(slot, 4) = 0;
        slot += 0x28;
    }

    if (B(Scratchpad, 0x2E1) != 5) {
        return result;
    }

    if (D_8010A5DE != 0) {
        unsigned char mode;

        if (B(Scratchpad, 0x29A) == 0xD) {
            unsigned char *actor = D_800FF748 + 0x3E8;
            for (i = 0; i < 0x16; i++) {
                B(actor, 0) = 1;
                actor += 0x3E8;
            }
        }

        mode = B(D_800FF748, 0xABDE);
        if (mode == 0) {
            if (B(Scratchpad, 0x29A) == 0xD) {
                func_8004FAC0(0, 0);
                func_8001722C(1);
                func_800171C4(0);
            }
        } else if (mode == 1) {
            if ((B(D_800FF748, 0xABDF) & 1U) != 0) {
                if (B(Scratchpad, 0x29A) != 2) {
                    B(Scratchpad, 0x2F2) = 0;
                    func_8004FAC0(0, 0);
                    func_800171C4(2);
                }
            } else {
                unsigned char phase = B(Scratchpad, 0x29A);
                if (phase != 2 && phase != 0xD) {
                    B(Scratchpad, 0x2F2) = 0;
                    func_8004FAC0(0, 0);
                    func_800171C4(0xD);
                }
            }
        } else if (mode == 2) {
            if (B(Scratchpad, 0x29A) != 4) {
                unsigned char *hud;
                unsigned char flag_bit;

                S(Scratchpad, 0x2BC) = 0x3320;
                S(Scratchpad, 0x2BE) = (B(D_800FF748, 0xABDF) >> 1) ? 0x2120 : -0x2120;
                W(Scratchpad, 0x2C8) = 0;
                func_8004FAC0(0, 0);

                flag_bit = (B(D_8010A5A8, 0xA) & 4U) >> 2;
                B(D_800FF748, 0x64CC) = flag_bit;
                B(D_800FF748, 0x656C) = flag_bit;
                B(D_800FF748, 0x64F4) = flag_bit;

                hud = func_8001D004(0);
                if ((B(D_8010A5A8, 0xA) & 0x20U) != 0) {
                    B(hud, 0x28) = 0xD0;
                    B(hud, 0x26) = 0x10;
                    B(hud, 0x34) = 0xE0;
                    B(hud, 0x32) = 0x10;
                } else {
                    B(hud, 0x28) = 0xC8;
                    B(hud, 0x26) = 0x08;
                    B(hud, 0x34) = 0xC8;
                    B(hud, 0x32) = 0x08;
                }
                func_800171C4(4);
                B(Scratchpad, 0x2AD) = (unsigned char)func_8007BA04(D_800FF748 + 0x3E8);
            }
        }

        func_800AB620(0xD);
    }

    return result;
}
