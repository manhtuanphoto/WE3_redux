/*
 * func_8011D82C (selform overlay)
 *
 * Purpose:
 *   Main state machine for the formation / lineup selection screen (SELFORM),
 *   dispatched by D_8010A321 (phases 0..3):
 *     - Phase 0: Initialize selection cursor/slot structs D_80123020[2] and
 *       flags (D_80123048, D_80123049, D_8012304A[2]), save Scratchpad+0x1EC
 *       into D_80123010 and clear Scratchpad+0x1EC, then advance to phase 1.
 *     - Phase 1: Register and configure UI primitives/sprites via func_8001CEE0
 *       and func_8001D004 (hiding 2P-side items when Scratchpad+0x2E1 == 5),
 *       initialize the 3D mesh/flag grid via func_8005C49C and set
 *       D_8010A370..D_8010A384 bounds based on camera parameter Scratchpad+0x32C,
 *       optionally pre-select a substituted player from Scratchpad+0x2AD,
 *       initialize both sides' formation widgets, dispatch mode-specific setup
 *       via jtbl_8011D800[Scratchpad+0x2E2], clear D_80123011 / D_8010C1D0, and
 *       advance to phase 2.
 *     - Phase 2: Poll screen readiness via func_8004FB68(0), run mode-specific
 *       input/confirmation loop via jtbl_8011D818[Scratchpad+0x2E2] (button
 *       confirm/cancel, timer timeout, func_8011ECE4, func_8012153C), and
 *       every 5 frames (Scratchpad+0x290 % 5 == 0) update cursor/highlight
 *       palette/animation via D_8010C1D0 and D_800CA300.
 *     - Phase 3: On exit readiness via func_8004FBC4(0), restore
 *       Scratchpad+0x1EC from D_80123010, clear D_8010A368/D_8010A369, tear down
 *       via func_8004FD14(1) / func_8006FEA8 / func_8006FF48(0xC), resolve
 *       selected player into Scratchpad+0x2AD (calling func_80120314 on change),
 *       call func_80072A2C, clear D_8010A320, and reset D_8010A321 to 1.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read / written:
 *   D_800CA300 (u8[])          - palette/highlight blink animation table (12 entries)
 *   D_800FF740 (u8)            - active side indicator in mode dispatch
 *   D_800FF748 (u8[])          - main game/actor state base:
 *     +0x0000.. (stride 0x3E8) - actor/player entries:
 *       +0x38D (u8)            - player/actor slot ID
 *       +0x392 (u8)            - squad/player ID within team
 *       +0x398 (u8)            - team index (indexes 22-byte lineup table)
 *     +0x5DCC (stride 0x28)    - UI element primitive pointer array (per slot)
 *     +0x5DD0 (stride 0x28)    - UI element s16 timer/state field (per slot)
 *     +0x6394 (u8 *)           - pointer to blink-controlled primitive
 *     +0x8E9C (u8[][22])      - team lineup tables (22 bytes/team; first 11 = starters)
 *     +0xABDC (u8[2])          - (= D_8010A324) per-side color/icon index into D_80122340
 *   D_8010A320 (u8)            - (= D_800FF748 + 0xABD8) cleared to 0 on exit
 *   D_8010A321 (u8)            - (= D_800FF748 + 0xABD9) formation menu phase (0..3)
 *   D_8010A368 (u8)            - cleared to 0 on exit
 *   D_8010A369 (u8)            - cleared to 0 when Scratchpad+0x2E1 == 5 or on exit
 *   D_8010A370..D_8010A384 (s16) - 3D mesh/camera bounds set in phase 1
 *   D_8010C1D0 (u8)            - blink animation step counter (0..11)
 *   D_80122340..D_80122343 (u8[]) - 4-byte UI color/offset table indexed by D_800FF748+0xABDC[side]
 *   D_80123010 (u8)            - saved Scratchpad+0x1EC across overlay lifetime
 *   D_80123011 (u8)            - cleared to 0 at end of phase 1
 *   D_80123020 (SelformSide[2]) - 2 x 18-byte per-side state structs:
 *     +0x00 (u16/u8)           - selected item/slot index
 *     +0x02..+0x0A (s16)       - cleared to 0 in phase 0
 *     +0x0C (u16)              - start slot index for blink range
 *     +0x0E (u16)              - end slot index for blink range (init 0xA, max 0x15)
 *     +0x10 (u16)              - side confirmed/ready flag
 *   D_80123044 (SelformSide *) - pointer to active SelformSide entry
 *   D_80123048 (u8)            - selected lineup slot (1-based; 0 = none)
 *   D_80123049 (u8)            - side index (0 or 1) for pre-selected player
 *   D_8012304A (u8[2])         - per-side flag cleared to 0 in phase 0
 *
 * Scratchpad (0x1F800000) fields read / written:
 *   +0x1EC (u8)                - saved to D_80123010 and cleared in phase 0; restored in phase 3
 *   +0x290 (s32)               - global frame counter (checked % 5 == 0 for blink)
 *   +0x294 (u16)               - sub-phase frame timeout counter (0..0xC0)
 *   +0x29A (u8)                - game/match sub-state (checked == 2 or 4)
 *   +0x2A0 (u8)                - checked == 0 for hiding/disabling slot 0x24
 *   +0x2A1 (u8)                - checked == 1 for substitution / player edit entry
 *   +0x2AD (u8)                - active/selected player slot ID
 *   +0x2DF (u8)                - side/controller bitmask (bit 0 = home/away side)
 *   +0x2E0 (u8)                - XOR mask applied to side index
 *   +0x2E1 (u8)                - match/player mode (5 = hide 2P-side UI)
 *   +0x2E2 (u8)                - sub-mode index (0..4) for jump tables jtbl_8011D800 / jtbl_8011D818
 *   +0x301..+0x304 (u8)        - per-side/slot UI y-offset parameters ((val << 4) - 0x100)
 *   +0x307..+0x308 (u8)        - per-side color/style toggle flags
 *   +0x32C (s32)               - camera FOV/zoom value mapped to D_8010A378/D_8010A37A
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

typedef struct SelformSide {
    unsigned short unk_00;
    signed short unk_02;
    signed short unk_04;
    signed short unk_06;
    signed short unk_08;
    signed short unk_0A;
    unsigned short unk_0C;
    unsigned short unk_0E;
    unsigned short unk_10;
} SelformSide; /* size = 0x12 (18 bytes) */

