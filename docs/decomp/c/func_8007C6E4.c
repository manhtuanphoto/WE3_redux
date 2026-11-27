/*
 * func_8007C6E4
 *
 * Purpose:
 *   Per-frame update during the goal/celebration sequence: triggers initial goal
 *   audio/camera setup (< 10 frames), resets eligible players to celebration state
 *   0x32, pairs scorer and assister at frame 22, flashes HUD/banner markers after
 *   frame 41, steps replay/camera/actors, and exits the sequence after 301 frames
 *   (or 501 frames if D_800E6B86 != 0) when func_8004FBC4(0) allows.
 *
 * Arguments:
 *   void
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad+0x290 (u32): global frame counter (R)
 *   Scratchpad+0x294 (u16): sequence frame timer, incremented by 1 (R/W)
 *   Scratchpad+0x2AD (u8): scorer actor ID (R)
 *   Scratchpad+0x2AE (u8): assister actor ID (R)
 *   Scratchpad+0x2AF (u8): goal/event subtype (2 = special marker update) (R)
 *   D_800FF748 + i*1000 (i = 1..22):
 *     +0x396 (u8): set to 0x32 when func_800537D0 != 0 (W)
 *     +0x397 (u8): set to 0 when func_800537D0 != 0 (W)
 *   D_800FF748+0x59D8 (u8), +0x61E2/+0x620A/+0x6232 (u16/s16): HUD/fade timers (R/W)
 *   D_800FF748+0x88A5 + side*22 + slot (u8): player slot lookup byte (R)
 *   D_800E6B86 (u16): extended celebration flag (R)
 *   D_800EF9C8 (u8): set to 4 on exit (W)
 *
 * Magic numbers:
 *   0xA (10 frames = 0.33 s): initial func_8004FB68(0) window.
 *   0x16 (22 frames = 0.73 s): frame to invoke func_80059E7C(scorer, assister).
 *   0x29 (41 frames = 1.37 s): start blinking UI sprite via func_8001D004(0x30).
 *   0x12D (301 frames = 10.03 s), 0x1F5 (501 frames = 16.70 s): sequence end thresholds.
 *   0x32 (50): actor celebration state.
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned short D_800E6B86;
extern unsigned char D_800EF9C8;

extern void func_8004FB68(int);
extern int func_800537D0(unsigned char *actor);
extern void func_80059E7C(unsigned char *scorer, unsigned char *assister);
extern unsigned char *func_8001D004(int);
extern void func_800477A0(unsigned char *actor, int);
extern void func_8002283C(void);
extern void func_8004A2D0(void);
extern void func_80051810(void);
extern void func_8007CC00(void);
extern int func_8004FBC4(int);
extern void func_800171C4(int);

void func_8007C6E4(void) {
    int i;
    unsigned char *actor;
    unsigned char *scorer;
    unsigned short timer;

    if (H(Scratchpad, 0x294) < 10u) {
        func_8004FB68(0);
    }

    actor = D_800FF748 + 1000;
    for (i = 0; i < 22; i++) {
        if (func_800537D0(actor) != 0) {
            B(actor, 0x396) = 0x32;
            B(actor, 0x397) = 0;
        }
        actor += 1000;
    }

    scorer = D_800FF748 + (B(Scratchpad, 0x2AD) % 24u) * 1000u;

    if (H(Scratchpad, 0x294) == 0x16u) {
        unsigned char *assister = D_800FF748 + (B(Scratchpad, 0x2AE) % 24u) * 1000u;
        func_80059E7C(scorer, assister);
    }

    if (H(Scratchpad, 0x294) >= 0x29u) {
        unsigned int side = B(scorer, 0x398);
        unsigned int slot = B(scorer, 0x392);
        unsigned int idx = B(D_800FF748, 0x88A5u + side * 22u + slot);
        unsigned char *ui = func_8001D004(0x30);

        if (B(Scratchpad, 0x2AF) == 2u) {
            if ((W(Scratchpad, 0x290) & 8u) == 0u) {
                B(ui, 0x22 + idx * 12u) = 7;
            } else {
                B(ui, 0x22 + idx * 12u) = 0x5B;
            }
        } else {
            B(ui, 0x2E) = ((W(Scratchpad, 0x290) & 8u) != 0u) ? 0x5B : 0x0B;
        }
    }

    if (S(D_800FF748, 0x6232) > 0) {
        H(D_800FF748, 0x6232) = (unsigned short)(H(D_800FF748, 0x6232) - 2u);
        H(D_800FF748, 0x620A) = (unsigned short)(H(D_800FF748, 0x620A) - 2u);
        H(D_800FF748, 0x61E2) = (unsigned short)(H(D_800FF748, 0x61E2) - 2u);
    }

    func_800477A0(scorer, 5);
    func_8002283C();
    func_8004A2D0();
    func_80051810();
    func_8007CC00();

    timer = (unsigned short)(H(Scratchpad, 0x294) + 1u);
    H(Scratchpad, 0x294) = timer;

    if (timer >= 0x12Du && (D_800E6B86 == 0u || timer >= 0x1F5u)) {
        if (func_8004FBC4(0) != 0) {
            B(D_800FF748, 0x59D8) = 0;
            D_800EF9C8 = 4;
            func_800171C4(0xB);
        }
    }
}
