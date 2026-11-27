/*
 * func_8001FA1C
 *
 * Purpose:
 *   Sets up the stadium 3D scene objects (pitch, stands, goals at +-0x3320,
 *   advertising boards, crowd/roof variants per stadium ID +0x2E3, mode +0x2E1,
 *   and time-of-day +0x2E5), configures ambient/lighting parameters via
 *   func_8001E138, calls func_8001F7C4, and initializes 3 scene-object material/flag
 *   blocks returned by func_8001D004.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global / Scratchpad (0x1F800000) fields read:
 *   Scratchpad +0x2E1 (u8) match/venue mode (5 checked),
 *   Scratchpad +0x2E3 (u8) stadium index (0..5),
 *   Scratchpad +0x2E5 (u8) time-of-day / lighting variant (1 checked)
 *
 * Global fields written:
 *   D_800FF748 + 0xABE1 (u8) set to 1
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   Goal X positions: -0x3320 (-13088 raw = -52.352 m) and +0x3320 (+13088 raw = +52.352 m).
 *   Scale factor: 0x1000 (4096 = 1.0 in PS1 1.19.12 fixed point).
 *   Material/brightness bytes set per stadium: 0 (stadium 0), 0x10 (stadium 2), 8 (others).
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define H(p, o)  (*(unsigned short *)((p) + (o)))
#define S(p, o)  (*(signed short *)((p) + (o)))
#define W(p, o)  (*(unsigned int *)((p) + (o)))
#define SW(p, o) (*(signed int *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];

extern void func_8001D470(int objId, int slot, int x, int y, int z, int sx, int sy, int sz, int modelId);
extern void func_8001E138(int a0, int a1, int a2);
extern void func_8001F7C4(void);
extern unsigned char *func_8001D004(int id);

void func_8001FA1C(void)
{
    int i;

    func_8001D470(3, 0, 0, 0, 0, 0x1000, 0x1000, 0x1000, 4);
    func_8001D470(4, 0, 0, 0, 0x80, 0x1000, 0x1000, 0x1000, 5);
    func_8001D470(0xD, 1, 0, 0, 0, 0x1000, 0x1000, 0x1000, 6);
    func_8001D470(0xF, 2, 0, 0, 0, 0x1000, 0x1000, 0x1000, 8);

    if (B(Scratchpad, 0x2E3) == 3) {
        func_8001D470(0x10, 3, 0, 0, 0, 0x1000, 0x1000, 0x1000, 9);
    } else {
        func_8001D470(0x10, 2, 0, 0, 0x80, 0x1000, 0x1000, 0x1000, 9);
    }

    func_8001D470(0xC, 4, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0xA);
    func_8001D470(0xE, 1, 0, 0, 0x80, 0x1000, 0x1000, 0x1000, 7);

    if (B(Scratchpad, 0x2E3) != 4) {
        func_8001D470(5, 5, 0, 0, 0, 0x1000, 0x1000, 0x1000, 1);
    }

    func_8001D470(8, 7, -0x3320, 0, 0, 0x1000, 0x1000, 0x1000, 0xB);
    func_8001D470(9, 7, 0x3320, 0, 0x80, 0x1000, 0x1000, 0x1000, 0xC);
    func_8001D470(2, 6, 0, 0, 0x80, 0x1000, 0x1000, 0x1000, 3);
    func_8001D470(1, 6, 0, 0, 0, 0x1000, 0x1000, 0x1000, 2);

    if (B(Scratchpad, 0x2E3) == 5) {
        func_8001D470(6, 0xA, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1E);
        func_8001D470(7, 0xA, 0, 0, 0x80, 0x1000, 0x1000, 0x1000, 0x1F);
    }

    if (B(Scratchpad, 0x2E1) == 5) {
        func_8001D470(0xA, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0x1C);
        func_8001D470(0xB, 8, 0, 0, 0x80, 0x1000, 0x1000, 0x1000, 0x1D);
    } else {
        func_8001D470(0xA, 8, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0xD);
        func_8001D470(0xB, 8, 0, 0, 0x80, 0x1000, 0x1000, 0x1000, 0xE);
        if (B(Scratchpad, 0x2E5) == 1) {
            func_8001D470(0x11, 9, 0, 0, 0, 0x1000, 0x1000, 0x1000, 0xF);
        }
    }

    B(D_800FF748, 0xABE1) = 1;

    if (B(Scratchpad, 0x2E1) == 5) {
        func_8001E138(0x48, 0x60, 0x30);
    } else {
        switch (B(Scratchpad, 0x2E3)) {
        case 0:
        case 4:
            func_8001E138(0x40, 0x60, 0x30);
            break;
        case 1:
        case 5:
            func_8001E138(0x40, 0x60, 0x28);
            break;
        case 2:
        case 3:
            func_8001E138(0x48, 0x60, 0x30);
            break;
        default:
            break;
        }
    }

    func_8001F7C4();

    for (i = 0; i < 3; i++) {
        int objId;
        unsigned char *obj;
        unsigned char val;

        if (i == 0) {
            objId = 0x28;
        } else if (i == 1) {
            objId = 0x42;
        } else {
            objId = 0x43;
        }

        obj = func_8001D004(objId);

        switch (B(Scratchpad, 0x2E3)) {
        case 0:
            val = 0;
            break;
        case 2:
            val = 0x10;
            break;
        case 1:
        case 3:
        case 4:
        default:
            val = 8;
            break;
        }

        B(obj, 0x02) = val;
        B(obj, 0x0E) = val;
        B(obj, 0x1A) = val;
        if (i == 2) {
            B(obj, 0x26) = val;
            B(obj, 0x32) = val;
            B(obj, 0x3E) = val;
        }
    }
}
