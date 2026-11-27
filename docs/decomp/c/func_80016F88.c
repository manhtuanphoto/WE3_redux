/*
 * func_80016F88
 *
 * Purpose:
 *   Top-level state dispatcher on Scratchpad+0x29A (states 0..5 via
 *   jtbl_80010088), coordinating match/scene transitions, audio/VRAM setup,
 *   and Scratchpad+0x198/+0x19C/+0x294 state updates.
 *
 * Arguments:
 *   None
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x198 (u32) cleared to 0 in case 2 (W)
 *     +0x19C (u32) set to 1 in case 2, 2 in case 4 (W)
 *     +0x294 (u16) cleared to 0 in case 2 (W)
 *     +0x29A (u8)  dispatcher state index (0..5) (R)
 *     +0x2E1 (u8)  sub-state checked in case 1 (R)
 *     +0x2E4 (u8)  cleared to 0 in case 0 (W)
 *   D_800FF748:
 *     +0x8F15 (u8) cleared to 0 in case 0 (W)
 *     +0xABA8 (u8) checked in case 1 (R)
 *   D_801A6000: (u32) pointer/handle passed to func_80019564 in case 3 (R)
 */

#define B(p,o) (*(unsigned char*)((p)+(o)))
#define H(p,o) (*(unsigned short*)((p)+(o)))
#define W(p,o) (*(unsigned int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned int D_801A6000;

extern void func_800171A0(void);
extern void func_80017564(int arg);
extern unsigned char func_800189F0(void);
extern void func_80019564(unsigned int arg0, int arg1);
extern void func_800198E8(int arg);
extern void func_8004FE30(void);
extern void func_8004FEE8(int arg);
extern int func_8006F07C(int arg);
extern void func_800AB620(int arg);
extern void func_800B6D78(void);
extern void func_801CD244(void);
extern int func_801CDB48(void);

void func_80016F88(void) {
    unsigned int state = B(Scratchpad, 0x29A);

    if (state >= 6) {
        return;
    }

    switch (state) {
    case 0:
        if (func_8006F07C(0x10) != 0) {
            B(Scratchpad, 0x2E4) = 0;
            B(D_800FF748, 0x8F15) = 0;
            func_8004FE30();
            func_800171A0();
        }
        break;
    case 1:
        func_8004FEE8(0x150);
        if (func_800189F0() != 0) {
            func_800171A0();
            if (B(Scratchpad, 0x2E1) == 1 && B(D_800FF748, 0xABA8) == 0) {
                func_800171A0();
            } else {
                func_800B6D78();
            }
        }
        break;
    case 2:
        if (func_801CDB48() != 0) {
            func_800AB620(0x0E);
            func_800AB620(0x0F);
            func_800171A0();
            H(Scratchpad, 0x294) = 0;
            W(Scratchpad, 0x198) = 0;
            W(Scratchpad, 0x19C) = 1;
        }
        break;
    case 3:
        func_800B6D78();
        func_80017564(0x0E);
        func_80019564(D_801A6000, 0);
        func_80017564(0xC4);
        func_80019564(D_801A6000, 0);
        func_800198E8(0x0A);
        func_800171A0();
        break;
    case 4:
        func_800B6D78();
        func_800AB620(0x202);
        func_800171A0();
        W(Scratchpad, 0x19C) = 2;
        break;
    case 5:
        func_801CD244();
        break;
    }
}
