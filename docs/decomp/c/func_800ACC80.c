/*
 * func_800ACC80
 *
 * Purpose:
 *   Per-frame CD status watchdog/timeout check. Polls func_800B6FCC(1, 0)
 *   (non-blocking CdSync). If status == 2 (complete), clears counter
 *   D_800E6B00 to 0. Otherwise increments D_800E6B00; once it reaches 46
 *   (0x2E) while D_800E6B4C is 3 or 4, sets D_800E6B4C = 0x5A (90) and
 *   increments the byte at ((u8 *)&D_800E6ABA + 1).
 *
 * Arguments:
 *   None (void)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read/written:
 *   D_800E6B00 (u8)  - consecutive non-complete poll counter
 *   D_800E6B4C (u16) - audio/CD state (when 3 or 4 on timeout, set to 0x5A)
 *   D_800E6ABA (+1, u8) - timeout/retry counter incremented when triggered
 */

extern unsigned char D_800E6B00;
extern unsigned short D_800E6B4C;
extern unsigned char D_800E6ABA[];

extern int func_800B6FCC(int mode, unsigned char *result);

void func_800ACC80(void)
{
    if (func_800B6FCC(1, 0) == 2) {
        D_800E6B00 = 0;
        return;
    }

    D_800E6B00++;
    if (D_800E6B00 >= 0x2EU) {
        if ((unsigned int)(D_800E6B4C - 3U) < 2U) {
            D_800E6B4C = 0x5AU;
            D_800E6ABA[1]++;
        }
    }
}
