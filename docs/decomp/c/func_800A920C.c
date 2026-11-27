/*
 * func_800A920C
 *
 * Purpose:
 *   Configures CD-ROM mode (command 0x0E, mode 3) and seeks/plays either a
 *   table-indexed CD-DA location (arg0 != -1, using D_800CDDD4 / D_800CE9D4)
 *   or the saved 3-byte MSF location at D_800E6AFD..D_800E6AFF (arg0 == -1).
 *
 * Arguments:
 *   trackOrIdx: (int) table entry index, or -1 to resume from D_800E6AFD..AFF
 *
 * Return value:
 *   (int) always 0
 *
 * Reads / Writes:
 *   D_800CDDD4: table of 6-byte CD location entries indexed by trackOrIdx (R)
 *   D_800CE9D4: table of 6-byte entries indexed by (trackOrIdx - 0x200) (R)
 *   D_800E6AE0..D_800E6AE2: (u8) active 3-byte CD MSF/time values (W)
 *   D_800E6AFD..D_800E6AFF: (u8) saved 3-byte CD location for -1 resume (R)
 *   D_800E6BAA: (u16) cleared to 0 (W)
 *   D_800E6BD9: (u8) cleared to 0 (W)
 */

extern unsigned char D_800CDDD4[];
extern unsigned char D_800CE9D4[];
extern unsigned char D_800E6AE0;
extern unsigned char D_800E6AE1;
extern unsigned char D_800E6AE2;
extern unsigned char D_800E6AFD;
extern unsigned char D_800E6AFE;
extern unsigned char D_800E6AFF;
extern unsigned short D_800E6BAA;
extern unsigned char D_800E6BD9;

extern void func_800A91E4(void);
extern void func_800B7020(int arg);
extern int func_800B72A4(int cmd, void *param, void *res);
extern void func_800B7450(int arg);
extern void func_800B9510(int arg);
extern void func_800BA738(unsigned char arg);

int func_800A920C(int trackOrIdx) {
    unsigned char mode;
    unsigned char loc[3];
    int ok;

    D_800E6BD9 = 0;
    D_800E6BAA = 0;
    mode = 3;

    do {
        ok = func_800B72A4(0x0E, &mode, 0);
    } while (ok == 0);

    if (trackOrIdx != -1) {
        int subOff = (trackOrIdx - 0x200) * 6;
        int locOff = trackOrIdx * 6;

        D_800E6AE0 = D_800CE9D4[subOff + 0];
        D_800E6AE1 = D_800CE9D4[subOff + 1];
        D_800E6AE2 = D_800CE9D4[subOff + 2];

        do {
            ok = func_800B72A4(0x02, &D_800CDDD4[locOff], 0);
            func_800B72A4(0x03, 0, 0);
        } while (ok == 0);
    } else {
        loc[0] = D_800E6AFD;
        loc[1] = D_800E6AFE;
        loc[2] = D_800E6AFF;

        func_800BA738(loc[2]);
        func_800B7450(0);
        func_800B9510(0);
        /* UNCLEAR: Assembly loads func_800A91E4 into $a0, then overwrites $a0
         * with 2 in the delay slot of `jal func_800B7020` (reused as the first
         * iteration setup for func_800B72A4(2, ...)).
         */
        (void)func_800A91E4;
        func_800B7020(2);

        do {
            ok = func_800B72A4(0x02, loc, 0);
            func_800B72A4(0x03, 0, 0);
        } while (ok == 0);
    }

    return 0;
}
