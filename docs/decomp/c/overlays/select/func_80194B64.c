/*
 * func_80194B64 (overlay: select)
 *
 * Purpose:
 *   Processes and renders up to 32 active text/label slots in D_801D8F90
 *   (24-byte stride). Supports two rendering paths based on entry->font_mode:
 *     - Tokenized / word-wrapped text mode (font_mode != 3 && font_mode != 0xC):
 *       Tokenizes the string into 8-byte token records at D_801D7DB8 (words,
 *       spaces, and control codes via jtbl_80189724), then performs line
 *       wrapping, optional space justification (when align_mode == 0), and
 *       line rendering via func_80197C78.
 *     - Direct 2-byte / Shift-JIS & icon glyph mode (font_mode == 3 or 0xC):
 *       Resets the Scratchpad sprite descriptor at 0x1F80015C, optionally
 *       centers each line (when align_mode == 3), renders special icon codes
 *       (0x817E..0x81A5 range) via func_801975B0 + func_800B3FB8, or renders
 *       up to 58 cached 12x16 glyphs (21-slot UV grid) with a 1px drop shadow
 *       via func_801962E4.
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 */

#define SCRATCHPAD_BASE 0x1F800000U

typedef struct {
    unsigned short ot_prio;     /* +0x00: ordering table priority / depth */
    short x;                    /* +0x02: base X coordinate */
    short y;                    /* +0x04: base Y coordinate */
    unsigned short max_width;   /* +0x06: max line width in pixels */
    unsigned char align_mode;   /* +0x08: 0 = justify, 3 = center */
    signed char char_spacing;   /* +0x09: extra horizontal spacing per char */
    signed char line_spacing;   /* +0x0A: extra vertical spacing per line */
    unsigned char font_mode;    /* +0x0B: 1/2/4 = tokenized, 3/0xC = direct 2-byte */
    unsigned char pad_0c[4];    /* +0x0C..+0x0F: unused here */
    unsigned char active;       /* +0x10: non-zero if slot is enabled */
    unsigned char pad_11[3];    /* +0x11..+0x13: padding */
    const unsigned char *text;  /* +0x14: pointer to string data */
} SelectTextSlot;               /* size = 0x18 (24 bytes) */

typedef struct {
    unsigned char type;         /* +0x00: token type (0=end, 1=space, 2=word, etc.) */
    unsigned char width;        /* +0x01: pixel width */
    unsigned char len;          /* +0x02: character count */
    unsigned char param;        /* +0x03: font_mode or control parameter */
    const unsigned char *ptr;   /* +0x04: pointer to word start (0 for control/space) */
} TextToken;                    /* size = 0x08 (8 bytes) */

typedef struct {
    unsigned int flags;         /* +0x00 (0x1F80015C) */
    short x;                    /* +0x04 (0x1F800160) */
    short y;                    /* +0x06 (0x1F800162) */
    unsigned short w;           /* +0x08 (0x1F800164) */
    unsigned short h;           /* +0x0A (0x1F800166) */
    unsigned short tpage;       /* +0x0C (0x1F800168) */
    unsigned char u;            /* +0x0E (0x1F80016A) */
    unsigned char v;            /* +0x0F (0x1F80016B) */
    unsigned short clut_x;      /* +0x10 (0x1F80016C) */
    unsigned short clut_y;      /* +0x12 (0x1F80016E) */
    unsigned char r;            /* +0x14 (0x1F800170) */
    unsigned char g;            /* +0x15 (0x1F800171) */
    unsigned char b;            /* +0x16 (0x1F800172) */
    unsigned char pad_17;       /* +0x17 (0x1F800173) */
    unsigned short unk_18;      /* +0x18 (0x1F800174) */
    unsigned short unk_1a;      /* +0x1A (0x1F800176) */
    unsigned short scale_x;     /* +0x1C (0x1F800178) */
    unsigned short scale_y;     /* +0x1E (0x1F80017A) */
    unsigned int unk_20;        /* +0x20 (0x1F80017C) */
} ScratchSpriteDesc;

extern SelectTextSlot D_801D8F90[32];
extern TextToken D_801D7DB8[];
extern unsigned char D_801D1B91[]; /* ASCII width table for font_mode == 1 (stride 2) */
extern unsigned char D_801D1BF9[]; /* ASCII width table for font_mode == 2 (stride 2) */
extern unsigned char D_800F1018[]; /* 20-byte stride OT/rendering table indexed by 0x1F80029D */
extern unsigned short D_8010786E;
extern unsigned short D_8010786C;

