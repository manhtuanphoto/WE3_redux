/*
 * func_800859C0
 *
 * Purpose:
 *   3-stage post-match / result-screen transition state machine driven by
 *   D_800EF838 (states 0, 1, 2).
 *   - State 0: waits for func_8004FBC4(0) != 0, sets Scratchpad+0x32C = 0x400,
 *     calls func_800B53D8(0x400) and func_8004FE30(), and advances D_800EF838 to 1.
 *   - State 1: calls func_8004FEE8(0xA0), picks a sound/music/sequence ID (0xCA
 *     if Scratchpad+0x2E1 is 1 or 2, else 0xC7 + ((D_8010A5B1 >> 2) & 3)), polls
 *     func_800188B4() until non-zero, clears D_8010A322 and D_8010A320, and
 *     advances D_800EF838 to 2.
 *   - State 2: calls func_80017178(4), dispatches screen/audio setup based on
 *     Scratchpad+0x2E1 (1, 2, or other) and D_80108667 / D_8010866B, saves
 *     Scratchpad+0x29A/29B and D_800FF748+0xABDA (D_8010A322) into
 *     D_800CA5DC..D_800CA5DE, selects screen mode 0x30 or 0x31 depending on
 *     S(D_8010A5A8, 0xC), and clears D_800FF748+0xABDA.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Scratchpad (0x1F800000) / Global fields read & written:
 *   Scratchpad +0x29A (u8)  - match phase (copied to D_800CA5DC)
 *   Scratchpad +0x29B (u8)  - sub-phase/flag (copied to D_800CA5DD)
 *   Scratchpad +0x2E1 (u8)  - competition/match mode (1, 2 checked)
 *   Scratchpad +0x32C (u32) - set to 0x400 in state 0
 *   D_800EF838        (u8)  - state machine step (0 -> 1 -> 2)
 *   D_8010A5A8 +0xC   (s16) - flag selecting func_800171C4(0x30) vs (0x31)
 *   D_8010A5B1        (u8)  - bits [3:2] select 0xC7..0xCA track/ID
 *   D_80108667        (u8)  - bit 6 and low nibble [3:0] branch mode-2 setup
 *   D_8010866B        (u8)  - non-zero -> D_8010A322 = 0x64, zero -> 0x32
 *   D_8010A320        (u8)  - cleared to 0 in state 1
 *   D_8010A322        (u8)  - also accessed as D_800FF748 + 0xABDA
 *   D_800FF748 +0xABD6 (u16) - cleared to 0 in state 2 (mode 2)
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define SCRATCHPAD ((unsigned char *)0x1F800000)

extern unsigned char D_800EF838;
extern unsigned char D_8010A5A8[];
extern unsigned char D_800FF748[];
extern unsigned char D_8010A5B1;
extern unsigned char D_8010A322;
extern unsigned char D_8010A320;
extern unsigned char D_80108667;
extern unsigned char D_8010866B;
extern unsigned char D_800CA5DC;
extern unsigned char D_800CA5DD;
extern unsigned char D_800CA5DE;

extern int func_8004FBC4(int a0);
extern void func_800B53D8(int a0);
extern void func_8004FE30(void);
extern void func_8004FEE8(int a0);
extern int func_800188B4(int a0);
extern void func_80017178(int a0);
extern void func_800AB620(int a0);
extern void func_800171C4(int a0);
extern void func_800171FC(int a0);

void func_800859C0(void)
{
    unsigned char state = D_800EF838;

    if (state == 0) {
        if (func_8004FBC4(0) != 0) {
            W(SCRATCHPAD, 0x32C) = 0x400;
            func_800B53D8(0x400);
            func_8004FE30();
            D_800EF838 = D_800EF838 + 1;
        }
    } else if (state == 1) {
        unsigned int mode;
        int id;

        func_8004FEE8(0xA0);
        mode = B(SCRATCHPAD, 0x2E1);
        if ((unsigned int)(mode - 1) < 2U) {
            id = 0xCA;
        } else {
            id = ((D_8010A5B1 >> 2) & 0x3) + 0xC7;
        }

        if (func_800188B4(id) != 0) {
            D_8010A322 = 0;
            D_8010A320 = 0;
            D_800EF838 = D_800EF838 + 1;
        }
    } else if (state == 2) {
        unsigned char mode;
        int nextScreen;

        func_80017178(4);
        mode = B(SCRATCHPAD, 0x2E1);

        if (mode == 1) {
            func_800AB620(0x205);
            func_800171C4(0x20);
            func_800171FC(1);
            D_8010A322 = 0x14;
        } else if (mode == 2) {
            unsigned char flags;

            func_800AB620(0x205);
            flags = D_80108667;
            if ((flags >> 6) & 1) {
                func_800171C4(0x22);
                func_800171FC(1);
                /* Delay slot of `bnez $v0, .L80085B60` sets $v0 = 0x64 when D_8010866B != 0 */
                if (D_8010866B != 0) {
                    D_8010A322 = 0x64;
                } else {
                    D_8010A322 = 0x32;
                }
            } else {
                if ((flags & 0xF) != 0) {
                    func_800171C4(0x23);
                    func_800171FC(3);
                } else {
                    func_800171C4(0x21);
                    func_800171FC(1);
                }
                D_8010A322 = 0;
            }
            H(D_800FF748, 0xABD6) = 0;
        } else {
            func_800171C4(0);
        }

        D_800CA5DC = B(SCRATCHPAD, 0x29A);
        D_800CA5DD = B(SCRATCHPAD, 0x29B);
        D_800CA5DE = B(D_800FF748, 0xABDA);
        nextScreen = (S(D_8010A5A8, 0xC) != 0) ? 0x30 : 0x31;
        func_800171C4(nextScreen);
        func_800171FC(0);
        B(D_800FF748, 0xABDA) = 0;
    }
}
