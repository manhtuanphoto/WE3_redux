/*
 * func_8019B898 (select overlay)
 *
 * Purpose:
 *   Main state machine for the select overlay's multi-step option/mode selection
 *   wizard (dispatched on D_8010A322, states 0..0x32 via jtbl_80189E7C).
 *   Initializes selection state, coordinates page/carousel transitions and fade
 *   checks, polls controller input for up to 5 setup stages (D_800AD638 = 0..4),
 *   plays menu SFX, and exits via func_800171FC(3) (confirm) or func_800171FC(1)
 *   (cancel) before resetting D_8010A322 to 0.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_8010A322 (== D_800FF748 + 0xABDA, u8): state machine phase (0..0x32)
 *   D_800AD638 (u16): current setup stage / page index (0..4)
 *   D_801DAE50 (u8): step direction (0=idle, 1=back/cancel, 2=forward/confirm)
 *   D_801D8C60 (s8/u8): carousel rotation step (+1, 0, or -1 / 0xFF)
 *   D_801D8DF4..D_801D8DFA (u8): selected option values across stages
 *   D_801D8C64..D_801D8C67 (u8): carousel slot IDs initialized to {0, 1, 2, 3}
 *   D_801DA2F4..D_801DA2F7 (u8): per-stage cursor indices
 *   D_801DA058, D_801DA05C, D_801DA060, D_801DA064 (u32): cleared to 0 on init
 *   D_8010A5A4 (u16): controller button mask from func_8006EC80(0)
 *   D_80108667 (== D_800FF748 + 0x8F1F, u8): mode/rules bitfield (low nibble checked)
 *   D_801D8FB8 (u8): cleared to 0 on exit transitions
 *   D_801055D4 (== D_800FF748 + 0x5E8C, u8): cleared to 0 on back-exit from stage 0
 *   D_80105CB4 (== D_800FF748 + 0x656C, u8): cleared to 0 on back from stage 4
 *   D_801D92BF, D_801D92DB (u8): cleared to 0 before func_800171FC()
 *   D_8011D698, D_8011D69C[], D_8011D6BC[], D_8011D6C8[], D_8011D6D0, D_8011D6D8,
 *   D_8011D6DC: UI/text descriptor pointers
 *
 * Scratchpad (0x1F800000) fields written:
 *   +0x2E2 (u8): cleared to 0 on init
 */

#define B(p, o)  (*(unsigned char *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned char D_800FF748[];
extern unsigned char D_8010A322; /* == D_800FF748 + 0xABDA */
extern unsigned char D_80108667; /* == D_800FF748 + 0x8F1F */
extern unsigned short D_800AD638;
extern unsigned short D_8010A5A4;

extern unsigned char D_801DAE50;
extern signed char D_801D8C60;
extern unsigned char D_801D8C64;
extern unsigned char D_801D8C65;
extern unsigned char D_801D8C66;
extern unsigned char D_801D8C67;
extern unsigned char D_801D8DF4;
extern unsigned char D_801D8DF5;
extern unsigned char D_801D8DF6;
extern unsigned char D_801D8DF7;
extern unsigned char D_801D8DF8;
extern unsigned char D_801D8DF9;
extern unsigned char D_801D8DFA;
extern unsigned char D_801D8FB8;
extern unsigned char D_801D92BF;
extern unsigned char D_801D92DB;
extern unsigned char D_801DA2F4[];
extern unsigned char D_801DA2F5;
extern unsigned char D_801DA2F6;
extern unsigned char D_801DA2F7;
extern unsigned int D_801DA058;
extern unsigned int D_801DA05C;
extern unsigned int D_801DA060;
extern unsigned int D_801DA064;

extern unsigned int D_8011D698;
extern unsigned int D_8011D69C[];
extern unsigned int D_8011D6BC[];
extern unsigned int D_8011D6C8[];
extern unsigned int D_8011D6D0;
extern unsigned int D_8011D6D8;
extern unsigned int D_8011D6DC;

extern void func_800171FC(unsigned int mode);
extern unsigned short func_8006EC80(unsigned int pad);
extern void func_8006FCFC(unsigned int id);
extern void func_800AB620(unsigned int sfx_id);
extern void func_8018E4AC(unsigned int a0, unsigned int a1, unsigned int a2);
extern unsigned int func_8018E5EC(unsigned int cur, unsigned int min_val, unsigned int max_val);
extern unsigned int func_8018E720(unsigned int a0, unsigned int a1);
extern void func_8018EC60(unsigned int a0, unsigned int desc);
extern void func_80193FB4(
    unsigned int obj_id, unsigned int sprite_id, unsigned int a2, unsigned int a3,
    int x, int y, int z, unsigned int scale_x, unsigned int scale_y,
    int off_x, int off_y, unsigned int r, unsigned int g, unsigned int b,
    unsigned int a14, unsigned int prio, unsigned int flag);
