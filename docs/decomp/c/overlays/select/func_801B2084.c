/*
 * func_801B2084 (overlay: select)
 *
 * Purpose:
 *   Top-level state machine dispatcher for the select / tournament-bracket flow,
 *   indexed by D_8010A322 (0..0xD3) through jump table jtbl_8018A9A4. Handles
 *   menu/screen initialization, fade-in/fade-out checks (func_8006F07C,
 *   func_8006F0DC), sub-screen/dialog handlers (func_801AB6E8, func_801ACE2C,
 *   func_801AE294, func_801B190C, func_801B1B0C, func_801B5A7C), and building
 *   the 16-team knockout bracket D_801D8768 from the 8 group tables
 *   (func_801B4C84 / D_801D8748).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Globals / Structs read & written:
 *   D_8010A322 (== D_800FF748 + 0xABDA, u8, R/W): current state index (0..0xD3)
 *   D_800FF748 (u8[], R/W): game/tournament state block:
 *     +0x8F1F (u8, W): set to 0x60 in setup states
 *     +0x8F20 (u8, W): set to 0x10 in setup states
 *     +0x8F21 (u8, R/W): remaining active/user team count (16 down to 0)
 *     +0x8F22 (u8, W): cleared to 0 (== D_8010866A)
 *     +0x8F23 (u8, W): cleared to 0 (== D_8010866B)
 *     +0x8F24 + team * 36 (u8, R): team flags (0xC0 mask checked == 0x40)
 *     +0x8F27 + team * 36 (u8, R): byte read during init
 *     +0x8F37 + slot * 36 (u8, W): slot status/phase byte (0 or 4)
 *     +0xAB80 + slot (u8, W): slot-to-team index mapping (0..31)
 *     +0xABA0..+0xABA8 (u8, R/W): match/selection config bytes
 *     +0xABD8 (u8, W): sub-state / timer byte (cleared or set to 0x14)
 *   D_801D873C (u8 *, R/W): pointer returned by func_801B4C84
 *   D_801D8748[32] (u8, R/W): group standings team table
 *   D_801D8768[32] (u8, R/W): knockout bracket (0..15) + eliminated (16..31)
 *   D_801D8A51, D_801D8A82..D_801D8A8F, D_801D8B18 (u8/u16, R/W): menu/UI state
 *   D_801DA058, D_801DA068..D_801DA06B, D_801DA2F0 (u8/u32, W): menu/UI flags
 *   D_801DA5D0..D_801DA5D6 (u16, W): list cursor/count fields (count = 9)
 *   D_801DAD84, D_801DAD85 (u8, W): UI mode bytes (4, 1)
 *   D_800AD638 (u16, W): cleared to 0
 *
 * Scratchpad (0x1F800000) read/written:
 *   +0x29F (u8, W): cleared to 0 on func_801ACE2C(1) == 3
 *   +0x2A2 (u8, W): set to 4 on init, cleared to 0 before func_801AB510
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char  D_8010A322;
extern unsigned char  D_800FF748[];
extern void          *jtbl_8018A9A4[];

extern unsigned short D_800AD638;
extern unsigned char *D_801D873C;
extern unsigned char  D_801D8748[];
extern unsigned char  D_801D8768[];
extern unsigned char  D_801D8A51;
extern unsigned short D_801D8A82;
extern unsigned short D_801D8A84;
extern unsigned short D_801D8A86;
extern unsigned short D_801D8A88;
extern unsigned char  D_801D8A8A;
extern unsigned char  D_801D8A8B;
extern unsigned char  D_801D8A8C;
extern unsigned char  D_801D8A8D;
extern unsigned char  D_801D8A8E;
extern unsigned char  D_801D8A8F;
extern unsigned char  D_801D8B18;
extern unsigned int   D_801DA058;
extern unsigned char  D_801DA068;
extern unsigned char  D_801DA069;
extern unsigned char  D_801DA06A;
extern unsigned char  D_801DA06B;
extern unsigned char  D_801DA2F0;
extern unsigned short D_801DA5D0;
extern unsigned short D_801DA5D2;
extern unsigned short D_801DA5D4;
extern unsigned short D_801DA5D6;
extern unsigned char  D_801DAD84;
extern unsigned char  D_801DAD85;

extern void           func_800171C4(int sfx);
extern void           func_800171FC(int mode);
extern int            func_8006F07C(int step);
extern int            func_8006F0DC(unsigned int step, unsigned int mode);
extern void           func_8018E538(int arg0);
extern void           func_801AB510(unsigned char *table);
extern int            func_801AB6E8(int arg0);
extern int            func_801ACE2C(int arg0);
extern int            func_801AE294(int arg0);
extern int            func_801B190C(void);
extern int            func_801B1B0C(int arg0);
extern void           func_801B30FC(void);
extern unsigned char  func_801B38E0(int arg0, signed short arg1);
extern unsigned char *func_801B4C84(unsigned int group_idx);
extern void           func_801B5A7C(void);

void func_801B2084(void)
{
    unsigned int state = D_8010A322;
    unsigned char *base = D_800FF748;

    if (state >= 0xD4u) {
        return;
    }

    /*
     * UNCLEAR: jtbl_8018A9A4 (0xD4 entries) is external rodata not included in
     * the assembly file, so the exact state-number-to-label mapping cannot be
     * verified from this file alone. Each jlabel block is preserved verbatim.
     */
    goto *jtbl_8018A9A4[state];