extern unsigned char D_800CA300[];
extern unsigned char D_800FF740;
extern unsigned char D_800FF748[];
extern unsigned char D_8010A320;
extern unsigned char D_8010A321;
extern unsigned char D_8010A368;
extern unsigned char D_8010A369;
extern signed short D_8010A370;
extern signed short D_8010A372;
extern signed short D_8010A374;
extern signed short D_8010A376;
extern signed short D_8010A378;
extern signed short D_8010A37A;
extern signed short D_8010A37C;
extern signed short D_8010A384;
extern unsigned char D_8010C1D0;
extern unsigned char D_80122340[];
extern unsigned char D_80122341[];
extern unsigned char D_80122342[];
extern unsigned char D_80122343[];
extern unsigned char D_80123010;
extern unsigned char D_80123011;
extern SelformSide D_80123020[2];
extern SelformSide *D_80123044;
extern unsigned char D_80123048;
extern unsigned char D_80123049;
extern unsigned char D_8012304A[2];

extern unsigned int func_800174E0(int port, int a1, unsigned int mask);
extern void func_8001CEE0(int slot, int id, int flags, int a3, int a4, int a5, int a6, int vis);
extern unsigned char *func_8001D004(int id);
extern unsigned int func_8004FB68(int a0);
extern unsigned int func_8004FBC4(int a0);
extern void func_8004FD14(int a0);
extern void func_8005C49C(void);
extern int func_8005CE14(unsigned int actor_id);
extern void func_8006FA84(int slot, int enable);
extern void func_8006FAF4(int slot, int a1, unsigned int a2);
extern void func_8006FB90(int slot, int idx, unsigned int val);
extern void func_8006FEA8(void);
extern void func_8006FF48(int a0);
extern void func_800729CC(void);
extern void func_80072A2C(void);
extern void func_800AB620(unsigned int sfx_id);

extern unsigned int func_8011ECE4(unsigned int side); /* returns u8 status */
extern void func_80120314(void);
extern void func_8012051C(unsigned int side, int idx, int val);
extern void func_8012059C(unsigned int side);
extern void func_80120698(unsigned int side);
extern void func_80120A58(unsigned int side);
extern void func_801211E8(unsigned int side_xored, unsigned int side);
extern void func_8012153C(unsigned int side);
extern void func_80121670(unsigned int side);
extern void func_8012199C(unsigned int side, unsigned int sel_idx);
extern void func_80121A2C(int side);

