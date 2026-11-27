/*
 * func_800AE1B8
 *
 * Purpose:
 *   PsyQ-style GPU / graphics subsystem reset (analogous to ResetGraph).
 *   Depending on (mode & 7):
 *     - mode & 7 == 0 or 3: logs format D_80014E60, clears/initializes GPU state
 *       struct D_800CEAC4, resets GPU via func_800B07F4(mode), populates display
 *       geometry from D_800CEB44 / D_800CEB50, and returns the resolved mode byte.
 *     - mode & 7 == 5: skips logging and performs the same D_800CEAC4 setup.
 *     - other modes (1, 2, 4, 6, 7): optionally calls callback D_800CEAC0 if
 *       D_800CEAC6 >= 2, then invokes callback (*(D_800CEABC + 0x34))(1).
 *
 * Arguments:
 *   mode (signed int): reset mode flags
 *
 * Return value:
 *   u8 mode byte (D_800CEAC4[0]) for modes 0/3/5, or callback return value otherwise.
 *
 * Global fields read / written:
 *   D_80014E60, D_80014E80: format strings
 *   D_800CEA7C: version/id argument for func_800AD698
 *   D_800CEABC (u32 / u8 *): GPU/control pointer (+0x34 function pointer)
 *   D_800CEAC0 (void (*)(void *, s32)): error/debug callback
 *   D_800CEAC4 (u8[0x80]): GPU environment/state struct
 *   D_800CEAC6 (u8): verbosity/debug level
 *   D_800CEB44, D_800CEB50 (u16[], stride 4 bytes): default display width/height tables
 */

extern char D_80014E60[];
extern char D_80014E80[];
extern unsigned char D_800CEA7C[];
extern unsigned int D_800CEABC;
extern void (*D_800CEAC0)(void *, signed int);
extern unsigned char D_800CEAC4[];
extern unsigned char D_800CEAC6;
extern unsigned char D_800CEB44[];
extern unsigned char D_800CEB50[];

extern void func_800AD698(void *fmt, ...);
extern void func_800B10D8(void *dst, signed int val, signed int size);
extern void func_800BA738(void);
extern void func_800B1108(unsigned int arg0);
extern unsigned int func_800B07F4(signed int mode);

signed int func_800AE1B8(signed int mode)
{
    unsigned int submode = (unsigned int)mode & 7U;

    if (submode == 0 || submode == 3 || submode == 5) {
        unsigned char *state = D_800CEAC4;
        unsigned char resolved;

        if (submode == 0 || submode == 3) {
            func_800AD698(D_80014E60, D_800CEA7C, D_800CEAC4);
        }

        func_800B10D8(state, 0, 0x80);
        func_800BA738();
        func_800B1108(D_800CEABC & 0xFFFFFFU);
        resolved = (unsigned char)func_800B07F4(mode);
        state[0] = resolved;
        state[1] = 1;
        *(unsigned short *)(state + 4) = *(unsigned short *)(D_800CEB44 + ((unsigned int)resolved << 2));
        *(unsigned short *)(state + 6) = *(unsigned short *)(D_800CEB50 + ((unsigned int)resolved << 2));
        func_800B10D8(state + 0x10, -1, 0x5C);
        func_800B10D8(state + 0x6C, -1, 0x14);
        return (signed int)state[0];
    }

    if (D_800CEAC6 >= 2) {
        D_800CEAC0(D_80014E80, mode);
    }

    return (*(signed int (**)(signed int))(D_800CEABC + 0x34))(1);
}