L801B20DC:
    D_801D8A8A = 0x5F;
    D_801D8A8B = 0xFF;
    D_801D8A82 = 0;
    D_801D8A84 = 0;
    D_801D8A86 = 0;
    D_801D8A88 = 0;
    D_801D8A8F = 0;
    D_801D8A8C = 0;
    D_801D8A8D = 0x60;
    D_801DA5D0 = 0;
    D_800AD638 = 0;
    D_801DA06B = 0;
    D_801DA069 = 0;
    D_801DA06A = 0;
    D_801DA068 = 0;
    D_801DA5D2 = 0;
    D_801DA5D4 = 1;
    D_801DA5D6 = 9;
    D_801D8A8E = (unsigned char)((D_801D8A8E & 0x61u) | 0x21u);
    func_801B30FC();

    {
        unsigned int team = B(base, 0xABA0);
        unsigned char val = B(base, 0x8F27 + team * 36);

        B(base, 0x8F22) = 0;
        B(base, 0x8F23) = 0;
        B(base, 0xABA8) = 0;
        D_801DA2F0 = 0;
        D_801DAD85 = 1;
        D_801D8B18 = 0;
        B(base, 0xABA1) = 0;
        B(base, 0xABA0) = 0;
        B(base, 0xABA3) = 0;
        B(base, 0xABA2) = 0;
        B(base, 0xABA6) = 0;
        B(base, 0xABA4) = 0;
        B(base, 0xABA7) = 0;
        B(base, 0xABA5) = 0;
        D_800AD638 = 0;
        D_801DAD84 = 4;
        D_801D8A51 = 0;
        D_801DA5D2 = 0;
        D_801DA5D4 = 1;
        D_801DA5D6 = 9;

        D_801DA5D0 = val;
        D_801DA5D0 = 0;
        func_8018E538(0);
    }

    B(Scratchpad, 0x2A2) = 4;
    func_801B30FC();
    func_801B38E0(0, 0);
    func_801AB6E8(0x5B);
    goto L801B28D4;

L801B22D4:
    if (func_8006F07C(0x10) != 0) {
        goto L801B28D4;
    }
    return;

L801B22E4:
    if (func_801AB6E8(0x5B) != 0) {
        D_801D8A8F = 1;
        D_801DA2F0 = 1;
        B(base, 0xABD8) = 0x14;
        B(base, 0xABDA) = 0x0B; /* D_8010A322 = 11 */
    }
    return;

L801B232C:
    {
        unsigned int cur = B(base, 0xABDA);
        B(base, 0xABD8) = 0;
        B(base, 0xABDA) = (unsigned char)(cur + 1);
    }
    return;

L801B234C:
    if (func_801B190C() == 2) {
        B(base, 0xABDA) = 0x14; /* D_8010A322 = 20 */
    }
    return;

L801B2368:
    {
        unsigned int cur = B(base, 0xABDA);
        B(base, 0xABD8) = 0;
        B(base, 0xABDA) = (unsigned char)(cur + 1);
    }
    return;

L801B2388:
    {
        int r = func_801B1B0C(0);
        if (r == 1) {
            B(base, 0xABDA) = 0x0A; /* D_8010A322 = 10 */
        } else if (r == 2) {
            B(base, 0xABDA) = 0x1E; /* D_8010A322 = 30 */
        }
    }
    return;

L801B23C8:
    {
        unsigned int cur = B(base, 0xABDA);
        B(base, 0xABD8) = 0;
        B(base, 0xABDA) = (unsigned char)(cur + 1);
    }
    return;

L801B23E8:
    {
        int r = func_801AE294(0);
        if (r == 1) {
            B(base, 0xABDA) = 0x14; /* D_8010A322 = 20 */
        } else if (r == 2) {
            B(base, 0xABDA) = 0x28; /* D_8010A322 = 40 */
        }
    }
    return;

L801B2428:
    {
        unsigned int cur = B(base, 0xABDA);
        B(base, 0xABD8) = 0;
        B(base, 0xABDA) = (unsigned char)(cur + 1);
    }
    return;

L801B2448:
    {
        int r = func_801AE294(1);
        if (r == 1) {
            B(base, 0xABDA) = 0x1E; /* D_8010A322 = 30 */
        } else if (r == 2) {
            B(base, 0xABDA) = 0x32; /* D_8010A322 = 50 */
        }
    }
    return;