/* Jump tables located in .rodata (raw entries not present in compact .s) */
extern void *jtbl_80189724[];
extern void *jtbl_801897AC[];

extern void func_80197C78(SelectTextSlot *slot, int y, TextToken *tokens, int width, int count);
extern void func_801975B0(int icon_id, int x, int y, int arg3);
extern void func_800B3FB8(void *sprite_desc, void *ot_entry, unsigned int ot_prio);
extern void func_801962E4(void *sprite_desc, unsigned int ot_prio);

/* Helper matching the inline line-width measurement used for align_mode == 3 */
static int measure_centered_line_x(const unsigned char *p, SelectTextSlot *slot)
{
    unsigned int half_units = 0;
    unsigned int glyph_count = 0;
    unsigned char ch;
    int line_px;
    int offset;

    ch = *p;
    if (ch != '\n' && ch != '\0' && ch != '\\' && ch != '\t') {
        while (1) {
            if (*p == 0x0B) {
                break;
            }
            if (*p == ' ') {
                half_units += 1;
                p += 1;
            } else {
                half_units += 2;
                p += 2;
            }
            ch = *p;
            glyph_count += 1;
            if (ch == '\n' || ch == '\0' || ch == '\\' || ch == '\t') {
                break;
            }
        }
    }

    glyph_count -= 1;
    line_px = (int)((half_units & 0xFF) * 6) + (int)((glyph_count & 0xFF) * slot->char_spacing);
    offset = (short)(slot->max_width - line_px) >> 1;
    if (offset < 0) {
        offset = 0;
    }
    return (short)(slot->x + offset);
}

