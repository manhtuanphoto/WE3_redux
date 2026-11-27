/*
 * func_8019963C (overlay: select)
 *
 * Purpose:
 *   Per-frame state-machine dispatcher for the match/tournament selection flow
 *   in the `select` overlay. Reads Scratchpad+0x29B (`*(u8 *)0x1F80029B`); if
 *   `< 0x40`, dispatches through `jtbl_80189BAC[state]` to handle fade-ins,
 *   sprite/menu setup, mode/cup selection input, team/match setup transitions,
 *   and match launch initialization.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 */

#define SCRATCHPAD_BASE 0x1F800000U

extern unsigned char D_800FF748[]; /* Base struct/buffer; offsets below are relative to D_800FF748 */
extern unsigned int D_8011D668[];
extern unsigned short D_8010A5A4;  /* Last polled button mask from pad 0 */
extern unsigned int D_801DA058;
extern unsigned int D_801DA05C;
extern unsigned int D_801DA060;
extern unsigned int D_801DA064;

extern void func_8019A168(void);
extern void func_800171DC(void);
extern void func_800171C4(int arg0);
extern void func_800171FC(int next_state);
extern int func_8018E994(void *obj, int target, int step);
extern int func_8019A2A0(void);
extern int func_8018E720(int arg0, int arg1);
extern void func_801942E0(int arg0, unsigned int text_ptr, int arg2, int arg3,
                          int arg4, int arg5, int arg6, int arg7,
                          int arg8, int arg9, int arg10, int arg11);
extern void func_80193FB4(int id, int arg1, int arg2, int arg3,
                          int frame, int h, int arg6, int sx,
                          int sy, int arg9, int arg10, int r,
                          int g, int b, int arg14, int arg15, int arg16);
extern void func_8006FA60(int slot, int val);
extern void func_8006FA84(int slot, int val);
extern void func_8006FB90(int id, int idx, int val);
extern void func_8006FBCC(int id, int idx, int val);
extern void func_8006FCFC(int clut_idx);
extern unsigned short func_8006EC80(int pad_mode);
extern void func_800AB620(int audio_cmd);
extern int func_8018E5EC(int cur, int min_val, int max_val);
extern void func_8018EC60(int slot, unsigned int text_ptr);
extern int func_8019A340(void);
extern unsigned int func_8001D004(int id);
extern void func_80199E8C(void);
extern void func_8018E4AC(int arg0, int arg1, int arg2);
extern int func_8019E620(void);
extern int func_8001E6B8(int max_val);
extern int func_801A6400(void);
extern int func_8006F0DC(unsigned int step, unsigned int mode);
extern void func_8018E538(int arg0);
extern void func_8005C49C(void);
extern void func_800501DC(int side);
extern void func_80198688(void);
extern void func_8002247C(void);
extern void func_800730F8(void);
extern void func_801C4DE8(void);
extern int func_8006F07C(unsigned int step);

