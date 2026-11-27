/*
 * func_80084D44
 *
 * Purpose:
 *   Post-match / celebration / transition sequence state machine driven by
 *   D_800EF838 (states 0..5). Clears actor active flags, sets up end-of-match
 *   or trophy/celebration player positions and animations (state 4), advances
 *   UI/camera fade counters (state 5), and dispatches commentary/audio/mode
 *   transitions via D_800E81FC and Scratchpad+0x2E1.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x198 (u32) cleared in state 4 (W)
 *     +0x19C (u32) set to 2 in state 4 (W)
 *     +0x1E8 (u8)  set to 1 in state 4 (W)
 *     +0x294 (u16) sequence wait timer in states 0, 2, 3, 4, 5 (R/W)
 *     +0x2DF (u8)  index passed (+1) to func_8001723C in state 2 (R)
 *     +0x2E0 (u8)  team index (0 or 1) selecting 6 celebrating/featured players in state 4 (R)
 *     +0x2E1 (u8)  match mode (0, 1, 2, 3, 4 checked) (R)
 *     +0x2E2 (u8)  sub-mode (4 checked for timeout bypass in states 2 & 3) (R)
 *     +0x2F2 (u8)  set to 2 in state 4 (W)
 *     +0x32C (u32) set to 0x400 in state 2 (W)
 *   Actor array D_800FF748 (1000-byte stride, actors 0..23):
 *     +0x000 (u8)  active flag cleared in states 0, 1, 4 (set to 1 for featured 6 in state 4) (W)
 *     +0x002 (s16) x position set in state 4 (W)
 *     +0x006 (s16) z position set in state 4 (W)
 *     +0x026 (s16) visual rotation set in state 4 (W)
 *     +0x3AA (u8)  body heading set in state 4 (W)
 *   Globals:
 *     D_800EF838 (u8)  state machine step (0..5) (R/W)
 *     D_800E81FC (u8)  match result / sub-state (0..5) (R)
 *     D_800E6B86 (u16) hold flag checked before ending state 5 (R)
 *     D_800FF748+0x8F1F (u8)  match flags (bits 0x40, 0x60, 0xCF) (R)
 *     D_800FF748+0x8F22 (u8)  match stage/type (0x0E, 0x1D) (R)
 *     D_800FF748+0x8F23 (u8)  compared against Scratchpad+0x2E1 (== 2) (R)
 *     D_800FF748+0xABA8 (u8)  result/advance flag (R)
 *     D_800FF748+0xABD8, +0xABDA, +0xAC20, +0xAC21 (u8) cleared in states 2 & 3 (W)
 *     D_800FF748+0x5E26, +0x5E27, +0x5E28 (u8) fade/progress counters incremented by 8 in state 5 (R/W)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x65   = 101 frames (~3.37 s) timeout in states 2 & 3 when Scratchpad+0x2E2 == 4
 *   0x259  = 601 frames (~20.03 s) timeout in state 5
 *   -0x1E00 = -7680 raw (-30.72 m) GK z position in state 4
 *   0x500  = 1280 raw (5.12 m) base x offset, 0x120 = 288 raw (1.152 m), 0x100 = 256 raw (1.024 m)
 *   0x2100 = 8448 raw (33.792 m) base z offset, 0x50 = 80 raw (0.32 m), 0x80 = 128 raw (0.512 m)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800EF838;
extern unsigned char D_800E81FC;
extern unsigned short D_800E6B86;
extern unsigned char D_800FF748[];

extern void func_80051810(void);
extern int func_8004FBC4(int);
extern void func_8005891C(void);
extern void func_800860DC(void);
extern int func_8004FB68(int);
extern void func_8002283C(void);
extern int func_800525A4(void);
extern void func_8004FD14(int);
extern void func_8001723C(int);
extern int func_800B53D8(int);
extern void func_8001722C(int);
extern void func_800171C4(int);
extern void func_800171DC(void);
extern void func_800171FC(int);
extern unsigned int func_80086BF8(void);
extern void func_800AB620(int);
extern int func_8001E6B8(int);
extern void func_8005BA30(unsigned char*, int, int);
extern void func_8001CEE0(int, int, int, int, int, int, int, int);
extern void func_8001CDAC(int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int);
extern void func_800B6D78(int);

void func_80084D44(void) {
    int i;
    int s0;
    unsigned char* actor;
    unsigned char mode;
    unsigned char flag;
    unsigned char stage;

    func_80051810();

    switch (D_800EF838) {
    case 0:
        if (func_8004FBC4(0) == 0) {
            return;
        }
        B(D_800FF748, 0) = 0;
        H(Scratchpad, 0x294) = 0;
        D_800EF838 = D_800EF838 + 1;
        return;

    case 1:
        for (i = 23; i >= 0; i--) {
            B(D_800FF748 + (23 - i) * 0x3E8, 0) = 0;
        }
        if (B(Scratchpad, 0x2E1) == 4) {
            D_800EF838 = 0;
            func_800171DC();
            return;
        }
        if (D_800E81FC <= 2) {
            func_8005891C();
        }
        func_800860DC();
        D_800EF838 = D_800EF838 + 1;
        return;

    case 2:
        if (func_8004FB68(0) == 0) {
            return;
        }
        func_8002283C();
        if (func_800525A4() == 0) {
            unsigned short timer;
            if (B(Scratchpad, 0x2E2) != 4) {
                return;
            }
            timer = H(Scratchpad, 0x294);
            H(Scratchpad, 0x294) = timer + 1;
            if (timer < 0x65) { /* 101 frames (~3.37 s) */
                return;
            }
        }
        switch (D_800E81FC) {
        case 0:
        case 1:
        case 2:
            B(D_800FF748, 0xABDA) = 0;
            B(D_800FF748, 0xABD8) = 0;
            H(Scratchpad, 0x294) = 0;
            D_800EF838 = D_800EF838 + 1;
            H(Scratchpad, 0x294) = 0;
            return;
        case 3:
            func_8001723C(4);
            func_800171C4(8);
            H(Scratchpad, 0x294) = 0;
            return;
        case 4:
        case 5:
            func_8004FD14(1);
            func_8001723C(B(Scratchpad, 0x2DF) + 1);
            W(Scratchpad, 0x32C) = 0x400; /* stored in delay slot of func_800B53D8(0x400) */
            func_800B53D8(0x400);
            func_8001722C(2);
            H(Scratchpad, 0x294) = 0;
            return;
        default:
            H(Scratchpad, 0x294) = 0;
            return;
        }

    case 3:
        if ((func_80086BF8() & 0xFF) == 0) {
            if (B(Scratchpad, 0x2E2) != 4) {
                return;
            }
            if (H(Scratchpad, 0x294) < 0x65) { /* 101 frames (~3.37 s) */
                return;
            }
        }
        H(Scratchpad, 0x294) = 0;
        B(D_800FF748, 0xAC21) = 0;
        B(D_800FF748, 0xAC20) = 0;
        func_8004FD14(0);
        mode = B(Scratchpad, 0x2E1);
        s0 = 0;
        if (mode == 0 || (unsigned int)(mode - 3) < 2) {
            D_800EF838 = 0;
            func_800171DC();
            return;
        }
        if (mode == 1) {
            stage = B(D_800FF748, 0x8F22);
            flag = B(D_800FF748, 0x8F1F);
            if ((stage == 0x1D && (flag & 0x40) != 0) ||
                (stage == 0x0E && (flag & 0x40) == 0)) {
                if (B(D_800FF748, 0xABA8) == 1) {
                    s0 = 1;
                }
            }
        }
        mode = B(Scratchpad, 0x2E1);
        if (mode == 2) {
            flag = B(D_800FF748, 0x8F1F);
            if ((flag & 0x40) == 0) {
                if ((flag & 0xCF) == 1 && B(D_800FF748, 0xABA8) == 1) {
                    s0 = 1;
                }
            } else if (D_800E81FC == 1) {
                if ((flag & 0x60) != 0x40 ||
                    (B(D_800FF748, 0x8F23) == 2 && B(D_800FF748, 0xABA8) == 0)) {
                    s0 = 1;
                }
            }
        }
        if (s0 == 0) {
            func_800171FC(4);
            D_800EF838 = 0;
            return;
        }
        func_800AB620(0x11);
        func_800AB620(0x0F);
        D_800EF838 = D_800EF838 + 1;
        return;

    case 4:
        if (func_8004FBC4(0) == 0) {
            return;
        }
        B(Scratchpad, 0x2F2) = 2;
        B(Scratchpad, 0x1E8) = 1;
        actor = D_800FF748 + 0x3E8; /* actor 1 (first player) */
        for (i = 0; i < 22; i++) {
            int startSlot = B(Scratchpad, 0x2E0) * 11;
            if (i >= startSlot && i < startSlot + 6) {
                int slot = i % 11;
                int heading;
                int turn;
                if (slot == 0) {
                    S(actor, 0x02) = 0;
                    S(actor, 0x06) = -0x1E00; /* -7680 raw (-30.72 m) */
                } else {
                    if (slot < 3) {
                        int rx = func_8001E6B8(0x50);
                        S(actor, 0x02) = (signed short)(-rx - slot * 288);
                    } else {
                        int rx = func_8001E6B8(0x50);
                        S(actor, 0x02) = (signed short)(0x500 - ((slot - 3) % 11) * 256 - rx);
                    }
                    {
                        int r1 = func_8001E6B8(0x20);
                        int r2 = func_8001E6B8(0x80);
                        S(actor, 0x06) = (signed short)(r1 * 48 - (r2 + 0x2100));
                    }
                }
                heading = func_8001E6B8(0x20) + 0x30;
                B(actor, 0x3AA) = (unsigned char)heading;
                turn = 0x1C0 - (heading & 0xFF);
                H(actor, 0x26) = (unsigned short)((turn % 256) * 16);

                switch (i % 11) {
                case 0:
                    func_8005BA30(actor, 0x37, 0x1B);
                    break;
                case 1:
                case 4:
                    func_8005BA30(actor, 0x33, 0x0F);
                    break;
                case 2:
                    func_8005BA30(actor, 0x38, 0x0D);
                    break;
                case 3:
                    func_8005BA30(actor, 0x3C, 0x10);
                    break;
                case 5:
                    func_8005BA30(actor, 0x48, 0x00);
                    break;
                }
                B(actor, 0x00) = 1;
            } else {
                B(actor, 0x00) = 0;
            }
            actor += 0x3E8;
        }
        func_8001CEE0(1, 0x9E, 0, 0, -0x10, 7, 0, 1);
        func_8001CDAC(2, 4, 0x60000000, 0, 0, 6, 0, 0x7000, 0x7000, 0, 0, 0, 0, 0, 0, 1);
        H(Scratchpad, 0x294) = 0;
        W(Scratchpad, 0x198) = 0;
        W(Scratchpad, 0x19C) = 2;
        func_800B6D78(0x201);
        /* UNCLEAR: $a0 is not reloaded after func_800B6D78(0x201) before jumping to jal func_800AB620 */
        func_800AB620(0x201);
        D_800EF838 = D_800EF838 + 1;
        return;

    case 5: {
        unsigned char val;
        s0 = 0;
        val = B(D_800FF748, 0x5E26);
        if (val < 10) {
            B(D_800FF748, 0x5E26) = val + 8;
            s0 = 1;
        }
        val = B(D_800FF748, 0x5E27);
        if (val < 35) {
            B(D_800FF748, 0x5E27) = val + 8;
            s0 = 1;
        }
        val = B(D_800FF748, 0x5E28);
        if (val < 55) {
            B(D_800FF748, 0x5E28) = val + 8;
            s0 = 1;
        }
        if (s0 == 0) {
            func_8004FB68(0);
        }
        if (func_800525A4() == 0) {
            unsigned short timer = H(Scratchpad, 0x294);
            H(Scratchpad, 0x294) = timer + 1;
            if (timer < 0x259) { /* 601 frames (~20.03 s) */
                return;
            }
            if (D_800E6B86 != 0) {
                return;
            }
        }
        func_800171FC(4);
        D_800EF838 = 0;
        return;
    }

    default:
        return;
    }
}
