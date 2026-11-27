/*
 * func_801C9110 (overlay: select)
 *
 * Purpose:
 *   Handles 2D grid cursor navigation and button-to-action mapping across a
 *   6-row (D_801D8DFE: 0..5) by 15-column (D_801D8DFD: 0..14) layout with two
 *   column-stepping modes selected by D_801DA5C8 (== 1 vs != 1).
 *   When (btn_mask & 0xF) == 0, diagonal D-pad inputs (0x3000, 0x6000, 0x9000,
 *   0xC000) are split into two steps via a silent recursive call for the first
 *   axis followed by processing the second axis with the caller's silent flag.
 *   Plays SFX 0x50D via func_800AB620 on D-pad cursor movement unless
 *   (silent & 0xFF) != 0.
 *
 * Arguments:
 *   btn_mask (unsigned int) - controller button bitmask (low 16 bits used)
 *   silent   (unsigned int) - if (silent & 0xFF) == 0, play cursor SFX 0x50D
 *                             on directional movement
 *
 * Return value:
 *   unsigned int - action code:
 *     0x00: D-pad navigation handled or unrecognized/no-op button
 *     0x01: Cross  (0x0040)
 *     0x02: Circle (0x0020)
 *     0x03: Square (0x0080)
 *     0x04: Triangle (0x0010)
 *     0x10: 0x0001, 0x8004, or 0x8008
 *     0x11: 0x0002, 0x2004, or 0x2008
 *
 * Globals read / written:
 *   D_801DA5C8 (u8, R)   - layout/grid mode flag (1 = compact row width 10/11,
 *                          non-1 = wide layout with column 6/7 skip on rows 1..4)
 *   D_801D8DFD (u8, R/W) - current column index (0..14)
 *   D_801D8DFE (u8, R/W) - current row index (0..5)
 */

extern unsigned char D_801DA5C8;
extern unsigned char D_801D8DFD;
extern unsigned char D_801D8DFE;

extern void func_800AB620(int sfx_id);

unsigned int func_801C9110(unsigned int btn_mask, unsigned int silent)
{
    unsigned int btn = btn_mask;
    unsigned int row;
    unsigned int col;
    unsigned int next_col;

    /* Split diagonal D-pad combinations when low nibble is clear */
    if ((btn_mask & 0xFU) == 0U) {
        switch (btn_mask & 0xF000U) {
        case 0x3000U: /* Up + Right */
            func_801C9110(0x1000U, 1U);
            btn = 0x2000U;
            break;
        case 0x6000U: /* Down + Right */
            func_801C9110(0x2000U, 1U);
            btn = 0x4000U;
            break;
        case 0xC000U: /* Down + Left */
            func_801C9110(0x4000U, 1U);
            btn = 0x8000U;
            break;
        case 0x9000U: /* Up + Left */
            func_801C9110(0x8000U, 1U);
            btn = 0x1000U;
            break;
        default:
            break;
        }
    }

    btn &= 0xFFFFU;

    /* Non-D-pad buttons and combined shoulder/dpad codes behave identically in both modes */
    switch (btn) {
    case 0x0040U: /* Cross */
        return 0x01U;
    case 0x0020U: /* Circle */
        return 0x02U;
    case 0x0080U: /* Square */
        return 0x03U;
    case 0x0010U: /* Triangle */
        return 0x04U;
    case 0x0001U:
    case 0x8004U:
    case 0x8008U:
        return 0x10U;
    case 0x0002U:
    case 0x2004U:
    case 0x2008U:
        return 0x11U;

    case 0x1000U: /* D-pad Up */
        row = D_801D8DFE;
        D_801D8DFE = (row != 0U) ? (unsigned char)(row - 1U) : 5U;
        if ((silent & 0xFFU) == 0U) {
            func_800AB620(0x50D);
        }
        return 0U;

    case 0x4000U: /* D-pad Down */
        row = D_801D8DFE;
        D_801D8DFE = (row == 5U) ? 0U : (unsigned char)(row + 1U);
        if ((silent & 0xFFU) == 0U) {
            func_800AB620(0x50D);
        }
        return 0U;

    case 0x8000U: /* D-pad Left */
        row = D_801D8DFE;
        col = D_801D8DFD;
        if (row == 0U) {
            if (col < 2U) {
                next_col = 0x0EU;
            } else if (col < 4U) {
                next_col = 0U;
            } else if (col < 8U) {
                next_col = 2U;
            } else if (col < 0x0CU) {
                next_col = 4U;
            } else {
                next_col = 8U;
            }
        } else if (col == 0U) {
            next_col = 0x0EU;
        } else if (D_801DA5C8 == 1U) {
            if (row == 5U) {
                next_col = (col < 0x0BU) ? (col - 1U) : 0x0AU;
            } else {
                next_col = (col < 0x0DU) ? (col - 1U) : 0x0BU;
            }
        } else {
            if (row == 5U) {
                next_col = (col < 0x0DU) ? (col - 1U) : 0x0CU;
            } else if ((col - 7U) < 2U) {
                /* Columns 7 and 8 step back by 2 (5 and 6 respectively) */
                next_col = col - 2U;
            } else {
                next_col = col - 1U;
            }
        }
        D_801D8DFD = (unsigned char)next_col;
        if ((silent & 0xFFU) == 0U) {
            func_800AB620(0x50D);
        }
        return 0U;

    case 0x2000U: /* D-pad Right */
        row = D_801D8DFE;
        col = D_801D8DFD;
        if (row == 0U) {
            if (col < 2U) {
                next_col = 2U;
            } else if (col < 4U) {
                next_col = 4U;
            } else if (col < 8U) {
                next_col = 8U;
            } else if (col < 0x0CU) {
                next_col = 0x0CU;
            } else {
                next_col = 0U;
            }
        } else if (col == 0x0EU) {
            next_col = 0U;
        } else if (D_801DA5C8 == 1U) {
            if (row == 5U) {
                next_col = (col < 0x0BU) ? (col + 1U) : 0U;
            } else {
                next_col = (col < 0x0CU) ? (col + 1U) : 0U;
            }
        } else {
            if (row == 5U) {
                next_col = (col < 0x0DU) ? (col + 1U) : 0U;
            } else if (col == 6U) {
                /* Column 6 skips forward over column 7 to column 8 */
                next_col = col + 2U;
            } else {
                next_col = col + 1U;
            }
        }
        D_801D8DFD = (unsigned char)next_col;
        if ((silent & 0xFFU) == 0U) {
            func_800AB620(0x50D);
        }
        return 0U;

    default:
        return 0U;
    }
}
