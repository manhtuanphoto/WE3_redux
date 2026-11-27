/*
 * func_8008248C
 *
 * Purpose:
 *   Penalty shootout post-kick / resolution tick. Blinks the active penalty
 *   marker sprite (D_800E6D60), reacts to the kick outcome (D_800E7DE9: 1 = goal,
 *   0 = miss/save, 0xFF = aborted/special), puts the GK (+0x2AE) into celebration
 *   or disappointment state (0x36), advances ball physics and camera/HUD, and
 *   after 81 frames (2.7 s) records the final shootout icon and either ends the
 *   shootout (if func_80083D94 != 0) or advances to the next kick (D_8010A400++).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Actor / global fields read/written:
 *   D_800E6D60 (u8) active penalty HUD marker index (0..9, or 0xFF when inactive)
 *   D_800E7DE9 (u8) penalty kick outcome (0, 1, 0xFF)
 *   D_8010A400 (u8) penalty kick counter (incremented when shootout continues)
 *   D_800FF748 (u8[]) actor table (24 * 1000 bytes; GK actor at (Scratchpad+0x2AE) % 24):
 *     +0x38E (u16) current anim id, +0x396 (u8) state (0x82 -> 0x36),
 *     +0x397 (u8) substate (0x80, 0x60, 0x98), +0x3A0 (u32) speed
 *   Ball actor (+0x2F4): +0x3B0 (u8) set to 1
 *   func_8001D004(0x3F) sprite entries (stride 12, +0x0A icon byte: 0x5B / 0x5D)
 *
 * Scratchpad (0x1F800000) fields read/written:
 *   +0x290 (u32) frame counter (bit 0x04 used for marker blink)
 *   +0x294 (u16) phase frame timer (incremented; >= 0x51 ends phase, reset to 0x3C on win)
 *   +0x2AE (u8) GK actor index
 *   +0x2DB / +0x2DC (u8) team 0 / team 1 penalty scores passed to func_80083D94
 *   +0x2F4 (u8 *) pointer to ball actor
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x51 (81 frames = 2.7 s) post-kick hold duration
 *   0x3C (60 frames = 2.0 s) timer reset when shootout is decided
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800E6D60;
extern unsigned char D_800E7DE9;
extern unsigned char D_800FF748[];
extern unsigned char D_8010A400;

extern unsigned char *func_8001D004(int a0);
extern void func_80083CC0(void);
extern unsigned int func_80083D94(unsigned int s0, unsigned int s1);
extern void func_8005BA30(unsigned char *actor, unsigned int anim, unsigned int frame);
extern int func_8001E6B8(int n);
extern void func_800A8B94(unsigned char *ball);
extern void func_800477A0(unsigned char *ball, int mode);
extern void func_8002283C(void);
extern void func_8004A2D0(void);
extern void func_800A8F3C(void);
extern void func_80051810(void);
extern void func_800171DC(void);
extern int func_8004FBC4(int a0);
extern void func_800171FC(int a0);

void func_8008248C(void)
{
    unsigned char outcome;
    unsigned char *gk;
    unsigned short timer;

    if (D_800E6D60 != 0xFF) {
        unsigned char *sprites = func_8001D004(0x3F);
        unsigned char idx = D_800E6D60;
        unsigned char icon = ((W(Scratchpad, 0x290) & 4U) != 0) ? 0x5D : 0x5B;
        B(sprites + idx * 12, 0xA) = icon;
    }

    outcome = D_800E7DE9;
    if (outcome == 1) {
        gk = D_800FF748 + ((unsigned int)B(Scratchpad, 0x2AE) % 24U) * 1000U;
        if (B(gk, 0x396) == 0x82) {
            B(gk, 0x396) = 0x36;
            W(gk, 0x3A0) = 0;
            if ((func_80083D94(B(Scratchpad, 0x2DB), B(Scratchpad, 0x2DC)) & 0xFFU) != 0) {
                func_8005BA30(gk, 0x3C, 0);
                B(gk, 0x397) = 0x80;
            } else {
                if (H(gk, 0x38E) != 1) {
                    func_8005BA30(gk, 1, 0);
                }
                if (func_8001E6B8(2) != 0) {
                    B(gk, 0x397) = 0x60;
                } else {
                    B(gk, 0x397) = 0x98;
                }
            }
        }
        func_800A8B94(*(unsigned char **)(Scratchpad + 0x2F4));
    } else if (outcome == 0) {
        gk = D_800FF748 + ((unsigned int)B(Scratchpad, 0x2AE) % 24U) * 1000U;
        if (B(gk, 0x396) == 0x82) {
            B(gk, 0x396) = 0x36;
            W(gk, 0x3A0) = 0;
            if (H(gk, 0x38E) != 1) {
                func_8005BA30(gk, 1, 0);
            }
            B(gk, 0x397) = 0x98;
        }
        func_800A8B94(*(unsigned char **)(Scratchpad + 0x2F4));
    } else if (outcome == 0xFF) {
        func_80083CC0();
    }

    func_800477A0(*(unsigned char **)(Scratchpad + 0x2F4), 2);
    func_8002283C();
    B(*(unsigned char **)(Scratchpad + 0x2F4), 0x3B0) = 1;
    func_8004A2D0();
    func_800A8F3C();
    func_80051810();

    timer = H(Scratchpad, 0x294) + 1;
    H(Scratchpad, 0x294) = timer;
    if (timer >= 0x51U) {
        unsigned char *sprites = func_8001D004(0x3F);
        unsigned char kick_num = D_8010A400;
        unsigned char slot = (unsigned char)((kick_num & 1U) * 5U + ((unsigned int)kick_num % 10U) / 2U);

        D_800E6D60 = slot;
        B(sprites + slot * 12, 0xA) = 0x5D;
        D_800E6D60 = 0xFF;

        if ((func_80083D94(B(Scratchpad, 0x2DB), B(Scratchpad, 0x2DC)) & 0xFFU) != 0) {
            H(Scratchpad, 0x294) = 0x3C;
            func_800171DC();
        } else if (func_8004FBC4(0) != 0) {
            D_8010A400++;
            func_800171FC(2);
        }
    }
}
