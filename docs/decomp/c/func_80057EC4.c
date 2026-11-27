/*
 * func_80057EC4 - Full-time / half-time referee whistle and commentary trigger.
 *
 * Purpose:
 *   Plays the whistle sound (0x10) and selects the appropriate commentary audio clip
 *   (and winning/leading team announcement via func_8005A1F0(..., 0xFD6)) at half-time
 *   or full-time based on match mode, score differential, and tournament/cup progression flags.
 *
 * Arguments:
 *   None.
 *
 * Return value:
 *   void.
 *
 * Scratchpad reads/writes:
 *   - 0x1F8002CE (u8, R): extra-time / single-leg flag (1 = compare current match score only).
 *   - 0x1F8002D1[2] (u8, R): 1st-leg / previous scores for side 0 and 1.
 *   - 0x1F8002DB[2] (u8, R): current match scores for side 0 and 1.
 *   - 0x1F8002DD[2] (u8, R): team IDs for side 0 and 1.
 *   - 0x1F8002E1 (u8, R): match/competition mode (4 = immediate exit, 2 = cup/tournament, 0 = exhibition).
 *   - 0x1F8002E2 (u8, R): two-leg / aggregate mode (1 or 4 selects leading side).
 *
 * Globals reads/writes:
 *   - D_800FF748 (u8[], R): competition/cup state table:
 *       +0x8F1F (u8): tournament/round flags (bit 6 = 0x40, bits 5..6 = 0x60, low nibble = 0xF).
 *       +0x8F22 (u8): upper nibble >> 4 == 3 check.
 *       +0x8F23 (u8): mode/stage guard.
 *       +0x8F26 + idx * 36 (u8): team/stage count (< 3 check).
 *       +0xABA0 (u8): team/group index for +0x8F26 lookup.
 *       +0xABA8 (u8): match-type/result flag (1 or 3).
 *       +0xABA9 (u8): round/stage status (1, 2, or 3).
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800FF748[];

extern void func_800AB620(int sfx);
extern int func_8001E6B8(int n);
extern int func_80087BF8(void);
extern void func_8005A1F0(int team_id, int clip);

void func_80057EC4(void)
{
    unsigned char *scratch = (unsigned char *)0x1F800000;
    unsigned char *tbl = D_800FF748;
    int lead_side = 0;
    int diff;
    int score0, score1;
    int mode;

    if (B(scratch, 0x2E1) == 4) {
        return;
    }

    func_800AB620(0x10);

    if (B(scratch, 0x2E2) == 1 || B(scratch, 0x2E2) == 4) {
        lead_side = (B(scratch, 0x2D1) + B(scratch, 0x2DB)) < (B(scratch, 0x2D2) + B(scratch, 0x2DC));
    }

    if (B(scratch, 0x2CE) == 1) {
        diff = B(scratch, 0x2DB + lead_side) - B(scratch, 0x2DB + (lead_side ^ 1));
    } else {
        diff = B(scratch, 0x2D1 + lead_side) - B(scratch, 0x2D1 + (lead_side ^ 1));
    }

    score0 = B(scratch, 0x2D1) + B(scratch, 0x2DB);
    score1 = B(scratch, 0x2D2) + B(scratch, 0x2DC);
    if (score0 == score1) {
        func_800AB620(func_8001E6B8(5) + 0xF28);
    } else {
        func_800AB620(func_8001E6B8(6) + 0xF27);
    }

    if ((func_80087BF8() & 0xFF) != 0 || B(scratch, 0x2CE) == 1) {
        if (diff < 0) {
            func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
            func_800AB620(func_8001E6B8(3) + 0xF37);
        } else if (diff == 0) {
            func_800AB620(func_8001E6B8(2) + 0xF3C);
        } else {
            func_800AB620(0xF32);
        }
        return;
    }

    score0 = B(scratch, 0x2D1) + B(scratch, 0x2DB);
    score1 = B(scratch, 0x2D2) + B(scratch, 0x2DC);
    if (score0 == score1) {
        func_800AB620(func_8001E6B8(2) + 0xF3C);
        return;
    }

    mode = B(scratch, 0x2E1);

    if (diff > 0) {
        if (diff >= 3) {
            if (mode == 2) {
                unsigned char flags = B(tbl, 0x8F1F);
                if ((flags & 0x40) == 0) {
                    if ((flags & 0xF) == 0 && B(tbl, 0xABA8) == 3 &&
                        B(tbl, 0x8F26 + B(tbl, 0xABA0) * 36) < 3) {
                        func_800AB620(0xF46);
                        func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                        func_800AB620(0xF47);
                        return;
                    }
                } else {
                    if ((flags & 0x60) != 0x40 ||
                        (B(tbl, 0x8F23) != 0 && B(tbl, 0x8F23) != mode)) {
                        if ((B(tbl, 0x8F22) >> 4) == 3) {
                            if ((flags & 0xF) == 0) {
                                func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                                func_800AB620(0xF63);
                                return;
                            }
                        } else {
                            unsigned char st = B(tbl, 0xABA9);
                            if (st == mode) {
                                func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                                func_800AB620(0xF54);
                                return;
                            } else if (st < 3) {
                                if (st == 1) {
                                    func_800AB620(0xF4F);
                                    return;
                                }
                            } else if (st == 3) {
                                func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                                func_800AB620(0xF5C);
                                return;
                            }
                        }
                    }
                }
            }
            func_800AB620(func_8001E6B8(4) + 0xF2D);
        } else if (diff == 1) {
            if (mode == 2) {
                unsigned char flags = B(tbl, 0x8F1F);
                if ((flags & 0x40) == 0) {
                    if ((flags & 0xF) == 0 && B(tbl, 0xABA8) == 3) {
                        func_800AB620(0xF49);
                        func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                        func_800AB620(0xF4A);
                        return;
                    }
                } else {
                    if ((flags & 0x60) != 0x40 ||
                        (B(tbl, 0x8F23) != 0 && B(tbl, 0x8F23) != mode)) {
                        if ((B(tbl, 0x8F22) >> 4) == 3) {
                            if ((flags & 0xF) == 0) {
                                func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                                func_800AB620(0xF64);
                                return;
                            }
                        } else {
                            unsigned char st = B(tbl, 0xABA9);
                            if (st == mode) {
                                func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                                func_800AB620(0xF55);
                                return;
                            } else if (st < 3) {
                                if (st == 1) {
                                    func_800AB620(0xF4F);
                                    return;
                                }
                            } else if (st == 3) {
                                func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                                func_800AB620(0xF5D);
                                return;
                            }
                        }
                    }
                }
            }
            func_800AB620(func_8001E6B8(3) + 0xF31);
        } else {
            /* diff == 2 */
            if (mode == 2) {
                unsigned char flags = B(tbl, 0x8F1F);
                if ((flags & 0x40) == 0) {
                    if ((flags & 0xF) == 0 && B(tbl, 0xABA8) == 3) {
                        func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                        func_800AB620(0xF48);
                    }
                } else {
                    if ((flags & 0x60) != 0x40 ||
                        (B(tbl, 0x8F23) != 0 && B(tbl, 0x8F23) != mode)) {
                        if ((B(tbl, 0x8F22) >> 4) == 3) {
                            func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                            if ((B(tbl, 0x8F1F) & 0xF) == 0) {
                                func_800AB620((func_8001E6B8(2) == 0) ? 0xF65 : 0xF62);
                            }
                        } else {
                            unsigned char st = B(tbl, 0xABA9);
                            if (st == mode) {
                                func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                                func_800AB620((func_8001E6B8(2) == 0) ? 0xF56 : 0xF53);
                            } else if (st < 3) {
                                if (st == 1) {
                                    if ((flags & 0xF) == 0) {
                                        func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                                        func_800AB620(func_8001E6B8(2) + 0xF4E);
                                    } else {
                                        func_800AB620(0xF4F);
                                    }
                                }
                            } else if (st == 3) {
                                func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                                func_800AB620((func_8001E6B8(2) == 0) ? 0xF5E : 0xF5B);
                            }
                        }
                    }
                }
            } else if (mode != 0) {
                func_800AB620(0xF69);
            } else if (func_8001E6B8(2) != 0) {
                func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                func_800AB620(func_8001E6B8(2) + 0xF3E);
            } else {
                func_800AB620(0xF69);
            }
        }
    } else {
        /* diff <= 0 */
        if (diff < -2) {
            if (mode == 2) {
                unsigned char flags = B(tbl, 0x8F1F);
                if ((flags & 0x40) != 0 &&
                    ((flags & 0x60) != 0x40 ||
                     (B(tbl, 0x8F23) != 0 && B(tbl, 0x8F23) != mode))) {
                    func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                    if ((B(tbl, 0x8F22) >> 4) == 3) {
                        func_800AB620(0xF68);
                        return;
                    } else {
                        unsigned char st = B(tbl, 0xABA9);
                        if (st == mode) {
                            func_800AB620(0xF58);
                            return;
                        } else if (st < 3) {
                            if (st == 1) {
                                func_800AB620(0xF52);
                                return;
                            }
                        } else if (st == 3) {
                            func_800AB620(0xF60);
                            return;
                        }
                    }
                }
            }
            func_800AB620(func_8001E6B8(2) + 0xF3A);
        } else if (diff == -1) {
            if (mode == 2) {
                unsigned char flags = B(tbl, 0x8F1F);
                if ((flags & 0x40) == 0) {
                    if ((flags & 0xF) == 0 && B(tbl, 0xABA8) == 1) {
                        func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                        func_800AB620(0xF4D);
                    }
                } else {
                    if ((flags & 0x60) != 0x40 ||
                        (B(tbl, 0x8F23) != 0 && B(tbl, 0x8F23) != mode)) {
                        func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                        if ((B(tbl, 0x8F22) >> 4) == 3) {
                            func_800AB620(0xF67);
                            return;
                        } else {
                            unsigned char st = B(tbl, 0xABA9);
                            if (st == mode) {
                                func_800AB620(0xF57);
                                return;
                            } else if (st < 3) {
                                if (st == 1) {
                                    func_800AB620(0xF51);
                                    return;
                                }
                            } else if (st == 3) {
                                func_800AB620(0xF61);
                                return;
                            }
                        }
                    }
                }
            }
            func_800AB620(func_8001E6B8(3) + 0xF34);
        } else {
            /* diff == -2 (or 0 with mismatched 2D1/2DB) */
            if (mode == 2) {
                unsigned char flags = B(tbl, 0x8F1F);
                if ((flags & 0x40) == 0) {
                    if ((flags & 0xF) == 0 && B(tbl, 0xABA8) == 1 &&
                        B(tbl, 0x8F26 + B(tbl, 0xABA0) * 36) < 3) {
                        func_800AB620(0xF4B);
                        func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                        func_800AB620(0xF4C);
                    }
                } else {
                    if ((flags & 0x60) != 0x40 ||
                        (B(tbl, 0x8F23) != 0 && B(tbl, 0x8F23) != mode)) {
                        func_8005A1F0(B(scratch, 0x2DD + lead_side), 0xFD6);
                        if ((B(tbl, 0x8F22) >> 4) == 3) {
                            func_800AB620(0xF66);
                        } else {
                            unsigned char st = B(tbl, 0xABA9);
                            if (st == mode) {
                                func_800AB620(func_8001E6B8(2) + 0xF59);
                            } else if (st < 3) {
                                if (st == 1) {
                                    func_800AB620(0xF50);
                                }
                            } else if (st == 3) {
                                func_800AB620(0xF5F);
                            }
                        }
                    }
                }
            } else {
                func_800AB620((func_8001E6B8(2) == 0) ? 0xF39 : 0xF37);
            }
        }
    }
}
