/*
 * func_80081C30
 *
 * Purpose:
 *   Handles match transition / substitution commit (e.g., returning from tactics
 *   or half-time / replay setup). When func_8004FBC4(0) returns non-zero, triggers
 *   a 0x400 fade/wipe, clears state flags, applies pending player substitutions
 *   from D_8010A5D8 / D_800E7660 into the 11-player lineup tables at
 *   D_800FF748 + 0x8E9C (when Scratchpad+0x334 != 0), initializes camera/view
 *   via func_8001D14C and func_80081DEC, and resets match overlay flags before
 *   calling func_800171DC().
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read / written:
 *   D_8010A322, D_8010A368, D_8010A369, D_8010A400, D_800FB5AA, D_800FB5AB (u8) = 0
 *   D_8010A5A2 (u16) = 0
 *   D_800FF748 + 0x8E9C (u8[2][0x16] lineup IDs, 11 active checked per team)
 *   D_8010A5D8 (u8[2][3] outgoing player IDs)
 *   D_800E7660 (u8[2][3] incoming player IDs)
 *
 * Scratchpad (0x1F800000) fields read / written:
 *   +0x32C (u32) = 0x400
 *   +0x334 (u16; also u8[2] per-team pending sub counts)
 *   +0x2C2, +0x2A1, +0x1EC, +0x1E8, +0x2E7 (u8) = 0
 *   +0x2CE (u8) = 1
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_8010A322;
extern unsigned char D_8010A368;
extern unsigned char D_8010A369;
extern unsigned char D_8010A400;
extern unsigned short D_8010A5A2;
extern unsigned char D_800FB5AA;
extern unsigned char D_800FB5AB;
extern unsigned char D_800FF748[];
extern unsigned char D_8010A5D8[];
extern unsigned char D_800E7660[];

extern int func_8004FBC4(unsigned int arg0);
extern void func_800B53D8(unsigned int arg0);
extern void func_8001F53C(void);
extern void func_8004FD14(unsigned int arg0);
extern void func_8001D14C(
    int a0, int a1, int a2, int a3,
    int s0, int s1, int s2, int s3, int s4, int s5
);
extern void func_80081DEC(void);
extern void func_800171DC(void);

void func_80081C30(void)
{
    int team, sub, slot;

    if (func_8004FBC4(0) == 0) {
        return;
    }

    W(Scratchpad, 0x32C) = 0x400;
    func_800B53D8(0x400);

    D_8010A322 = 0;
    D_8010A369 = 0;
    D_8010A368 = 0;
    B(Scratchpad, 0x2C2) = 0;
    B(Scratchpad, 0x2A1) = 0;

    if (H(Scratchpad, 0x334) != 0) {
        for (team = 0; team < 2; team++) {
            int sub_count = (int)B(Scratchpad, 0x334 + team);
            for (sub = 0; sub < sub_count; sub++) {
                unsigned char out_id = D_8010A5D8[team * 3 + sub];
                unsigned char in_id = D_800E7660[team * 3 + sub];
                unsigned char *lineup = D_800FF748 + 0x8E9C + team * 0x16;

                for (slot = 0; slot < 11; slot++) {
                    if (out_id == lineup[slot]) {
                        lineup[slot] = in_id;
                    }
                }
            }
        }
    }

    func_8001F53C();
    func_8004FD14(1);
    func_8001D14C(0x17, 0x41, 0x35, -0x26F0, -0x800, 0x48, 0xE0C, 0x1000, 0xE0C, 1);

    D_8010A400 = 0;
    func_80081DEC();

    B(Scratchpad, 0x1EC) = 0;
    B(Scratchpad, 0x1E8) = 0;
    B(Scratchpad, 0x2CE) = 1;
    D_8010A5A2 = 0;
    D_800FB5AB = 0;
    D_800FB5AA = 0;
    B(Scratchpad, 0x2E7) = 0;
    func_800171DC();
}
