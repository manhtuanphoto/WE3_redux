/*
 * func_800B8D80 (and unsplit adjacent functions at 0x800B8D8C and D_800B8FF8)
 *
 * Purpose:
 *   0x800B8D80: PS1 BIOS B0:0x3F syscall stub (puts / print string).
 *   0x800B8D8C: Block/sector transfer state-machine step operating on D_800D3C68:
 *     advances reads/writes, checks status/position via func_800B7598, handles
 *     1200-tick (0x4B0) timeouts, tears down channels, and invokes callback D_800D3C64.
 *   D_800B8FF8: Asynchronous completion callback registered via func_800B7450 that
 *     advances D_800D3C68 pointers/counters and triggers teardown + D_800D3C64(2, ...).
 *
 * Arguments:
 *   func_800B8D80: (const char *str) string for BIOS B0:0x3F
 *   func_800B8D8C: (unsigned char status, unsigned int userArg)
 *   D_800B8FF8:    (void)
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   D_800D3C68 (0x38-byte I/O transfer context):
 *     +0x08 (u32): current buffer address (advanced by +0x10 * 4 bytes) (R/W)
 *     +0x10 (u32): transfer size in 32-bit words (0x200 = 512 words = 2048 bytes) (R)
 *     +0x14 (s32): remaining block count, or -1 on error/timeout (R/W)
 *     +0x18 (u32): last timestamp from func_800BA518(-1) (W)
 *     +0x1C (s32): start timestamp for 0x4B0 (1200) tick timeout (R)
 *     +0x20 (u32): completed block index / sector counter (also D_800D3C88) (R/W)
 *     +0x24, +0x28, +0x2C (u32): handles passed to teardown helpers (R)
 *     +0x30 (u32): flags (bit 0 = async/callback mode; also D_800D3C98) (R)
 *     +0x34 (u32): saved userArg passed to D_800D3C64 (R/W)
 *   D_800D3C64: completion callback fn(int resultCode, unsigned int userArg)
 *
 * Magic numbers:
 *   0xB0 / 0x3F: PS1 BIOS B0-table vector 0x3F.
 *   0x200 (512): word count triggering header/sector position verification.
 *   0x4B0 (1200): timeout delta against func_800BA518(-1).
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800D3C68[];
extern void (*D_800D3C64)(int, unsigned int);
extern char D_800153DC[];
extern void func_800B700C(unsigned int);
extern void func_800B7020(unsigned int);
extern void func_800B7170(unsigned int, unsigned int);
extern void func_800B7410(unsigned int, unsigned int);
extern void func_800B7430(unsigned int, unsigned int);
extern void func_800B7450(unsigned int);
extern void func_800B7474(unsigned int);
extern unsigned int func_800B7598(void *);
extern void func_800B90C4(int);
extern signed int func_800BA518(int);

void D_800B8FF8(void);

/* 0x800B8D80: PS1 BIOS B0:0x3F syscall stub */
void func_800B8D80(const char *str) {
    /* UNCLEAR: direct PS1 BIOS jump to 0x000000B0 with $t1 = 0x3F (puts). */
    ((void (*)(const char *))0x000000B0)(str);
}

/* 0x800B8D8C: transfer state-machine handler (unsplit inside func_800B8D80.s) */
void func_800B8D8C(unsigned char status, unsigned int userArg) {
    unsigned char buf16[16];
    signed int now;

    W(D_800D3C68, 0x34) = userArg;
    if (status == 1) {
        if (SW(D_800D3C68, 0x14) > 0) {
            if (W(D_800D3C68, 0x10) == 0x200) {
                if (W(D_800D3C68, 0x30) & 1u) {
                    func_800B7450(0);
                    func_800B7430((unsigned int)buf16, 3);
                    func_800B7474(0);
                    func_800B7450((unsigned int)D_800B8FF8);
                } else {
                    func_800B7410((unsigned int)buf16, 3);
                }
                if (func_800B7598(buf16) != W(D_800D3C68, 0x20)) {
                    func_800B8D80(D_800153DC);
                    SW(D_800D3C68, 0x14) = -1;
                }
            }
            if (W(D_800D3C68, 0x30) & 1u) {
                func_800B7430(W(D_800D3C68, 0x08), W(D_800D3C68, 0x10));
            } else {
                func_800B7410(W(D_800D3C68, 0x08), W(D_800D3C68, 0x10));
                W(D_800D3C68, 0x08) += W(D_800D3C68, 0x10) << 2;
                SW(D_800D3C68, 0x14) -= 1;
                W(D_800D3C68, 0x20) += 1;
            }
        }
    } else {
        SW(D_800D3C68, 0x14) = -1;
    }

    W(D_800D3C68, 0x18) = (unsigned int)func_800BA518(-1);
    if (SW(D_800D3C68, 0x14) < 0) {
        func_800B90C4(1);
    }

    now = func_800BA518(-1);
    if (SW(D_800D3C68, 0x1C) + 0x4B0 < now) {
        SW(D_800D3C68, 0x14) = -1;
    }

    if (SW(D_800D3C68, 0x14) == 0 || (SW(D_800D3C68, 0x1C) + 0x4B0 < func_800BA518(-1))) {
        func_800B700C(W(D_800D3C68, 0x24));
        func_800B7020(W(D_800D3C68, 0x28));
        if (W(D_800D3C68, 0x30) & 1u) {
            func_800B7450(W(D_800D3C68, 0x2C));
        }
        func_800B7170(9, 0);
        if (D_800D3C64 != 0) {
            D_800D3C64(SW(D_800D3C68, 0x14) != 0 ? 5 : 2, userArg);
        }
    }
}

/* 0x800B8FF8: completion callback registered via func_800B7450 */
void D_800B8FF8(void) {
    W(D_800D3C68, 0x08) += W(D_800D3C68, 0x10) << 2;
    SW(D_800D3C68, 0x14) -= 1;
    W(D_800D3C68, 0x20) += 1;

    if (SW(D_800D3C68, 0x14) == 0) {
        func_800B700C(W(D_800D3C68, 0x24));
        func_800B7020(W(D_800D3C68, 0x28));
        if (W(D_800D3C68, 0x30) & 1u) {
            func_800B7450(W(D_800D3C68, 0x2C));
        }
        func_800B7170(9, 0);
        if (D_800D3C64 != 0) {
            D_800D3C64(2, W(D_800D3C68, 0x34));
        }
    }
}
