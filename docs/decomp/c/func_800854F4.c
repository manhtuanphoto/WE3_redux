/*
 * func_800854F4 - State machine for post-match / replay / end-of-match menu flow
 *
 * Purpose:
 *   Drives a 9-state menu/transition sequence controlled by D_800EF838 (cases 0..8),
 *   fading/drawing UI overlays, handling up/down/confirm pad inputs for a 3-option
 *   menu (D_800F109C = 0..2), and transitioning out to replay, rematch, or exit.
 *
 * Arguments:
 *   None (void).
 *
 * Return value:
 *   None (void).
 *
 * Scratchpad / Global fields read & written:
 *   Scratch 0x294 (u16) - frame/wait counter
 *   Scratch 0x29F (u8)  - cleared to 0 when option 1 selected
 *   Scratch 0x2E1 (u8)  - game mode (checked == 4 in state 0)
 *   Scratch 0x2E3 (u8)  - checked == 3 in state 2
 *   Scratch 0x2F2 (u8)  - checked != 0 in state 2
 *   Scratch 0x32C (u32) - set to 0x400 in state 5
 *   D_800EF838    (u8)  - state index (0..8)
 *   D_800F109C    (u8)  - menu cursor index (0..2)
 *   D_8010A5A8    (struct) - +0x09 (u8), +0x0C (s16) read in states 7 & 8
 *   D_8010A320    (u8)  - cleared in state 7 (D_800FF748 + 0xABD8)
 *   D_8010A322    (u8)  - cleared in states 7 & 8 (D_800FF748 + 0xABDA)
 *   D_800CA5DC..E (u8)  - written in state 8 (2, 0x3C, 0)
 *
 * Magic numbers:
 *   0x191 (401)   = timeout in frames (~13.37 s at 30 Hz) in state 1
 *   0x1000/0x800/0x4000/0x20 = menu input masks (up, start, down, confirm)
 *   0xFFDF        = low 16 bits represent -33 y-offset for menu cursor
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))

extern unsigned char D_800FF748[];
extern unsigned char D_8010A5A8[];
extern unsigned char D_800EF838;
extern unsigned char D_800F109C;
extern unsigned char D_800CA5DC;
extern unsigned char D_800CA5DD;
extern unsigned char D_800CA5DE;

extern void func_80051810(void);
extern int func_8004FB68(int);
extern void func_8001CDAC(int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int);
extern int func_8006F87C(int, int, int, int);
extern int func_800525A4(void);
extern int func_8006F934(int, int, int, int);
extern void func_80071AF8(int, int, int, int, int);
extern unsigned char *func_8001D004(int);
extern void func_8006F4A8(int, int);
extern int func_80071D94(int, int);
extern int func_8006EF90(void);
extern void func_800AB620(int);
extern void func_8006FCFC(int);
extern int func_8004FBC4(int);
extern void func_800B53D8(int);
extern void func_8004FC54(int);
extern void func_8002247C(void);
extern void func_8001722C(int);
extern void func_800171C4(int);
extern void func_800171FC(int);
extern void func_8004FE30(void);
extern void func_8004FEE8(int);
extern int func_800188B4(int);
extern void func_80017178(int);

void func_800854F4(void)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;

    func_80051810();

    switch (D_800EF838) {
    case 0:
        if (B(scratch, 0x2E1) == 4 && func_8004FB68(0) == 0) {
            return;
        }
        func_8001CDAC(1, 0x93, 0x40000000, 0, 0, 6, 0, 0x1000,
                      0, 0, 0, 0x80, 0x80, 0x80, 0, 1);
        H(scratch, 0x294) = 0;
        D_800EF838++;
        break;

    case 1:
        if (func_8006F87C(1, 0x1000, 0x1000, 0x200) == 0) {
            return;
        }
        H(scratch, 0x294) = (unsigned short)(H(scratch, 0x294) + 1);
        if (H(scratch, 0x294) < 0x191 && func_800525A4() == 0) {
            return;
        }
        D_800EF838++;
        break;

    case 2: {
        unsigned int sel;
        if (func_8006F934(1, 0x1000, 0, 0x400) == 0) {
            return;
        }
        D_800F109C = 0;
        func_80071AF8(5, 3, 0x83, 0x84, 0);
        if (B(scratch, 0x2E3) == 3 && B(scratch, 0x2F2) != 0) {
            unsigned char *obj = func_8001D004(0x68);
            H(obj, 0x12) = 0x4F;
            H(obj, 0x2A) = 0x4F;
            H(obj, 0x20) = 7;
            H(obj, 0x2C) = 7;
        }
        sel = D_800F109C;
        func_8006F4A8(2, (int)(sel * 18 + 0xFFDF));
        H(scratch, 0x294) = 0;
        D_800EF838++;
        break;
    }

    case 3: {
        unsigned short cnt = H(scratch, 0x294);
        H(scratch, 0x294) = (unsigned short)(cnt + 1);
        if (cnt < 8) {
            return;
        }
        if (func_80071D94(5, 3) != 0) {
            return;
        }
        D_800EF838++;
        break;
    }

    case 4: {
        int input = func_8006EF90();

        if (input == 0x800 || input == 0x20) {
            func_800AB620(0x528);
            D_800EF838++;
        } else if (input == 0x4000) {
            unsigned char sel = D_800F109C;
            sel = (sel < 2) ? (unsigned char)(sel + 1) : 0;
            D_800F109C = sel;
            func_8006F4A8(2, (int)(sel * 18 + 0xFFDF));
            func_800AB620(0x50D);
        } else if (input == 0x1000) {
            unsigned char sel = D_800F109C;
            sel = (sel != 0) ? (unsigned char)(sel - 1) : 2;
            D_800F109C = sel;
            func_8006F4A8(2, (int)(sel * 18 + 0xFFDF));
            func_800AB620(0x50D);
        }
        func_8006FCFC(0x36);
        break;
    }

    case 5:
        if (func_8004FBC4(0) == 0) {
            return;
        }
        W(scratch, 0x32C) = 0x400;
        func_800B53D8(0x400);
        func_8004FC54(1);

        switch (D_800F109C) {
        case 0:
            func_800AB620(0xE);
            func_8002247C();
            func_8001722C(1);
            func_800171C4(0);
            break;
        case 1:
            B(scratch, 0x29F) = 0;
            D_800EF838++;
            break;
        case 2:
            D_800EF838 = 0;
            func_800171FC(4);
            break;
        }
        break;

    case 6:
        func_8004FE30();
        D_800EF838++;
        break;

    case 7:
        func_8004FEE8(0xA0);
        if (func_800188B4(((B(D_8010A5A8, 9) >> 2) & 3) + 0xC7) == 0) {
            return;
        }
        B(D_800FF748, 0xABDA) = 0; /* 0x8010A322 */
        B(D_800FF748, 0xABD8) = 0; /* 0x8010A320 */
        D_800EF838++;
        break;

    case 8:
        func_800AB620(0x206);
        func_80017178(4);
        D_800CA5DC = 2;
        D_800CA5DD = 0x3C; /* 2.0 s at 30 Hz */
        D_800CA5DE = 0;
        func_800171C4((S(D_8010A5A8, 0xC) != 0) ? 0x30 : 0x31);
        func_800171FC(0);
        B(D_800FF748, 0xABDA) = 0; /* 0x8010A322 */
        break;
    }
}