void func_8019963C(void)
{
    volatile unsigned char *sp_1ec = (volatile unsigned char *)(SCRATCHPAD_BASE + 0x1EC);
    volatile unsigned char *sp_299 = (volatile unsigned char *)(SCRATCHPAD_BASE + 0x299);
    volatile unsigned char *sp_29b = (volatile unsigned char *)(SCRATCHPAD_BASE + 0x29B);
    volatile unsigned char *sp_2dd = (volatile unsigned char *)(SCRATCHPAD_BASE + 0x2DD);
    volatile unsigned char *sp_2de = (volatile unsigned char *)(SCRATCHPAD_BASE + 0x2DE);
    volatile unsigned char *sp_2e1 = (volatile unsigned char *)(SCRATCHPAD_BASE + 0x2E1);
    volatile unsigned char *sp_2e2 = (volatile unsigned char *)(SCRATCHPAD_BASE + 0x2E2);
    volatile unsigned char *sp_2e3 = (volatile unsigned char *)(SCRATCHPAD_BASE + 0x2E3);
    volatile unsigned char *sp_2e4 = (volatile unsigned char *)(SCRATCHPAD_BASE + 0x2E4);
    volatile unsigned char *sp_2e5 = (volatile unsigned char *)(SCRATCHPAD_BASE + 0x2E5);
    volatile unsigned char *sp_330 = (volatile unsigned char *)(SCRATCHPAD_BASE + 0x330);

    /* Globals accessed relative to D_800FF748 */
    volatile unsigned int *g_8010553C   = (volatile unsigned int *)(D_800FF748 + 0x5DF4);
    volatile unsigned short *g_80105550 = (volatile unsigned short *)(D_800FF748 + 0x5E08);
    volatile unsigned short *g_80105578 = (volatile unsigned short *)(D_800FF748 + 0x5E30);
    volatile unsigned short *g_80105592 = (volatile unsigned short *)(D_800FF748 + 0x5E4A);
    volatile unsigned short *g_801055BA = (volatile unsigned short *)(D_800FF748 + 0x5E72);
    volatile unsigned char *g_80108669  = (volatile unsigned char *)(D_800FF748 + 0x8F21); /* +0x10000 - 0x70DF */
    volatile unsigned short *g_8010A31E = (volatile unsigned short *)(D_800FF748 + 0xABD6); /* +0x10000 - 0x542A */
    volatile unsigned char *g_8010A320  = (volatile unsigned char *)(D_800FF748 + 0xABD8); /* +0x10000 - 0x5428 */
    volatile unsigned char *g_8010A322  = (volatile unsigned char *)(D_800FF748 + 0xABDA); /* +0x10000 - 0x5426 */
    volatile unsigned char *g_8010A368  = (volatile unsigned char *)(D_800FF748 + 0xAC20); /* +0x10000 - 0x53E0 */
    volatile unsigned char *g_8010A369  = (volatile unsigned char *)(D_800FF748 + 0xAC21); /* +0x10000 - 0x53DF */

    unsigned int state = *sp_29b;
    int ready = 1;

    if (state >= 0x40U) {
        return;
    }

    switch (state) {
    case 0: /* .L80199698: Initial entry check */
        if (*sp_2e1 == 5) {
            func_800171FC(0x14);
            *g_8010A322 = 0;
        } else {
            func_8019A168();
            func_800171DC();
        }
        break;

    case 1: /* .L801996C4: Fade-in / asset readiness wait */
        ready &= func_8018E994((void *)g_80105550, 0x1000, 0x100);
        ready &= func_8018E994((void *)g_80105578, 0x1000, 0x100);
        ready &= func_8019A2A0();
        ready &= func_8018E720(1, *sp_2e2 + 0x1D);
        if (ready != 0) {
            func_800171DC();
        }
        break;

    case 2: /* .L80199720: Setup mode banner and option sprites */
    {
        unsigned int m = *sp_2e2;
        func_801942E0(0, D_8011D668[m], -0xA2, 0x45, 0x147, 0, 1, 0, 0, 3, 2, 1);
        func_80193FB4(3, 0x301F, 0, 0, (int)(*sp_2e2 * 24), 0x18, 0,
                      0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 2, 1);
        func_80193FB4(4, 0x301E, 0, 0, (int)(*sp_2e2 * 24), 0x18, 0,
                      0x1000, 0x1000, 0, 0, 0x80, 0x80, 0x80, 0, 2, 1);
        func_8006FA60(*sp_2e2 + 0x13, 0x1F);
        D_801DA064 = 0;
        D_801DA060 = 0;
        D_801DA05C = 0;
        D_801DA058 = 0;
        func_800171DC();
        break;
    }

    case 3: /* .L80199870: Interactive menu animation and pad 0 input */
    {
        unsigned short btn;
        D_801DA05C++;
        if (D_801DA05C == 3) {
            D_801DA05C = 0;
            D_801DA058 = (D_801DA058 + 1) & 7;
        }

        func_8006FB90(4, 0, (((int)D_801DA058 >> 2) & 1) * 24 | 0x80);
        func_8006FB90(4, 1, (((int)(7 - D_801DA058) >> 2) & 1) * 24 | 0x80);
        func_8006FBCC(4, 0, (D_801DA058 & 3) * 24);
        func_8006FBCC(4, 1, ((7 - D_801DA058) & 3) * 24);
        func_8006FCFC(0x36);

        btn = func_8006EC80(0);
        D_8010A5A4 = btn;
        if (btn == 0x1000 || btn == 0x4000) {
            unsigned int next_m;
            func_8006FA60(*sp_2e2 + 0x13, 0x1E);
            func_800AB620(0x50D);
            next_m = (unsigned int)(func_8018E5EC(*sp_2e2, 0, 4) & 0xFF);
            *sp_2e2 = (unsigned char)next_m;
            func_8006FA60((int)next_m + 0x13, 0x1F);
            *g_80105592 = (unsigned short)(*sp_2e2 * 24);
            *g_801055BA = (unsigned short)(*sp_2e2 * 24);
            func_8018EC60(0, D_8011D668[*sp_2e2]);
        } else if (btn == 0x40 || btn == 0x20) {
            func_8006FA84(3, 0);
            func_8006FA84(4, 0);
            if (D_8010A5A4 == 0x20) {
                func_800AB620(0x528);
                func_800171FC(0xF);
            } else {
                func_800AB620(0x529);
                func_800171FC(0xA);
            }
            D_801DA058 = 0;
        }
        break;
    }

    case 10: /* .L80199A88: Fade-out and transition to state 3 */
        ready &= func_8018E994((void *)g_80105550, 0, 0x100);
        ready &= func_8018E994((void *)g_80105578, 0, 0x100);
        ready &= func_8019A340();
        if (ready != 0) {
            *g_8010553C = func_8001D004(0x3004);
            func_80199E8C();
            func_8018E4AC(0x13, 0x1D, 0);
            func_800171C4(1);
            func_800171FC(3);
        }
        break;

    case 15: /* .L80199AF8: Fade-out and transition to state 0x14 */
        ready &= func_8019A340();
        ready &= func_8018E720(2, *sp_2e2 + 0x1D);
        if (ready != 0) {
            func_8018E4AC(3, 0x35, 0);
            func_8018E4AC(0x13, 0x1D, 0);
            func_800171FC(0x14);
            *g_8010A322 = 0;
        }
        break;

    case 20: /* .L80199B44: Sub-screen func_8019E620() result handling */
    {
        int res = func_8019E620();
        if (res == 1) {
            if (*sp_2e1 == 5) {
                func_800171C4(1);
                func_800171FC(1);
                *sp_330 = 0;
                *g_8010A31E = 0;
            } else {
                func_800171FC(0);
            }
        } else if (res == 2) {
            if (*sp_2e1 == 5) {
                *sp_2e4 = 0;
                *sp_2e5 = 0;
                *sp_2e3 = 6;
                *sp_330 = 0;
                *g_8010A31E = 0;
                func_800171FC(0x32);
            } else if (*sp_2e1 == 4) {
                *sp_2e4 = (unsigned char)(func_8001E6B8(10) < 2);
                *sp_2e5 = (unsigned char)(func_8001E6B8(10) < 2);
                *sp_2e3 = (unsigned char)func_8001E6B8(6);
                *sp_330 = 0;
                *g_8010A31E = 0;
                func_800171FC(0x32);
            } else {
                func_800171FC(0x1E);
                *g_8010A322 = 0;
            }
        }
        break;
    }

    case 30: /* .L80199C3C: Sub-screen func_801A6400() result handling */
    {
        int res = func_801A6400();
        if (res == 1) {
            func_800171FC(0x14);
        } else if (res == 2) {
            func_800171FC(0x28);
        }
        break;
    }

    case 40: /* .L80199C84: Clear fade ramp and enter state 0x32 */
        *g_8010A31E = 0;
        func_800171FC(0x32);
        break;

    case 50: /* .L80199CA0: Final fade-out and match/squad initialization before gameplay */
        if (func_8006F0DC(0x10, 0) != 0) {
            func_8018E538(0);
            *sp_330 = 0;
            *g_8010A31E = 0;
            *g_80108669 = 0;

            if (*sp_2dd == 0x2B) {
                *sp_2dd = 2;
                *g_80108669 |= 2;
            }
            if (*sp_2de == 0x2B) {
                *sp_2de = 2;
                *g_80108669 |= 4;
            }

            *g_8010A322 = 0;
            *g_8010A320 = 0;
            *sp_1ec = 1;
            *g_8010A369 = 0;
            *g_8010A368 = 0;

            func_8005C49C();
            func_800501DC(0);
            func_800501DC(1);
            func_80198688();
            func_8002247C();

            if ((unsigned char)(*sp_2e1 - 1) < 2) {
                func_800730F8();
            }
            *sp_299 = 1;
        }
        break;

    case 60: /* .L80199DC4: Clear animation counters and advance */
        D_801DA064 = 0;
        D_801DA060 = 0;
        D_801DA05C = 0;
        D_801DA058 = 0;
        func_8018E538(0);
        func_800171DC();
        break;

    case 61: /* .L80199DFC: Load graphics/resources for current game mode */
        func_801C4DE8();
        *g_8010553C = func_8001D004(*sp_2e1 + 0x3005);
        *g_80105578 = 0x1000;
        *g_80105550 = 0x1000;
        func_800171DC();
        break;

    case 62: /* .L80199E2C: Fade-in step */
        if (func_8006F07C(0x40) != 0) {
            func_800171DC();
        }
        break;

    case 63: /* .L80199E4C: Transition to state 0x14 */
        func_800171FC(0x14);
        *g_8010A322 = 0;
        break;

    default: /* .L80199E64 */
        break;
    }
}
