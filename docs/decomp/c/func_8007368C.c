/*
 * func_8007368C - Substitution HUD overlay animation and update state machine.
 *
 * Purpose:
 *   Drives the 5-stage (states 0..4) substitution banner overlay in the match HUD:
 *   - State 0: Allocates 8 UI sprite/text slots (0x1A..0x21, 4 per team), binds
 *              team headers, and populates outgoing player names & jersey numbers.
 *   - State 1: Slides the substitution panels inward by 32 px/frame over 4 frames (4/30s = 0.133s).
 *   - State 2: Waits 33 frames (1.10s), flips out active UI elements, swaps textures/numbers
 *              to incoming players (D_8010A5D8), and changes icon flag to incoming (8).
 *   - State 3: Flips in active UI elements with the incoming players.
 *   - State 4: Waits 41 frames (1.37s), slides panels outward by 32 px/frame over 4 frames,
 *              then marks completion.
 *   On completion, frees UI slots 0x1A..0x21, clears substitution queues/counts, and returns 1.
 *
 * Arguments:
 *   None.
 *
 * Return value:
 *   int: 1 when the full substitution overlay sequence completes and cleans up, 0 otherwise.
 *
 * Scratchpad reads/writes:
 *   - 0x1F8002E0 (u8, R):   home/away UI side flip flag (XORed with team index).
 *   - 0x1F8002DD[2] (u8, R): team IDs/names for side 0 and 1.
 *   - 0x1F800333 (u8, W):   cleared to 0 on completion.
 *   - 0x1F800334[2] (u8, R/W): pending substitution count (0..3) per side; cleared to 0 on completion.
 *
 * Globals reads/writes:
 *   - D_8010C538 (u8, R/W): state machine phase (0..4).
 *   - D_8010C1D0 (u8, R/W): slide-in/slide-out frame counter (0..3).
 *   - D_800E6CC0 + 2 (u8, R/W): hold timer (33 frames = 1.10s in state 2; 41 frames = 1.37s in state 4).
 *   - D_800FF748 (u8[], R/W): UI panel table at +0x61D0 / +0x61F8 (stride 0x50, halfword +0x10 X pos).
 *   - D_800E7660[2][3] (u8, R/W): outgoing player slot indices (0..21) per side.
 *   - D_8010A5D8[2][3] (u8, R/W): incoming player slot indices (0..21) per side.
 *   - D_800EF788[2][22] (u8, R): jersey numbers (stride 4 bytes per player entry, 88 bytes per side).
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_8010C538;
extern unsigned char D_8010C1D0;
extern unsigned char D_800E6CC0[];
extern unsigned char D_800FF748[];
extern unsigned char D_800E7660[];
extern unsigned char D_8010A5D8[];
extern unsigned char D_800EF788[];

extern void func_8001CEE0(int slot, int res, int a2, int a3, int a4, int a5, int a6, int count);
extern unsigned char *func_8001D004(int id);
extern void func_800219F4(unsigned char *dst, int team_id, int a2);
extern void func_8005A100(int a0);
extern int func_8006F934(int slot, int a1, int a2, int speed);
extern int func_8006F87C(int slot, int a1, int a2, int speed);
extern void func_8006FA84(int slot, int a1);

int func_8007368C(void)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *base = D_800FF748;
    unsigned char all_ready = 1;
    unsigned char done = 0;
    unsigned int i, j;

    if (D_8010C538 < 5) {
        switch (D_8010C538) {
        case 0:
            /* Allocate 8 HUD slots (0x1A..0x21) across both sides */
            func_8001CEE0(0x1A, 0x7F, 0, -0x80, 0, 7,  0, B(scratch, 0x334 + B(scratch, 0x2E0)));
            func_8001CEE0(0x1B, 0x80, 0,  0x80, 0, 7,  0, B(scratch, 0x334 + (B(scratch, 0x2E0) ^ 1)));
            func_8001CEE0(0x1C, 0x7D, 0, -0x80, 0, 10, 0, B(scratch, 0x334 + B(scratch, 0x2E0)));
            func_8001CEE0(0x1D, 0x7E, 0,  0x80, 0, 10, 0, B(scratch, 0x334 + (B(scratch, 0x2E0) ^ 1)));
            func_8001CEE0(0x1E, 0x7B, 0, -0x80, 0, 6,  0, B(scratch, 0x334 + B(scratch, 0x2E0)));
            func_8001CEE0(0x1F, 0x7C, 0,  0x80, 0, 6,  0, B(scratch, 0x334 + (B(scratch, 0x2E0) ^ 1)));
            func_8001CEE0(0x20, 0x81, 0, -0x80, 0, 10, 0, B(scratch, 0x334 + B(scratch, 0x2E0)));
            func_8001CEE0(0x21, 0x82, 0,  0x80, 0, 10, 0, B(scratch, 0x334 + (B(scratch, 0x2E0) ^ 1)));

            for (i = 0; (i & 0xFF) < 2; i++) {
                unsigned int side = i & 0xFF;
                if (B(scratch, 0x334 + side) != 0) {
                    unsigned char *hdr = func_8001D004(((i ^ B(scratch, 0x2E0)) & 0xFF) + 0x7B);
                    unsigned char *icon_p;
                    unsigned char *out_list;
                    unsigned int team_num_off;

                    func_800219F4(hdr, B(scratch, 0x2DD + side), 0);
                    icon_p = func_8001D004(((i ^ B(scratch, 0x2E0)) & 0xFF) + 0x81);
                    out_list = D_800E7660 + side * 3;
                    team_num_off = side * 88; /* 22 players * 4 bytes */
                    B(icon_p, 4) = 0; /* outgoing icon */

                    for (j = 0; (j & 0xFF) < 3; j++) {
                        unsigned int slot = j & 0xFF;
                        if (slot < B(scratch, 0x334 + side)) {
                            unsigned char *name_p = func_8001D004(((i ^ B(scratch, 0x2E0)) & 0xFF) + 0x7F);
                            unsigned char *num_p;
                            unsigned char *icon_slot;
                            unsigned char player = B(out_list, slot);
                            unsigned char jersey;

                            /* Name atlas: 11 players per 64px column, 16px row height */
                            B(name_p + slot * 12, 3) = (unsigned char)(((player / 11) * 64) + (side * 128));
                            B(name_p + slot * 12, 0xA) = 8;
                            B(name_p + slot * 12, 4) = (unsigned char)((player % 11) * 16);

                            num_p = func_8001D004(((i ^ B(scratch, 0x2E0)) & 0xFF) + 0x7D);
                            jersey = B(D_800EF788, team_num_off + (unsigned int)player * 4);
                            B(num_p + slot * 24, 3) = (unsigned char)((jersey / 10) * 8);
                            B(num_p + slot * 24, 0xA) = 0x16;
                            B(num_p + slot * 24, 0x16) = 0x16;
                            B(num_p + slot * 24, 0xF) = (unsigned char)((jersey % 10) * 8);

                            icon_slot = func_8001D004(((i ^ B(scratch, 0x2E0)) & 0xFF) + 0x81);
                            B(icon_slot + slot * 12, 0x22) = 0x97;
                        } else {
                            /* Unused substitution row: hide sprites (0x5B) */
                            unsigned char *name_p = func_8001D004(((i ^ B(scratch, 0x2E0)) & 0xFF) + 0x7F);
                            unsigned char *num_p;
                            unsigned char *icon_slot;

                            B(name_p + slot * 12, 0xA) = 0x5B;
                            num_p = func_8001D004(((i ^ B(scratch, 0x2E0)) & 0xFF) + 0x7D);
                            B(num_p + slot * 24, 0xA) = 0x5B;
                            B(num_p + slot * 24, 0x16) = 0x5B;
                            icon_slot = func_8001D004(((i ^ B(scratch, 0x2E0)) & 0xFF) + 0x81);
                            B(icon_slot + slot * 12, 0x22) = 0x5B;
                        }
                    }
                }
            }
            D_8010C1D0 = 0;
            func_8005A100(1);
            D_8010C538 = D_8010C538 + 1;
            break;

        case 1:
            /* Slide panels inward over 4 frames (4/30s = 0.133s) */
            if (D_8010C1D0 < 4) {
                D_8010C1D0++;
                for (i = 0; (i & 0xFF) < 4; i++) {
                    unsigned int off = (i & 0xFF) * 0x50;
                    if (B(base + off, 0x61D0 + 4) != 0) {
                        H(base + off, 0x61D0 + 0x10) += 0x20;
                    }
                    if (B(base + off, 0x61F8 + 4) != 0) {
                        H(base + off, 0x61F8 + 0x10) -= 0x20;
                    }
                }
            } else {
                D_8010C1D0++;
                B(D_800E6CC0, 2) = 0;
                D_8010C538 = D_8010C538 + 1;
            }
            break;

        case 2:
            /* Hold 33 frames (1.10s), then flip out & swap to incoming players */
            if (B(D_800E6CC0, 2) < 0x21) {
                B(D_800E6CC0, 2)++;
                break;
            }
            B(D_800E6CC0, 2)++;
            for (i = 0; (i & 0xFF) < 8; i++) {
                unsigned int idx = i & 0xFF;
                if (B(scratch, 0x334 + (idx & 1)) != 0) {
                    unsigned char r = (unsigned char)func_8006F934(((i ^ B(scratch, 0x2E0)) & 0xFF) + 0x1A, 0x1000, 0, 0x600);
                    all_ready &= r;
                }
            }
            if ((all_ready & 0xFF) != 0) {
                for (i = 0; (i & 0xFF) < 2; i++) {
                    unsigned int side = i & 0xFF;
                    if (B(scratch, 0x334 + side) != 0) {
                        unsigned char *icon_p = func_8001D004(((i ^ B(scratch, 0x2E0)) & 0xFF) + 0x81);
                        unsigned char *in_list = D_8010A5D8 + side * 3;
                        unsigned int team_num_off = side * 88;
                        B(icon_p, 4) = 8; /* incoming icon */

                        for (j = 0; (j & 0xFF) < 3; j++) {
                            unsigned int slot = j & 0xFF;
                            if (slot < B(scratch, 0x334 + side)) {
                                unsigned char *name_p = func_8001D004(((i ^ B(scratch, 0x2E0)) & 0xFF) + 0x7F);
                                unsigned char *num_p;
                                unsigned char player = B(in_list, slot);
                                unsigned char jersey;

                                B(name_p + slot * 12, 3) = (unsigned char)(((player / 11) * 64) + (side * 128));
                                B(name_p + slot * 12, 4) = (unsigned char)((player % 11) * 16);

                                num_p = func_8001D004(((i ^ B(scratch, 0x2E0)) & 0xFF) + 0x7D);
                                jersey = B(D_800EF788, team_num_off + (unsigned int)player * 4);
                                B(num_p + slot * 24, 3) = (unsigned char)((jersey / 10) * 8);
                                B(num_p + slot * 24, 0xF) = (unsigned char)((jersey % 10) * 8);
                            }
                        }
                    }
                }
                D_8010C538 = D_8010C538 + 1;
            }
            break;

        case 3:
            /* Flip in incoming players */
            for (i = 0; (i & 0xFF) < 8; i++) {
                unsigned int idx = i & 0xFF;
                if (B(scratch, 0x334 + (idx & 1)) != 0) {
                    unsigned char r = (unsigned char)func_8006F87C(((i ^ B(scratch, 0x2E0)) & 0xFF) + 0x1A, 0x1000, 0x1000, 0x600);
                    all_ready &= r;
                }
            }
            if ((all_ready & 0xFF) != 0) {
                D_8010C1D0 = 0;
                B(D_800E6CC0, 2) = 0;
                D_8010C538 = D_8010C538 + 1;
            }
            break;

        case 4:
            /* Hold 41 frames (1.37s), then slide panels outward over 4 frames */
            if (B(D_800E6CC0, 2) < 0x29) {
                B(D_800E6CC0, 2)++;
                break;
            }
            B(D_800E6CC0, 2)++;
            if (D_8010C1D0 < 4) {
                D_8010C1D0++;
                for (i = 0; (i & 0xFF) < 4; i++) {
                    unsigned int off = (i & 0xFF) * 0x50;
                    if (B(base + off, 0x61D0 + 4) != 0) {
                        H(base + off, 0x61D0 + 0x10) -= 0x20;
                    }
                    if (B(base + off, 0x61F8 + 4) != 0) {
                        H(base + off, 0x61F8 + 0x10) += 0x20;
                    }
                }
            } else {
                D_8010C1D0++;
                done = 1;
            }
            break;
        }
    }

    if (done != 0) {
        for (i = 0; (i & 0xFF) < 8; i++) {
            func_8006FA84((i & 0xFF) + 0x1A, 0);
        }
        D_8010C538 = 0;
        B(scratch, 0x333) = 0;
        for (i = 0; (i & 0xFF) < 2; i++) {
            unsigned int side = i & 0xFF;
            B(scratch, 0x334 + side) = 0;
            for (j = 0; (j & 0xFF) < 3; j++) {
                D_8010A5D8[side * 3 + (j & 0xFF)] = 0;
                D_800E7660[side * 3 + (j & 0xFF)] = 0;
            }
        }
        B(D_800E6CC0, 2) = 0;
        return 1;
    }

    return 0;
}