void func_8011D82C(void)
{
    unsigned char phase;
    unsigned int i;
    unsigned char *ptr;
    int vis_2p;
    unsigned int mode_idx;

    phase = D_8010A321;

    switch (phase) {
    case 0:
        for (i = 0; i < 2; i++) {
            SelformSide *side = &D_80123020[i];
            D_80123044 = side;
            side->unk_00 = 0;
            side->unk_02 = 0;
            side->unk_04 = 0;
            side->unk_06 = 0;
            side->unk_08 = 0;
            side->unk_0A = 0;
            side->unk_0C = 0;
            side->unk_0E = 0xA;
            side->unk_10 = 0;
            D_8012304A[i] = 0;
        }
        D_80123010 = B(Scratchpad, 0x1EC);
        B(Scratchpad, 0x1EC) = 0;
        D_80123049 = 0;
        D_80123048 = 0;
        D_8010A321 = D_8010A321 + 1;
        break;

    case 1:
        vis_2p = (B(Scratchpad, 0x2E1) != 5) ? 1 : 0;

        func_8001CEE0(0x01, 0x006A, 0x50000000, 0, 0, 9, 0, 1);
        func_8001CEE0(0x02, 0x006B, 0x50000000, 0, 0, 9, 0, vis_2p);
        func_8001CEE0(0x03, 0x1018, 0, 0, 0, 9, 0, 1);
        func_8001CEE0(0x04, 0x1019, 0, 0, 0, 9, 0, 1);
        func_8001CEE0(0x07, 0x101A, 0, 0, 0, 0xA, 0, 0);
        func_8001CEE0(0x08, 0x1009, 0, 0, 0, 9, 0, 1);
        func_8001CEE0(0x09, 0x100A, 0, 0, 0, 9, 0, (B(Scratchpad, 0x2E1) != 5) ? 1 : 0);
        func_8001CEE0(0x0A, 0x1004, 0, 0, 0, 9, 0, 1);
        func_8001CEE0(0x0B, 0x1005, 0, 0, 0, 9, 0, 1);
        func_8001CEE0(0x0C, 0x006E, 0, 0, 0, 9, 0, 1);
        func_8001CEE0(0x0D, 0x006F, 0, 0, 0, 9, 0, (B(Scratchpad, 0x2E1) != 5) ? 1 : 0);
        func_8001CEE0(0x0E, 0x0070, 0, 0, 0, 7, 0, 1);
        func_8001CEE0(0x0F, 0x0071, 0, 0, 0, 7, 0, (B(Scratchpad, 0x2E1) != 5) ? 1 : 0);
        func_8001CEE0(0x10, 0x0072, 0, 0, 0, 9, 0, 1);
        func_8001CEE0(0x11, 0x0072, 0, 0x106, 0, 9, 0, 0);
        func_8001CEE0(0x12, 0x0073, 0, 0, 0, 9, 0, 1);
        func_8001CEE0(0x13, 0x0074, 0, 0, 0, 9, 0, (B(Scratchpad, 0x2E1) != 5) ? 1 : 0);
        func_8001CEE0(0x14, 0x1007, 0, 0, 0, 9, 0, 1);
        func_8001CEE0(0x15, 0x1008, 0, 0, 0, 9, 0, (B(Scratchpad, 0x2E1) != 5) ? 1 : 0);
        func_8001CEE0(0x16, 0x006C, 0, 0, 0, 9, 0, 1);
        func_8001CEE0(0x17, 0x006D, 0, 0, 0, 9, 0, (B(Scratchpad, 0x2E1) != 5) ? 1 : 0);
        func_8001CEE0(0x18, 0x0075, 0, 0, 0, 9, 0, 1);
        func_8001CEE0(0x19, 0x0076, 0, 0, 0, 9, (B(Scratchpad, 0x2E1) == 5) ? 0x5B : 0x33, 1);

        func_8001CEE0(
            0x1A,
            (B(Scratchpad, 0x2DF) & 1) ? 0x1001 : 0x1000,
            0, 0, 0, 9, 0, 1
        );
        func_8001CEE0(
            0x1B,
            (B(Scratchpad, 0x2DF) & 1) ? 0x1000 : 0x1001,
            0, 0, 0, 9, 0, (B(Scratchpad, 0x2E1) != 5) ? 1 : 0
        );
        func_8001CEE0(0x1C, 0x1006, 0, 0, 0, 9, 0, 1);
        func_8001CEE0(0x1D, 0x1006, 0, 0x50, 0, 9, 0, (B(Scratchpad, 0x2E1) != 5) ? 1 : 0);

        func_8001CEE0(0x1E, 0x100B, 0, 0,    (B(Scratchpad, 0x301) << 4) - 0x100, 9, 0, 1);
        func_8001CEE0(0x1F, 0x100B, 0, 0x90, (B(Scratchpad, 0x302) << 4) - 0x100, 9, 0, 1);
        func_8001CEE0(0x20, 0x100C, 0, 0,    (B(Scratchpad, 0x303) << 4) - 0x100, 9, 0, 1);
        func_8001CEE0(0x21, 0x100C, 0, 0x90, (B(Scratchpad, 0x304) << 4) - 0x100, 9, 0, 1);

        func_8001CEE0(0x22, 0x100D, 0, 0, -0x100, 9, 0, 1);
        func_8001CEE0(0x23, 0x100E, 0, 0, -0x100, 9, 0, 1);
        func_8001CEE0(0x24, 0x100F, 0, 0, -4, 9, 0, 1);

        ptr = func_8001D004(0x100F);
        for (i = 0; i < 4; i++) {
            S(ptr, i * 12 + 8) = -0x60;
        }

        mode_idx = B(Scratchpad, 0x2E2);
        if (mode_idx == 3) {
            mode_idx = 0;
        } else if (mode_idx == 4) {
            mode_idx = 3;
        }

        func_8001CEE0(
            0x25,
            0x1010 + (B(Scratchpad, 0x2DF) & 1) + (mode_idx * 2),
            0, 0, -2, 9, 0, (B(Scratchpad, 0x2E1) != 5) ? 1 : 0
        );
        func_8001CEE0(0x26, 0x1003, 0, 0, 0, 9, 0, 1);
        func_8006FAF4(0x26, 0, (B(Scratchpad, 0x2DF) & 1) | 0x10);
        func_8001CEE0(0x27, 0x101B, 0, 0, 0, 9, 0, 1);

        func_8005C49C();
        D_8010A370 = -0xE4;
        D_8010A372 = 0xE4;
        D_8010A376 = -0xA4;
        D_8010A374 = -0xA4;
        D_8010A384 = -0x20;
        D_8010A37C = -0x20;

        {
            signed int fov = SW(Scratchpad, 0x32C);
            signed short bound;
            int set_bound = 1;

            if (fov == 0x150) {
                bound = 0x1A0;
            } else if (fov < 0x151) {
                if (fov == 0xC7) {
                    bound = 0xC8;
                } else if (fov == 0xD3) {
                    bound = 0xE0;
                } else {
                    set_bound = 0;
                }
            } else if (fov == 0x2C0) {
                bound = 0x400;
            } else if (fov < 0x2C1) {
                if (fov == 0x200) {
                    bound = 0x2C8;
                } else {
                    set_bound = 0;
                }
            } else if (fov == 0x400) {
                bound = 0x620;
            } else {
                set_bound = 0;
            }

            if (set_bound) {
                D_8010A37A = bound;
                D_8010A378 = bound;
            }
        }

        if (B(Scratchpad, 0x2E1) == 5) {
            D_8010A369 = 0;
        }

        if (B(Scratchpad, 0x2A1) == 1 && B(Scratchpad, 0x2AD) != 0) {
            unsigned char sub_state = B(Scratchpad, 0x29A);
            if (sub_state == 4 || sub_state == 2) {
                unsigned char player_slot = B(Scratchpad, 0x2AD);
                unsigned int team_slot;
                unsigned char team_id;
                unsigned char squad_num;
                unsigned int j;

                if (player_slot < 0xC) {
                    D_80123048 = player_slot;
                    if ((B(Scratchpad, 0x2DF) & 1) == 1) {
                        D_80123049 = 1;
                    }
                } else {
                    D_80123048 = player_slot - 0xB;
                    if ((B(Scratchpad, 0x2DF) & 1) == 0) {
                        D_80123049 = 1;
                    }
                }

                team_slot = player_slot % 24;
                team_id = B(D_800FF748, team_slot * 0x3E8 + 0x398);
                squad_num = B(D_800FF748, team_slot * 0x3E8 + 0x392);

                for (j = 0; j < 11; j++) {
                    if (B(D_800FF748, 0x8E9C + team_id * 22 + j) == squad_num) {
                        D_80123048 = (unsigned char)(j + 1);
                        break;
                    }
                }

                D_80123020[D_80123049].unk_00 = (unsigned short)(D_80123048 - 1);
                func_8006FA84(7, 1);
            }
        }

        if (B(Scratchpad, 0x2A0) == 0 || B(Scratchpad, 0x2E1) == 5) {
            func_8006FA84(0x24, 0);
        } else {
            func_80121A2C(0);
            func_80121A2C(1);
        }

        for (i = 0; i < 2; i++) {
            unsigned char *prim_a;
            unsigned char *prim_b;
            unsigned char color_idx;
            unsigned char *entry;

            func_80120A58(i);
            func_80120698(i);
            func_80121670(i);
            func_801211E8((i ^ B(Scratchpad, 0x2E0)) & 0xFF, i);
            func_8012059C(i);
            func_8012199C(i, (unsigned char)D_80123020[i].unk_00);

            ptr = func_8001D004(i + 0x6A);
            B(ptr, 0x0A) = 0x5B;
            B(ptr, 0x16) = 0x0A;

            func_8012051C(i, 0, 0x5E);
            func_8012051C(i, 1, 0x5E);

            prim_a = func_8001D004(0x1004);
            entry = prim_a + i * 12;
            if (B(Scratchpad, 0x2E1) == 5 && i != 0) {
                B(entry, 0x0A) = 0x5B;
            } else {
                B(entry, 0x0A) = 0x35;
            }

            color_idx = B(D_800FF748, 0xABDC + i);
            B(entry, 0x01) = D_80122340[color_idx * 4];
            B(entry, 0x03) = D_80122341[color_idx * 4];
            B(entry, 0x04) = D_80122342[color_idx * 4];
            S(entry, 0x06) = (signed short)(D_80122343[color_idx * 4] + 0xFFC0 + (signed int)(i * 0x4E));

            prim_b = func_8001D004(0x1005);
            entry = prim_b + i * 12;
            if (B(Scratchpad, 0x2E1) == 5 && i != 0) {
                B(entry, 0x0A) = 0x5B;
            } else {
                B(entry, 0x0A) = 0x35;
            }

            if (B(Scratchpad, 0x307 + i) != 0) {
                B(entry, 0x01) = 0x30;
                B(entry, 0x03) = 0x80;
                B(entry, 0x04) = 0x20;
                S(entry, 0x06) = (signed short)(i * 0x4E - 0x3C);
            } else {
                B(entry, 0x01) = 0x20;
                B(entry, 0x03) = 0xE0;
                B(entry, 0x04) = 0x80;
                S(entry, 0x06) = (signed short)(i * 0x4E - 0x37);
            }
        }

        switch (B(Scratchpad, 0x2E2)) {
        case 0:
        case 2:
        case 3:
            func_8012153C(1);
            break;

        case 1:
            func_8006FA84(0x11, 1);
            func_8006FA84(0x1B, 1);
            if (B(Scratchpad, 0x2A1) == 1) {
                unsigned int s = (D_800FF740 == 0) ? 1 : 0;
                D_80123020[s].unk_10 = 1;
                func_8012153C(s);
            }
            break;

        case 4:
            ptr = func_8001D004(0x6B);
            B(ptr, 0x16) = 0x5B;
            ptr = func_8001D004(0x6A);
            B(ptr, 0x16) = 0x5B;
            func_8006FA84(0x10, 0);
            func_8006FA84(0x1C, 0);
            func_8006FA84(0x1D, 0);
            H(Scratchpad, 0x294) = 0;
            break;

        default:
            break;
        }

        D_80123011 = 0;
        D_8010C1D0 = 0;
        D_8010A321 = D_8010A321 + 1;
        break;

    case 2:
        if (func_8004FB68(0) == 0) {
            break;
        }

        switch (B(Scratchpad, 0x2E2)) {
        case 0:
        case 2:
        case 3:
            if ((func_8011ECE4(0) & 0xFF) != 0) {
                D_8010A321 = D_8010A321 + 1;
            }
            break;

        case 1:
            for (i = 0; i < 2; i++) {
                SelformSide *side = &D_80123020[i];
                D_80123044 = side;

                if (side->unk_10 == 0) {
                    unsigned int res = func_8011ECE4(i & 0xFF) & 0xFF;
                    if (res == 1 || res == 2) {
                        unsigned int other = (i < 1) ? 1 : 0;
                        if (D_80123020[other].unk_10 != 0) {
                            D_8010A321 = D_8010A321 + 1;
                        } else {
                            D_80123044->unk_10 = 1;
                            func_8012153C(i & 0xFF);
                        }
                    }
                } else {
                    if (func_800174E0(i, 0, 0x40) != 0) {
                        unsigned int slot_idx;

                        D_80123044->unk_10 = 0;
                        ptr = func_8001D004(i + 0x6A);
                        if (D_80123044->unk_0C != 0) {
                            B(ptr, 0x0A) = 0xA;
                        }
                        if (D_80123044->unk_0E != 0x15) {
                            B(ptr, 0x16) = 0xA;
                        }
                        func_8006FA84(i + 0x10, 1);
                        func_8006FA84((B(Scratchpad, 0x2E0) ^ i) + 0x1C, 1);
                        slot_idx = i + 3;
                        func_8006FA84(slot_idx, 1);
                        W(D_800FF748, slot_idx * 0x28 + 0x5DCC) = (unsigned int)func_8001D004(i + 0x1018);
                        S(D_800FF748, slot_idx * 0x28 + 0x5DD0) = 0;
                        func_800AB620(0x529);
                        break;
                    }
                }
            }
            break;

        case 4:
            if (func_800174E0(0, 0, 0x800) != 0 || func_800174E0(0, 0, 0x20) != 0) {
                func_800AB620(0x528);
                D_8010A321 = D_8010A321 + 1;
            }
            if (H(Scratchpad, 0x294) < 0xC1) {
                H(Scratchpad, 0x294) = H(Scratchpad, 0x294) + 1;
            } else {
                D_8010A321 = D_8010A321 + 1;
            }
            break;

        default:
            break;
        }

        if ((SW(Scratchpad, 0x290) % 5) != 0) {
            break;
        }

        if (D_8010C1D0 < 0xB) {
            D_8010C1D0 = D_8010C1D0 + 1;
        } else {
            D_8010C1D0 = 0;
        }

        for (i = 0; i < 2; i++) {
            SelformSide *side = &D_80123020[i];
            unsigned int row;

            D_80123044 = side;
            for (row = side->unk_0C; row <= side->unk_0E; row++) {
                func_8006FB90(i + 0x16, row - D_80123044->unk_0C, (unsigned int)D_800CA300[D_8010C1D0] << 4);
            }
        }

        if (B(Scratchpad, 0x2E2) != 4) {
            unsigned char *blink_prim = (unsigned char *)W(D_800FF748, 0x6394);
            B(blink_prim, 0x0A) = ((D_8010C1D0 & 3) != 0) ? 0xC : 0x5B;
            if (B(Scratchpad, 0x2E2) == 1) {
                B(blink_prim, 0x16) = ((D_8010C1D0 & 3) != 0) ? 0xC : 0x5B;
            }
        }

        func_8006FB90(7, 0, (unsigned int)(D_8010C1D0 % 3) * 16 + 0x28);
        break;

    case 3:
        if (func_8004FBC4(0) == 0) {
            break;
        }

        D_8010A369 = 0;
        D_8010A368 = 0;
        B(Scratchpad, 0x1EC) = D_80123010;
        func_8004FD14(1);
        func_8006FEA8();

        /* Note: $s2 remains 1 from function prologue across the phase==3 branch. */
        if (B(Scratchpad, 0x2A1) == 1) {
            func_8006FF48(0xC);
        }

        if (D_80123048 != 0) {
            unsigned char orig_player;
            int actor_base_idx;
            unsigned char *actor;
            unsigned int j;

            func_800729CC();
            orig_player = B(Scratchpad, 0x2AD);
            actor_base_idx = func_8005CE14(orig_player) % 24;
            actor = D_800FF748 + actor_base_idx * 0x3E8 + 0x38D;

            for (j = 0; j < 11; j++) {
                unsigned char team_id = B(actor, 0xB); /* actor + 0x398 */
                unsigned char target_num = B(D_800FF748, 0x8E9C + team_id * 22 + (D_80123048 - 1));
                if (B(actor, 0x5) == target_num) { /* actor + 0x392 */
                    unsigned char new_player = B(actor, 0x0); /* actor + 0x38D */
                    B(Scratchpad, 0x2AD) = new_player;
                    if (new_player != orig_player) {
                        func_80120314();
                    }
                    break;
                }
                actor += 0x3E8;
            }
        }

        func_80072A2C();
        D_8010A320 = 0;
        D_8010A321 = 1;
        break;

    default:
        break;
    }
}
