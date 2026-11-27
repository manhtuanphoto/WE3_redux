/*
 * func_8002A83C
 *
 * Purpose:
 *   Updates byte +0x01 of a slot/buffer returned by func_8001D004() with
 *   `min(val, 16) * 6` when actor+0x39A is 0. Determines the slot selector
 *   (0 -> arg 7, 1 -> arg 8) from func_8005B99C() if Scratchpad+0x2E2 == 2,
 *   otherwise from actor+0x399.
 *
 * Arguments:
 *   actor (unsigned char *) - player/actor struct (+0x399 u8 team/side, +0x39A u8 lock/skip flag)
 *   val   (unsigned int)    - value (low 8 bits clamped to max 0x10 = 16, scaled by 6)
 *
 * Return value:
 *   None (void)
 *
 * Actor / struct fields read:
 *   actor+0x399 (u8) team/side selector (0 or 1)
 *   actor+0x39A (u8) non-zero guard flag (skips update if != 0)
 *
 * Scratchpad (0x1F800000) fields read:
 *   +0x2E2 (u8) when == 2, overrides team/side selector via func_8005B99C()
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define Scratchpad ((unsigned char *)0x1F800000)

extern unsigned int func_8005B99C(void);
extern unsigned char *func_8001D004(unsigned int slot);

void func_8002A83C(unsigned char *actor, unsigned int val)
{
    unsigned int sel;
    unsigned char *dst = 0; /* UNCLEAR: left uninitialized in asm if sel != 0 && sel != 1 */
    unsigned int clamped;

    if (B(actor, 0x39A) != 0) {
        return;
    }

    if (B(Scratchpad, 0x2E2) == 2) {
        sel = func_8005B99C() & 0xFFU;
    } else {
        sel = B(actor, 0x399);
    }

    if (sel == 0U) {
        dst = func_8001D004(7U);
    } else if (sel == 1U) {
        dst = func_8001D004(8U);
    }

    clamped = val & 0xFFU;
    if (clamped >= 0x11U) {
        clamped = 0x10U;
    }

    B(dst, 0x01) = (unsigned char)(clamped * 6U);
}