L801B2494:
    if (D_801D8B18 != 1) {
        int s3 = 0;
        int s0 = 0;
        int i;

        while (s3 < 0x20) {
            D_801D873C = func_801B4C84((unsigned int)(s0 & 0xFF));
            for (i = 0; i < 4; i++) {
                int slot = s3 + i;
                unsigned char team_id = D_801D873C[i * 2];
                D_801D8748[slot] = team_id;
                B(base, 0x8F37 + slot * 36) = 4;
                /*
                 * NOTE: In the MIPS binary, `addiu $s3, $s3, 4` sits in the
                 * delay slot of the inner `bnez .L801B24CC`, so it executes on
                 * every inner iteration (advancing s3 by 16 per outer pass)
                 * rather than once per outer pass.
                 */
                s3 += 4;
            }
            s0++;
        }

        /* Seed the 16-team knockout bracket (A1 vs B2, C2 vs D1, etc.) */
        D_801D8768[0]  = D_801D8748[0];
        D_801D8768[1]  = D_801D8748[5];
        D_801D8768[3]  = D_801D8748[12];
        D_801D8768[2]  = D_801D8748[9];
        D_801D8768[4]  = D_801D8748[16];
        D_801D8768[5]  = D_801D8748[21];
        D_801D8768[6]  = D_801D8748[25];
        D_801D8768[7]  = D_801D8748[28];
        D_801D8768[8]  = D_801D8748[4];
        D_801D8768[9]  = D_801D8748[1];
        D_801D8768[10] = D_801D8748[13];
        D_801D8768[11] = D_801D8748[8];
        D_801D8768[12] = D_801D8748[20];
        D_801D8768[13] = D_801D8748[17];
        D_801D8768[14] = D_801D8748[29];
        D_801D8768[15] = D_801D8748[24];

        /* Copy 3rd and 4th place teams from each of the 8 groups to [16..31] */
        for (i = 0; i < 8; i++) {
            D_801D8768[16 + i * 2]     = D_801D8748[i * 4 + 2];
            D_801D8768[16 + i * 2 + 1] = D_801D8748[i * 4 + 3];
        }

        B(base, 0x8F21) = 0x10;
        for (i = 0; i < 0x10; i++) {
            unsigned int team = D_801D8768[i];
            if ((B(base, 0x8F24 + team * 36) & 0xC0u) == 0x40u) {
                B(base, 0x8F21) = (unsigned char)(B(base, 0x8F21) - 1);
            }
        }

        if (B(base, 0x8F21) != 0) {
            goto L801B28D4;
        }
        D_801D8B18 = 1;
    }
    B(base, 0xABDA) = 0xC8; /* D_8010A322 = 200 */
    return;

L801B2728:
    if (func_801AB6E8(0x5C) == 2) {
        goto L801B28D4;
    }
    return;

L801B2744:
    if (func_8006F0DC(0x10, 0) != 0) {
        B(base, 0xABDA) = 0x3C; /* D_8010A322 = 60 */
    }
    return;

L801B276C:
    {
        int i;
        B(base, 0x8F1F) = 0x60;
        B(base, 0x8F20) = 0x10;
        func_800171C4(0x22);
        func_800171FC(0);
        B(base, 0xABDA) = 1; /* D_8010A322 = 1 */
        for (i = 0; i < 0x20; i++) {
            B(base, 0xAB80 + i) = (unsigned char)i;
            B(base, 0x8F37 + i * 36) = (i < 0x10) ? 0 : 4;
        }
        goto L801B2990;
    }

L801B280C:
    func_801B5A7C();
    return;

L801B281C:
    {
        int r = func_801ACE2C(1);
        if (r == 2) {
            B(base, 0xABDA) = 0xD2; /* D_8010A322 = 210 */
        } else if (r == 1) {
            D_801DA058 = 0;
            B(base, 0xABDA) = (unsigned char)(B(base, 0xABDA) + 1);
        } else if (r == 3) {
            B(Scratchpad, 0x29F) = 0;
            func_800171C4(0xFF);
        }
    }
    return;

L801B28A8:
    B(base, 0xABDA) = 0x28; /* D_8010A322 = 40 */
    return;

L801B28C0:
    if (func_8006F0DC(0x10, 0) == 0) {
        return;
    }
L801B28D4:
    B(base, 0xABDA) = (unsigned char)(B(base, 0xABDA) + 1);
    return;

L801B28FC:
    {
        int i;
        B(base, 0x8F1F) = 0x60;
        B(base, 0x8F20) = 0x10;
        func_800171C4(0x22);
        func_800171FC(2);
        B(base, 0xABDA) = 0; /* D_8010A322 = 0 */
        for (i = 0; i < 0x20; i++) {
            B(base, 0xAB80 + i) = (unsigned char)i;
            B(base, 0x8F37 + i * 36) = (i < 0x10) ? 0 : 4;
        }
    }

L801B2990:
    B(Scratchpad, 0x2A2) = 0;
    func_801AB510(D_801D8768);
}