void func_80194B64(void)
{
    volatile ScratchSpriteDesc *spr = (volatile ScratchSpriteDesc *)(SCRATCHPAD_BASE + 0x15C);
    volatile unsigned char *scratch_29d = (volatile unsigned char *)(SCRATCHPAD_BASE + 0x29D);
    int slot_idx;

    for (slot_idx = 0; slot_idx < 32; slot_idx++) {
        SelectTextSlot *slot = &D_801D8F90[slot_idx];
        int cur_x;
        int cur_y;
        const unsigned char *src;
        unsigned char font_mode;
        unsigned int align_mode;

        if (slot->active == 0) {
            continue;
        }

        cur_x = slot->x;
        src = slot->text;
        cur_y = slot->y;
        font_mode = slot->font_mode;
        align_mode = slot->align_mode;

        if (font_mode != 3 && font_mode != 0xC) {
            /* ------------------------------------------------------------
             * Path 1: Tokenize string into D_801D7DB8[], wrap & render lines
             * ------------------------------------------------------------ */
            const unsigned char *p = src;
            TextToken *tok = D_801D7DB8;
            unsigned int override_mode = 0;
            int keep_going = 1;

            while (keep_going) {
                if (*p >= 0x21) {
                    /* Word run of characters >= 0x21 */
                    int word_width = 0;
                    int char_count = 0;
                    int char_width = 0;
                    const unsigned char *word_start = p;
                    unsigned int active_mode = (override_mode != 0) ? override_mode : slot->font_mode;

                    do {
                        unsigned int c = *p;

                        if (c == '_') {
                            char_width = ((active_mode & 0xFF) == 4) ? 6 : 8;
                            p++;
                        } else if (c < 0x80) {
                            unsigned int m = active_mode & 0xFF;
                            if (m == 1) {
                                unsigned int tbl_idx = (c < 0x61) ? (c - 0x41) : (c - 0x47);
                                char_width = D_801D1B91[tbl_idx * 2];
                                p++;
                            } else if (m == 2) {
                                char_width = D_801D1BF9[(c - 0x20) * 2];
                                p++;
                            } else {
                                char_width = 8;
                                p++;
                            }
                        } else {
                            if (c < 0x86) {
                                /* Single-byte special glyph token (width=16, len=1, mode=5) */
                                word_width = 0x10;
                                char_count = 1;
                                active_mode = 5;
                                p++;
                                break;
                            } else if ((active_mode & 0xFF) == 4) {
                                if (c == 0xB2 || c == 0xDB) {
                                    char_width = 0x10;
                                } else if (c == 0xCA) {
                                    char_width = 0x12;
                                } else if (c == 0xC6) {
                                    char_width = 6;
                                }
                                /* UNCLEAR: for other c >= 0x86 in mode 4, asm executes `bne $a0, 0xC6, .L80194D40`
                                 * with `j .L80194D40` in the delay slot, leaving char_width unmodified. */
                                p++;
                            } else if (c < 0xA0) {
                                if ((unsigned char)(c + 0x59) < 9) {
                                    char_width = 7;
                                } else if (c == 0xB0) {
                                    char_width = 4;
                                } else {
                                    char_width = 8;
                                }
                                p++;
                            } else {
                                active_mode = 2;
                                char_width = 0xA;
                                p++;
                            }
                        }

                        char_count++;
                        word_width += char_width + slot->char_spacing;
                    } while (*p >= 0x21);

                    override_mode = 0;
                    tok->type = 2;
                    tok->width = (unsigned char)word_width;
                    tok->len = (unsigned char)char_count;
                    tok->param = (unsigned char)active_mode;
                    tok->ptr = word_start;
                    tok++;
                } else {
                    /* Control or whitespace character (< 0x21) */
                    unsigned char c = *p;
                    tok->len = 1;
                    tok->ptr = 0;

                    /*
                     * UNCLEAR: Exact mapping of control byte `c` (0x00..0x20) via
                     * jtbl_80189724[c] is stored in .rodata and not included in the .s file.
                     * The distinct handler targets present in the assembly are:
                     *   - .L80194DD4/.L80194DD8 (space ' '): type=1, width=char_spacing+4, p++
                     *   - .L80194DF4: type=5, width=0, p++
                     *   - .L80194E04: p++, type=6, width=0, param=*p, p++
                     *   - .L80194E18: p++, type=8, width=0, param=*p, p++
                     *   - .L80194E2C (NUL '\0'): keep_going=0, type=0, width=0, p++
                     *   - .L80194E3C: type=3, width=char_spacing+0x12, p++, param=*p, p++
                     *   - .L80194E54: type=4, width=char_spacing+0x18, p++, param=*p, p++
                     *   - .L80194E6C: p++, override_mode=*p, p++ (no token emitted)
                     *   - .L80194E7C: p++, type=9, width=0x40, param=*p, p++
                     *   - .L80194E88: p++, type=0xA, width=0x40, param=*p, p++
                     *   - .L80194EB0: p++; sub=*p;
                     *       if (sub == 0) { type=0xFF; p++; tok++; }
                     *       else if (sub == 0x14 || sub == 0x15) { p++; type=sub; width=0x10; param=*p; tok++; p++; }
                     *       else { tok++; p++; }
                     */
                    (void)jtbl_80189724[c];
                    if (c == '\0') {
                        keep_going = 0;
                        tok->type = 0;
                        tok->width = 0;
                        tok++;
                        p++;
                    } else if (c == ' ') {
                        tok->type = 1;
                        tok->width = (unsigned char)(slot->char_spacing + 4);
                        tok++;
                        p++;
                    } else {
                        /* Fallback placeholder for remaining jtbl_80189724 control codes */
                        tok->type = 5;
                        tok->width = 0;
                        tok++;
                        p++;
                    }
                }
            }

            /* Line wrapping & justification pass over D_801D7DB8[] */
            {
                int line_w = 0;
                int line_cnt = 0;
                int line_start = 0;
                int tok_idx = 0;

                while (1) {
                    TextToken *cur_tok = &D_801D7DB8[tok_idx];
                    unsigned int t = cur_tok->type;

                    /*
                     * UNCLEAR: Exact mapping of token type `t` (0..0x15) via
                     * jtbl_801897AC[t] is stored in .rodata and not included in the .s file.
                     * The distinct handler targets present in the assembly are:
                     *   - .L80194F7C (word wrap before current token if line_w > max_width):
                     *       adds cur_tok->width to line_w; if overflow and line_cnt >= 2,
                     *       justifies D_801D7DB8[line_start .. line_start + line_cnt - 2],
                     *       calls func_80197C78(slot, cur_y, &D_801D7DB8[line_start], sum_w, line_cnt - 1),
                     *       sets line_cnt = 1, line_start = tok_idx, cur_y += 14 + line_spacing,
                     *       line_w = cur_tok->width; otherwise line_cnt++.
                     *   - .L80195098 (wrap including current token if line_w > max_width):
                     *       adds cur_tok->width to line_w; if overflow, justifies
                     *       D_801D7DB8[line_start .. line_start + line_cnt - 1], calls
                     *       func_80197C78(slot, cur_y, &D_801D7DB8[line_start], sum_w, line_cnt),
                     *       resets line_w = 0, line_cnt = 0, line_start = tok_idx + 1,
                     *       cur_y += 14 + line_spacing; otherwise line_cnt++.
                     *   - .L8019518C (explicit newline):
                     *       calls func_80197C78(slot, cur_y, &D_801D7DB8[line_start], line_w, line_cnt),
                     *       resets line_w = 0, line_cnt = 0, line_start = tok_idx + 1,
                     *       cur_y += 14 + line_spacing.
                     *   - .L801951C4 (custom vertical advance):
                     *       calls func_80197C78(slot, cur_y, &D_801D7DB8[line_start], line_w, line_cnt),
                     *       resets line_w = 0, line_cnt = 0, line_start = tok_idx + 1,
                     *       cur_y += (signed char)cur_tok->param.
                     *   - .L801951F8 (end of string, type == 0):
                     *       calls func_80197C78(slot, cur_y, &D_801D7DB8[line_start], line_w, line_cnt).
                     */
                    (void)jtbl_801897AC;

                    if (t == 0) {
                        func_80197C78(slot, cur_y, &D_801D7DB8[line_start], line_w, line_cnt);
                    } else if (t == 2) {
                        line_w += cur_tok->width;
                        if (line_w > (int)slot->max_width) {
                            if (line_cnt >= 2) {
                                int k;
                                int sum_w = 0;
                                for (k = 0; k < line_cnt - 1; k++) {
                                    sum_w += D_801D7DB8[line_start + k].width;
                                }
                                if (align_mode == 0) {
                                    int rem = (int)slot->max_width - sum_w;
                                    int sp_idx = 0;
                                    rem = (rem >= 0) ? (rem - 1) : -1;
                                    while (rem != -1) {
                                        TextToken *st = &D_801D7DB8[line_start + sp_idx];
                                        if (st->type == 1 && st->width < 10) {
                                            st->width++;
                                        }
                                        sp_idx++;
                                        if (sp_idx > line_cnt - 2) {
                                            sp_idx = 0;
                                        }
                                        rem--;
                                    }
                                }
                                func_80197C78(slot, cur_y, &D_801D7DB8[line_start], sum_w, line_cnt - 1);
                                line_cnt = 1;
                                line_start = tok_idx;
                                cur_y += 14 + slot->line_spacing;
                                line_w = cur_tok->width;
                            }
                        } else {
                            line_cnt++;
                        }
                    } else if (t == 1) {
                        line_w += cur_tok->width;
                        if (line_w > (int)slot->max_width) {
                            int k;
                            int sum_w = 0;
                            for (k = 0; k < line_cnt; k++) {
                                sum_w += D_801D7DB8[line_start + k].width;
                            }
                            if (align_mode == 0) {
                                int rem = (int)slot->max_width - sum_w;
                                int sp_idx = 0;
                                rem = (rem >= 0) ? (rem - 1) : -1;
                                while (rem != -1) {
                                    TextToken *st = &D_801D7DB8[line_start + sp_idx];
                                    if (st->type == 1 && st->width < 10) {
                                        st->width++;
                                    }
                                    sp_idx++;
                                    if (sp_idx > line_cnt - 1) {
                                        sp_idx = 0;
                                    }
                                    rem--;
                                }
                            }
                            func_80197C78(slot, cur_y, &D_801D7DB8[line_start], sum_w, line_cnt);
                            line_w = 0;
                            line_cnt = 0;
                            line_start = tok_idx + 1;
                            cur_y += 14 + slot->line_spacing;
                        } else {
                            line_cnt++;
                        }
                    } else if (t == 5) {
                        func_80197C78(slot, cur_y, &D_801D7DB8[line_start], line_w, line_cnt);
                        line_w = 0;
                        line_cnt = 0;
                        line_start = tok_idx + 1;
                        cur_y += 14 + slot->line_spacing;
                    }

                    if (cur_tok->type == 0) {
                        break;
                    }
                    tok_idx++;
                }
            }
        } else {
            /* ------------------------------------------------------------
             * Path 2: Direct 2-byte / Shift-JIS & icon glyph mode (3 or 0xC)
             * ------------------------------------------------------------ */
            int glyph_idx = 0;

            spr->unk_1a = 0;
            spr->unk_18 = 0;
            spr->scale_y = 0x1000;
            spr->scale_x = 0x1000;
            spr->unk_20 = 0;

            if (slot->font_mode == 0xC) {
                /* Skip prefix terminated by '\\' while counting 2-byte glyphs */
                while (1) {
                    unsigned char c = *src;
                    if (c == '\\') {
                        src++;
                        break;
                    }
                    if (c == '\t' || c == '\n' || c == ' ') {
                        src++;
                    } else {
                        src += 2;
                        glyph_idx++;
                    }
                }
            } else {
                if (*src == '\\') {
                    src++;
                }
            }

            if (slot->align_mode == 3) {
                cur_x = measure_centered_line_x(src, slot);
            }

            while (glyph_idx < 0x3A) {
                unsigned char c = *src;

                if (c == '\0' || c == '\\') {
                    break;
                }
                if (c == ' ') {
                    cur_x += 6;
                    src++;
                    continue;
                }
                if (c == '\t') {
                    /* Note: asm branches back without advancing src on '\t' */
                    continue;
                }

                if (c == '\n') {
                    src++;
                    cur_y += 0x10 + slot->line_spacing;
                    cur_x = (slot->align_mode == 3) ? measure_centered_line_x(src, slot) : slot->x;
                } else if (c == 0x0B) {
                    src++;
                    cur_y += 0x08 + slot->line_spacing;
                    cur_x = (slot->align_mode == 3) ? measure_centered_line_x(src, slot) : slot->x;
                } else {
                    unsigned int code = ((unsigned int)src[0] << 8) | src[1];

                    if ((unsigned int)(code - 0x817E) < 0x28U) {
                        /* Special button/symbol icon code */
                        int icon_id = 0;
                        unsigned int ot_idx;

                        if (code == 0x819B) {
                            icon_id = 0x80;
                        } else if (code == 0x817E) {
                            icon_id = 0x81;
                        } else if (code == 0x81A0) {
                            icon_id = 0x83;
                        } else if (code == 0x81A2) {
                            icon_id = 0x82;
                        }

                        func_801975B0(icon_id, cur_x, cur_y, 0);
                        cur_x += 0x10;
                        src += 2;
                        spr->r = 0x80;
                        spr->g = 0x80;
                        spr->b = 0x80;
                        ot_idx = *scratch_29d;
                        func_800B3FB8((void *)spr, &D_800F1018[ot_idx * 20], slot->ot_prio);
                    } else {
                        /* 12x16 cached glyph arranged in 21-column rows in VRAM */
                        int row = glyph_idx / 21;
                        int col = glyph_idx - (row * 21);

                        spr->tpage = 0x17;
                        spr->w = 0x0C;
                        spr->h = 0x10;
                        spr->x = (short)cur_x;
                        spr->y = (short)cur_y;
                        spr->flags = 0;
                        spr->r = 0x80;
                        spr->g = 0x78;
                        spr->b = 0x78;
                        spr->clut_x = D_8010786E;
                        spr->clut_y = D_8010786C;
                        spr->u = (unsigned char)(col * 12);
                        spr->v = (unsigned char)((row << 4) - 0x80);

                        func_801962E4((void *)spr, slot->ot_prio);

                        /* Draw 1px black drop shadow below */
                        spr->flags = 0;
                        spr->r = 0;
                        spr->g = 0;
                        spr->b = 0;
                        spr->y = (short)(spr->y + 1);
                        cur_x += 0x0C;
                        func_801962E4((void *)spr, slot->ot_prio);
                        spr->y = (short)(spr->y - 1);
                        src += 2;
                    }

                    cur_x += slot->char_spacing;
                    glyph_idx++;

                    if ((cur_x - slot->x + 0x0C) > (int)slot->max_width) {
                        cur_x = (slot->align_mode == 3) ? measure_centered_line_x(src, slot) : slot->x;
                        cur_y += 0x10 + slot->line_spacing;
                    }
                }
            }
        }
    }
}
