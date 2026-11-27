/*
 * func_80015C84
 *
 * Purpose:
 *   Top-level match/menu frame dispatcher. When Scratchpad+0x2A0 is 0, dispatches
 *   by game state (Scratchpad+0x298, cases 0..6). When pause/overlay is active (+0x2A0 != 0),
 *   updates match replay/HUD state and processes resume/exit actions from func_80070008().
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_8010A320 (u8), D_800FF748 (actor table base, 1000-byte stride)
 *
 * Global fields written:
 *   D_8010C2A2 (u8) = 0x10 on exit action 1
 *   D_800FF748 + 0x6404 (u8) = 0 on exit action 2
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x1F0..+0x20F (32 bytes) copied to +0x210..+0x22F when +0x29A == 1
 *   +0x298 (u8) game state (0..6; 5 = match)
 *   +0x29A (u8) match phase
 *   +0x2A0 (u8) pause/overlay active flag
 *   +0x2A8 (u8) actor index (mod 24 indexes D_800FF748)
 *   +0x2CE (u8) pause/replay guard flag
 *   +0x334 (u16) checked on exit action 2
 *
 * Scratchpad fields written:
 *   +0x1EC (u8) = 0
 *   +0x210..+0x22F (32 bytes)
 *   +0x2A0 (u8) = 0, +0x2A1 (u8) = 0
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010A320;
extern unsigned char D_8010C2A2;

extern void func_80015F28(void);
extern void func_80015F80(void);
extern void func_80016108(void);
extern void func_8001617C(void);
extern void func_80016F44(void);
extern void func_8001E778(void);
extern void func_80016F88(void);
extern void func_80096094(unsigned char *actor, unsigned int param);
extern void func_80051810(void);
extern void func_80020C14(void);
extern unsigned int func_80070008(void);
extern void func_800AB620(unsigned int sfx);
extern void func_8004FAC0(unsigned int a0, unsigned int a1);
extern void func_80017178(unsigned int a0);
extern void func_8005ADE4(void);
extern void func_80050510(unsigned int a0);
extern void func_8005A100(void);

void func_80015C84(void)
{
    unsigned int action;

    if (B(Scratchpad, 0x2A0) == 0) {
        switch (B(Scratchpad, 0x298)) {
        case 0:
            func_80015F28();
            break;
        case 1:
            func_80015F80();
            break;
        case 2:
            func_80016108();
            break;
        case 3:
            func_8001617C();
            break;
        case 4:
            func_80016F44();
            break;
        case 5:
            func_8001E778();
            break;
        case 6:
            func_80016F88();
            break;
        }
        return;
    }

    if (B(Scratchpad, 0x298) == 5 && D_8010A320 != 3 && B(Scratchpad, 0x2CE) == 0) {
        unsigned char phase = B(Scratchpad, 0x29A);
        B(Scratchpad, 0x1EC) = 0;
        if (phase == 0) {
            unsigned int actorIdx = (unsigned int)(B(Scratchpad, 0x2A8) % 24U) & 0xFF;
            func_80096094(D_800FF748 + actorIdx * 1000, 0);
        } else if (phase < 2) {
            int i;
            func_80051810();
            for (i = 0; i < 32; i += 4) {
                W(Scratchpad, 0x210 + i) = W(Scratchpad, 0x1F0 + i);
            }
        }
        func_80020C14();
    }

    action = func_80070008();
    if (action == 1) {
        B(Scratchpad, 0x2A0) = 0;
        func_800AB620(2);
        B(Scratchpad, 0x2A1) = 0;
        func_8004FAC0(0x10, 0);
        func_80017178(3);
        D_8010C2A2 = 0x10;
        func_800AB620(0x11);
        func_800AB620(0xF);
        func_800AB620(0x528);
    } else if (action == 2) {
        B(Scratchpad, 0x2A0) = 0;
        func_800AB620(2);
        B(D_800FF748, 0x6404) = 0;
        func_8005ADE4();
        func_80050510(0);
        if (H(Scratchpad, 0x334) != 0) {
            func_8005A100();
        }
    }
}
