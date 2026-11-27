/*
 * func_80084B40
 *
 * Purpose:
 *   Full-time / post-match whistle & celebration phase handler. Transitions all
 *   22 players to state 0x2E (when func_800537D0 returns 0), moves the active
 *   actor, updates penalty/replay timers, triggers referee whistle SFX (0x516,
 *   0x517) at frame thresholds, and exits the phase via func_800171DC() once
 *   the timer passes 100 (or 200) frames.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800FF748 (actor table base, 1000-byte / 0x3E8 stride, 22 actors at +0x3E8)
 *   D_8010A368 / D_8010A369 (u8) cleared if Scratchpad+0x2CE != 0
 *   D_800E81FC (u8) match result code (R)
 *   D_800E6B86 (u16) hold-exit flag (R)
 *   D_800EF838 (u8) cleared to 0 on exit
 *
 * Actor fields read/written:
 *   +0x38D (u8)  actor ID (R)
 *   +0x396 (u8)  state (W: 0x2E)
 *   +0x397 (u8)  substate (W: 0)
 *   +0x3A0 (u32) speed (R; passed +8 to func_8004B0F0)
 *   +0x3AA (u8)  facing angle (R)
 *
 * Scratchpad (0x1F800000) fields read/written:
 *   +0x294 (u16) phase frame counter (R/W, incremented by 1)
 *   +0x2A8 (u8)  active actor ID (R)
 *   +0x2CE (u8)  penalty shootout flag (R)
 *   +0x2DF (u8)  flag checked == 1 when D_800E81FC == 4 (R)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010A368;
extern unsigned char D_8010A369;
extern unsigned char D_800E81FC;
extern unsigned short D_800E6B86;
extern unsigned char D_800EF838;

extern int  func_800537D0(unsigned char *actor);
extern void func_8004B0F0(unsigned char *actor, unsigned char ang, unsigned int speed, int arg3);
extern void func_8004FD5C(void);
extern void func_8004FB68(int arg0);
extern void func_800AB620(unsigned int sfx);
extern void func_8002283C(void);
extern void func_8004A2D0(void);
extern int  func_800525A4(void);
extern void func_800171DC(void);

void func_80084B40(void)
{
    unsigned char *actor = D_800FF748 + 0x3E8;
    unsigned short timer;
    int i;

    for (i = 0; i < 0x16; i++, actor += 0x3E8) {
        if (func_800537D0(actor) == 0) {
            B(actor, 0x396) = 0x2E;
            B(actor, 0x397) = 0;
        }
        if (B(actor, 0x38D) == B(Scratchpad, 0x2A8)) {
            func_8004B0F0(actor, B(actor, 0x3AA), W(actor, 0x3A0) + 8U, 0);
        }
    }

    func_8004FD5C();

    if (B(Scratchpad, 0x2CE) != 0) {
        func_8004FB68(0);
        D_8010A369 = 0;
        D_8010A368 = 0;
    }

    timer = (unsigned short)(H(Scratchpad, 0x294) + 1U);
    H(Scratchpad, 0x294) = timer;

    if (D_800E81FC == 4 && B(Scratchpad, 0x2DF) != 1) {
        if (timer == 1) {
            func_800AB620(0x516);
        }
        if (H(Scratchpad, 0x294) == 0x14) {
            func_800AB620(0x517);
        }
    } else {
        if (timer == 1 || timer == 0x14) {
            func_800AB620(0x516);
        }
        if (H(Scratchpad, 0x294) == 0x28) {
            func_800AB620(0x517);
        }
    }

    func_8002283C();
    func_8004A2D0();

    if (H(Scratchpad, 0x294) >= 0x65) {
        unsigned char res = D_800E81FC;
        if ((res < 2 || res == 2) &&
            func_800525A4() == 0 &&
            H(Scratchpad, 0x294) < 0xC9 &&
            D_800E6B86 != 0) {
            return;
        }
        D_800EF838 = 0;
        func_800171DC();
    }
}
