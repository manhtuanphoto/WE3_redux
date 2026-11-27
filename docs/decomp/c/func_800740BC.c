/*
 * func_800740BC - Substitution / tactical overlay UI state machine.
 *
 * Purpose:
 *   Drives the in-match/pause substitution menu state machine (state D_8010A322:
 *   0 = initialize team player-slot lists & counters, 1 = spawn UI sprites/panels
 *   and configure slot colors, 2 = process input/timers/cursor blinking and team
 *   slot cycling), returning a status code (0 = stay, 1/2 = exit/confirm).
 *
 * Arguments:
 *   none.
 *
 * Return value:
 *   int: 0 during active menu or when early guard fails; 1 or 2 when exiting/confirmed.
 *
 * Scratchpad reads/writes:
 *   - 0x1F800290 (s32, R): frame counter (checked mod 5 for cursor flash).
 *   - 0x1F800294 (u16, R/W): auto-close / display timer (0x61 = 97 frames = 3.23 s).
 *   - 0x1F8002E1 (u8, R): game/UI mode (4 = special highlight color 0xA).
 *   - 0x1F8002E2 (u8, R): menu submode (0..4).
 *
 * Globals reads/writes:
 *   - D_8010A322 (== D_800FF748 + 0xABDA, u8, R/W): state machine phase (0, 1, 2).
 *   - D_800FF748 + 0xABAA (u8[2][22], W): team player slot order arrays.
 *   - D_800FF748 + 0x6404 / 0xAC20 / 0xAC21 (u8, W): flags cleared on exit condition.
 *   - D_800FF748 + 0xAC28..0xAC3C (s16, W): UI panel bounds/coordinates.
 *   - D_800EF840 (18-byte structs[2], R/W): per-team menu cursor/range records;
 *     D_800EF850 = D_800EF840 + 0x10.
 *   - D_800F1068 (u8*, R/W): pointer to active team's 18-byte record.
 *   - D_8010C1D0 (u8, R/W): 0..11 blink phase index into D_800CA300.
 *
 * Magic numbers (30 Hz):
 *   - 0x61 (97 frames) = 3.23 s timer limit in submode 0.
 *   - frame % 5 == 0 -> every 5 frames (6 Hz) advances blink phase D_8010C1D0 (0..11).
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];
extern unsigned char D_8010A322[];
extern unsigned char D_800EF840[];
extern unsigned char D_800EF850[];
extern unsigned char *D_800F1068;
extern unsigned char D_8010C1D0;
extern unsigned char D_800CA300[];

extern int func_8004FBC4(int a0);
extern void func_8006FF48(int a0);
extern void func_8001CEE0(int id, int a1, int a2, int a3, int a4, int a5, int a6, int a7);
extern void func_8007524C(int team);
extern unsigned char *func_8001D004(int id);
extern void func_8006FA84(int a0, int a1);
extern void func_8005C49C(void);
extern int func_8004FB68(int a0);
extern int func_800749A4(int team);
extern void func_80075B50(int team);
extern int func_800174E0(int a0, int a1, int a2);
extern void func_800AB620(int sfx);
extern void func_8006FB90(int a0, int a1, int a2);
extern int func_8006FDB4(int status, unsigned char *state_ptr);
extern void func_80072A2C(void);

int func_800740BC(void)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    int status = 0;
    int mode;
    int team, slot;
    unsigned char *obj;
    int ret;

    mode = B(D_8010A322, 0);

    if (mode == 0) {
        if (func_8004FBC4(0) == 0) {
            return 0;
        }
        func_8006FF48(0xC);
        for (team = 0; team < 2; team++) {
            unsigned char *order = D_800FF748 + 0xABAA + team * 22;
            unsigned char *rec = D_800EF840 + team * 18;
            D_800F1068 = rec;
            H(rec, 0x0) = 0;
            H(rec, 0x2) = 0;
            H(rec, 0x4) = 0;
            H(rec, 0x6) = 0;
            H(rec, 0x8) = 0;
            H(rec, 0xA) = 0;
            H(rec, 0xC) = 0;
            H(rec, 0xE) = 0xA;
            H(rec, 0x10) = 0;
            for (slot = 0; slot < 0x16; slot++) {
                if ((unsigned int)slot < 0xBU) {
                    B(order, slot) = (unsigned char)(0xB - slot);
                } else {
                    B(order, slot) = (unsigned char)(slot + 1);
                }
            }
        }
        D_8010C1D0 = 0;
        /* D_800FF748 + 0xABDA == D_8010A322 */
        B(D_800FF748, 0xABDA) = (unsigned char)(B(D_800FF748, 0xABDA) + 1);
    } else if (mode == 1) {
        func_8001CEE0(0x01, 0x6A, 0x50000000,     0, 0, 0x9, 0, mode);
        func_8001CEE0(0x02, 0x6B, 0x50000000,     0, 0, 0x9, 0, mode);
        func_8001CEE0(0x0C, 0x6C, 0x00000000,     0, 0, 0x9, 0, mode);
        func_8001CEE0(0x0D, 0x6D, 0x00000000,     0, 0, 0x9, 0, mode);
        func_8001CEE0(0x0E, 0x6E, 0x00000000,     0, 0, 0x9, 0, mode);
        func_8001CEE0(0x0F, 0x6F, 0x00000000,     0, 0, 0x9, 0, mode);
        func_8001CEE0(0x10, 0x70, 0x00000000,     0, 0, 0x7, 0, mode);
        func_8001CEE0(0x11, 0x71, 0x00000000,     0, 0, 0x7, 0, mode);
        func_8001CEE0(0x12, 0x72, 0x00000000,     0, 0, 0x9, 0, mode);
        func_8001CEE0(0x13, 0x72, 0x00000000, 0x106, 0, 0x9, 0,    0);
        func_8001CEE0(0x1A, 0x73, 0x00000000,     0, 0, 0x9, 0, mode);
        func_8001CEE0(0x1B, 0x74, 0x00000000,     0, 0, 0x9, 0, mode);
        func_8001CEE0(0x1C, 0x79, 0x00000000, -0x10, 0, 0xA, 0, mode);
        func_8001CEE0(0x1D, 0x7A, 0x00000000,  0x10, 0, 0xA, 0, mode);
        func_8001CEE0(0x1E, 0x77, 0x00000000, -0x10, 0, 0xA, 0, mode);
        func_8001CEE0(0x1F, 0x78, 0x00000000,  0x10, 0, 0xA, 0, mode);
        func_8001CEE0(0x23, 0x75, 0x00000000,     0, 0, 0x9, 0, mode);
        func_8001CEE0(0x24, 0x76, 0x00000000,     0, 0, 0x9, 0, mode);

        for (team = 0; team < 2; team++) {
            func_8007524C(team);
            obj = func_8001D004(0x6A + team);
            B(obj, 0xA) = 0x5B;
            if (B(scratch, 0x2E1) == 4) {
                B(obj, 0x16) = 0xA;
            } else {
                B(obj, 0x16) = 0x5B;
            }
        }

        switch (B(scratch, 0x2E2)) {
        case 0:
        case 2:
        case 3:
            obj = func_8001D004(0x6B);
            B(obj, 0x16) = 0x5B;
            break;
        case 1:
            func_8006FA84(0x13, 1);
            break;
        case 4:
            obj = func_8001D004(0x6B);
            B(obj, 0x16) = 0x5B;
            obj = func_8001D004(0x6A);
            B(obj, 0x16) = 0x5B;
            func_8006FA84(0x12, 0);
            H(scratch, 0x294) = 0;
            break;
        }

        func_8005C49C();
        S(D_800FF748, 0xAC28) = -0xE4;
        S(D_800FF748, 0xAC2A) = 0xE4;
        S(D_800FF748, 0xAC2E) = -0xA8;
        S(D_800FF748, 0xAC2C) = -0xA8;
        S(D_800FF748, 0xAC32) = 0x640;
        S(D_800FF748, 0xAC30) = 0x640;
        S(D_800FF748, 0xAC3C) = -0x20;
        S(D_800FF748, 0xAC34) = -0x20;
        B(D_800FF748, 0xABDA) = (unsigned char)(B(D_800FF748, 0xABDA) + 1);
    } else if (mode == 2) {
        int skip_blink_update = 0;

        if (func_8004FB68(0) == 0) {
            return 0;
        }

        switch (B(scratch, 0x2E2)) {
        case 0:
        case 2:
        case 3:
            ret = func_800749A4(0) & 0xFF;
            if (ret == 1 || ret == 2) {
                status = 2;
            }
            break;
        case 1:
            for (team = 0; team < 2; team++) {
                unsigned char *rec = D_800EF840 + team * 18;
                D_800F1068 = rec;
                if (H(rec, 0x10) == 0) {
                    if ((func_800749A4(team) & 0xFF) == 2) {
                        if (H(D_800EF850, (team == 0) ? 0x12 : 0) != 0) {
                            status = 2;
                        } else {
                            H(D_800F1068, 0x10) = 1;
                            func_80075B50(team);
                        }
                    }
                } else {
                    if (func_800174E0(team, 0, 0x40) != 0) {
                        H(D_800F1068, 0x10) = 0;
                        obj = func_8001D004(team + 0x6A);
                        if (H(D_800F1068, 0xC) != 0 && B(scratch, 0x2E1) == 4) {
                            B(obj, 0xA) = 0xA;
                        }
                        if (H(D_800F1068, 0xE) != 0x15 && B(scratch, 0x2E1) == 4) {
                            B(obj, 0x16) = 0xA;
                        }
                        func_8006FA84(team + 0x12, 1);
                        func_800AB620(0x529);
                        break;
                    }
                }
            }
            break;
        case 4:
            if (H(scratch, 0x294) < 0x61U) {
                H(scratch, 0x294) = (unsigned short)(H(scratch, 0x294) + 1);
            } else {
                status = 1;
            }
            break;
        }

        if (!skip_blink_update && (SW(scratch, 0x290) % 5) == 0) {
            if (D_8010C1D0 < 0xBU) {
                D_8010C1D0 = (unsigned char)(D_8010C1D0 + 1);
            } else {
                D_8010C1D0 = 0;
            }
            for (team = 0; team < 2; team++) {
                unsigned char *rec = D_800EF840 + team * 18;
                unsigned int idx;
                D_800F1068 = rec;
                idx = H(rec, 0xC);
                while (idx <= H(D_800F1068, 0xE)) {
                    func_8006FB90(
                        team + 0xC,
                        idx - H(D_800F1068, 0xC),
                        D_800CA300[D_8010C1D0] << 4
                    );
                    idx++;
                }
            }
        }
    }

    status &= 0xFF;
    if ((func_8006FDB4(status, D_800FF748 + 0xABDA) & 0xFF) != 0) {
        B(D_800FF748, 0x6404) = 0;
        func_80072A2C();
        B(D_800FF748, 0xAC21) = 0;
        B(D_800FF748, 0xAC20) = 0;
    }
    return status;
}