extern void func_8019C43C(void);
extern unsigned int func_8019C61C(void);
extern unsigned int func_8019C73C(void);
extern void func_8019C85C(void);
extern unsigned int func_8019CB54(void);
extern void func_8019CD0C(void);
extern void func_8019D23C(unsigned int stage);
extern void func_8019D75C(unsigned int stage);
extern void func_8019D904(unsigned int stage);
extern void func_8019DB3C(void);
extern void func_8019DDA8(unsigned int stage);
extern void func_8019E09C(void);
extern void func_8019E1D0(unsigned int idx);
extern void func_8019E25C(unsigned int idx);

void func_8019B898(void)
{
    unsigned int state = D_8010A322;
    unsigned int ready = 1;

    if (state >= 0x33) {
        return;
    }

    switch (state) {
    case 0: /* .L8019B8E4: Initial setup (state 0 -> 1) */
        D_800AD638 = 0;
        D_801DAE50 = 0;
        D_801D8C60 = 0;
        B(Scratchpad, 0x2E2) = 0;
        D_801D8DF4 = 0;
        D_801D8DF5 = 1;
        D_801D8DF6 = 1;
        D_801D8DF7 = 2;
        D_801D8DF8 = 1;
        D_801D8DF9 = 0;
        D_801D8DFA = 1;
        D_801D8C64 = 0;
        D_801D8C65 = 1;
        D_801D8C66 = 2;
        D_801D8C67 = 3;
        D_801DA2F4[0] = 0;
        D_801DA2F5 = 0;
        D_801DA2F6 = 0;
        D_801DA2F7 = 0;
        D_801DA064 = 0;
        D_801DA060 = 0;
        D_801DA05C = 0;
        D_801DA058 = 0;
        D_8010A322++;
        break;

    case 1: /* .L8019B9CC: Build initial UI/resources (state 1 -> 2) */
        func_8019C43C();
        D_8010A322++;
        break;

    case 2: /* .L8019B9DC: Wait for fade/init completion (state 2 -> 3) */
        ready &= func_8018E720(1, 0);
        ready &= func_8019C61C();
        if (ready != 0) {
            D_8010A322++;
        }
        break;

    case 3: /* .L8019BA04: Enter stage D_800AD638 (0..4) -> state 0x14 + D_800AD638 */
    {
        unsigned short stage = D_800AD638;

        if (stage != 4) {
            func_8018E4AC(0x26, 0x31, 0);
            stage = D_800AD638;
        }

        if (stage < 5) {
            switch (stage) {
            case 0: /* .L8019BA54 */
                func_8019E09C();
                func_8019E1D0(D_801DA2F4[D_800AD638]);
                func_8019E25C(D_801DA2F4[D_800AD638]);
                func_8018EC60(0, D_8011D698);
                break;

            case 1: /* .L8019BAA4 */
                func_8019D23C(D_800AD638);
                func_8019D75C(D_800AD638);
                func_8019D904(D_800AD638);
                func_8018EC60(0, D_8011D6BC[D_801DA2F4[D_800AD638]]);
                break;

            case 2: /* .L8019BB04 */
                if (D_801D8DF4 != 1) {
                    D_801DA2F4[D_800AD638] = 2;
                }
                func_8019D23C(D_800AD638);
                func_8019D75C(D_800AD638);
                func_8019D904(D_800AD638);
                if (D_801D8DF4 == 1) {
                    func_8019DDA8(D_800AD638);
                    func_8018EC60(0, D_8011D6C8[D_801DA2F4[D_800AD638]]);
                } else {
                    func_8018EC60(0, D_8011D6D0);
                }
                break;

            case 3: /* .L8019BBC0 */
                func_8019E25C(D_801DA2F4[D_800AD638]);
                func_8019DB3C();
                func_8018EC60(0, D_8011D6D8);
                break;

            case 4: /* .L8019BBFC */
                func_8019DB3C();
                func_8018EC60(0, D_8011D6DC);
                func_80193FB4(0x31, 0x3025, 0, 0xA, 0, 0x18, 0, 0x1000, 0x1000,
                             0, 0, 0x80, 0x80, 0x80, 0, 2, 1);
                break;
            }
        }

        D_8010A322 = (unsigned char)D_800AD638 + 0x14;
        break;
    }

    case 4: /* .L8019BC94: Dispatch transition based on D_801DAE50 (state 4 -> D_801DAE50 + 5) */
        D_8010A322 = D_801DAE50 + 5;
        break;

    case 7: /* .L8019BCB8: Forward/Confirm stage transition (inferred state 7 = D_801DAE50(2) + 5) */
    {
        unsigned short stage = D_800AD638;

        if (stage == 4) {
            D_801D8FB8 = 0;
            func_8018E4AC(0x26, 0x31, 0);
            D_8010A322 = 0x28;
        } else if (stage == 3) {
            D_800AD638 = stage + 1;
            D_8010A322 = 3;
        } else {
            D_801D8C60 = -1;
            D_8010A322 = 0xA;
        }
        break;
    }

    case 6: /* .L8019BD1C: Back/Cancel stage transition (inferred state 6 = D_801DAE50(1) + 5) */
    {
        unsigned short stage = D_800AD638;

        if (stage == 0) {
            D_801D8FB8 = 0;
            func_8018E4AC(0x26, 0x31, 0);
            B(D_800FF748, 0x5E8C) = 0; /* D_801055D4 = 0 */
            D_8010A322 = 0x32;
        } else if (stage == 4) {
            B(D_800FF748, 0x656C) = 0; /* D_80105CB4 = 0 */
            D_8010A322 = 3;
            D_800AD638 = D_800AD638 - 1;
        } else {
            D_801D8C60 = 1;
            D_8010A322 = 0xA;
        }
        break;
    }

    case 10: /* .L8019BDBC: Start carousel rotation (inferred state 0xA -> 0xB) */
        func_8018E4AC(0x1E, 0x25, 0);
        func_8019CD0C();
        D_8010A322++;
        break;

    case 11: /* .L8019BDFC: Animate carousel rotation until func_8019CB54() != 0 */
        if (func_8019CB54() == 0) {
            func_8019CD0C();
        } else {
            signed char dir = D_801D8C60;
            unsigned short stage = D_800AD638;

            if (dir < 0) {
                stage = stage + 1;
            } else if (dir > 0) {
                stage = stage - 1;
            }
            D_800AD638 = stage;

            if ((D_80108667 & 0xF) == 0 && stage == 2) {
                signed char step = D_801D8C60;
                D_801D8DF7 = 0x20;
                D_801D8DF8 = 0;
                D_801D8DF9 = 1;
                if (step < 0) {
                    D_800AD638 = 3;
                } else if (step == 0) {
                    D_800AD638 = 2;
                } else {
                    D_800AD638 = 1;
                }
            }

            D_801D8C60 = 0;
            func_8018E4AC(0x14, 0x1B, 0);
            func_8019C85C();
            D_8010A322 = 3;
        }
        break;

    case 5: /* .L8019BEE4: Return to state 3 */
        D_8010A322 = 3;
        break;

    case 0x14: /* .L8019BEFC: Stage 0 input handler (state 0x14) */
    {
        unsigned short btn;

        D_801DAE50 = 0;
        btn = func_8006EC80(0);
        D_8010A5A4 = btn;
        if (btn == 0x1000 || btn == 0x4000) {
            unsigned char sel;
            func_800AB620(0x50D);
            sel = (unsigned char)func_8018E5EC(D_801DA2F4[0], 0, 5);
            D_801DA2F4[0] = sel;
            func_8018EC60(0, D_8011D69C[sel]);
            func_8019E1D0(D_801DA2F4[0]);
            func_8019E25C(D_801DA2F4[0]);
        }

        btn = D_8010A5A4;
        if (btn == 0x20) {
            unsigned char val;
            func_800AB620(0x528);
            val = D_801DA2F4[0];
            D_801D8DF5 = 1;
            D_801D8DF6 = 1;
            D_801DAE50 = 2;
            D_8010A322 = 4;
            D_801D8DF4 = val;
            D_80108667 = val;
        } else if (btn == 0x40) {
            func_800AB620(0x529);
            D_801DAE50 = 1;
            D_8010A322 = 4;
        }
        break;
    }

    case 0x15: /* .L8019BFFC: Stage 1 & 2 input handler (inferred states 0x15, 0x16) */
    case 0x16:
    {
        unsigned short btn;

        D_801DAE50 = 0;
        btn = func_8006EC80(0);
        D_8010A5A4 = btn;

        if (btn == 0x1000 || btn == 0x4000) {
            unsigned short stage;

            if (D_800AD638 == 2 && D_801D8DF4 != 1) {
                func_800AB620(0x512);
                return;
            }

            func_800AB620(0x50D);
            stage = D_800AD638;
            if (stage == 1) {
                unsigned char sel = (unsigned char)func_8018E5EC(D_801DA2F5, 0, 1);
                D_801DA2F5 = sel;
                func_8018EC60(0, D_8011D6BC[sel]);
            } else if (stage == 2) {
                unsigned int max_opt = (D_801D8DF8 == 0) ? 1U : 2U;
                D_801DA2F6 = (unsigned char)func_8018E5EC(D_801DA2F6, 0, max_opt);
                func_8018EC60(0, D_8011D6C8[D_801DA2F6]);
            }
            func_8019D75C(D_800AD638);
        }

        btn = D_8010A5A4;
        if (btn == 0x8000 || btn == 0x2000) {
            func_800AB620(0x50D);
            func_8019DDA8(D_800AD638);
            func_8019D904(D_800AD638);
            btn = D_8010A5A4;
        }

        if (btn == 0x20) {
            func_800AB620(0x528);
            if (D_800AD638 == 1) {
                unsigned int sub = (unsigned int)D_801D8DF4 - 1U;
                if (sub < 5U) {
                    switch (sub) {
                    case 0: /* .L8019C1E8 */
                        D_801D8DF7 = 0x10;
                        D_801D8DF8 = 1;
                        D_801D8DF9 = 0;
                        break;
                    case 1: /* .L8019C210 */
                        D_801D8DF7 = 0x10;
                        D_801D8DF8 = 1;
                        D_801D8DF9 = 1;
                        break;
                    case 2: /* .L8019C220 */
                    case 4:
                        D_801D8DF7 = 5;
                        D_801D8DF8 = 1;
                        D_801D8DF9 = 1;
                        break;
                    case 3: /* .L8019C218 */
                        D_801D8DF7 = 8;
                        D_801D8DF8 = 1;
                        D_801D8DF9 = 1;
                        break;
                    }
                }
            } else {
                D_801D8DFA = 1;
            }
            D_801DAE50 = 2;
            D_8010A322 = 4;
        } else if (btn == 0x40) {
            func_800AB620(0x529);
            D_801DAE50 = 1;
            D_8010A322 = 4;
        }
        break;
    }

    case 0x17: /* .L8019C274: Stage 3 input handler (state 0x17) */
    {
        unsigned short btn;

        D_801DAE50 = 0;
        btn = func_8006EC80(0);
        D_8010A5A4 = btn;

        if (btn == 0x1000 || btn == 0x4000) {
            func_800AB620(0x50D);
            btn = D_8010A5A4;
            if (btn == 0x1000 || btn == 0x4000) {
                D_8010A5A4 = btn ^ 0x5000;
            }
            D_801D8DFA = (unsigned char)func_8018E5EC(D_801D8DFA, 1, D_801D8DF7);
            func_8019DB3C();
        } else if (btn == 0x20) {
            func_800AB620(0x528);
            D_801DAE50 = 2;
            D_8010A322 = 4;
        } else if (btn == 0x40) {
            func_800AB620(0x529);
            D_801DAE50 = 1;
            D_8010A322 = 4;
        }
        break;
    }

    case 0x18: /* .L8019C338: Stage 4 confirmation input handler (state 0x18) */
    {
        unsigned short btn;

        func_8006FCFC(0x36);
        btn = func_8006EC80(0);
        D_8010A5A4 = btn;
        if (btn == 0x20) {
            func_800AB620(0x528);
            D_801DAE50 = 2;
            D_8010A322 = 4;
        } else if (btn == 0x40) {
            func_800AB620(0x529);
            D_801DAE50 = 1;
            D_8010A322 = 4;
        }
        break;
    }

    case 0x28: /* .L8019C3A4: Confirm exit transition (state 0x28) */
        ready &= func_8018E720(2, 0x3A);
        ready &= func_8019C73C();
        if (ready != 0) {
            func_8018E4AC(0x1E, 0x31, 0);
            D_801D92BF = 0;
            D_801D92DB = 0;
            func_800171FC(3);
            D_8010A322 = 0;
        }
        break;

    case 0x32: /* .L8019C3D8: Cancel exit transition (state 0x32) */
        ready &= func_8019C73C();
        if (ready != 0) {
            func_8018E4AC(0x1E, 0x31, 0);
            D_801D92BF = 0;
            D_801D92DB = 0;
            func_800171FC(1);
            D_8010A322 = 0;
        }
        break;

    default: /* .L8019C420 */
        break;
    }
}
