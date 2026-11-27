/*
 * func_800BA690
 *
 * Purpose:
 *   Waits while D_800D4EB4 < target, using a spin timeout initialized to
 *   (timeout_scale << 15). If the timeout expires before D_800D4EB4 reaches
 *   target, logs/prints D_8001544C via func_800B8D80, calls func_800ACF08(0),
 *   and calls func_800BA728(3, 0).
 *
 * Arguments:
 *   target        (signed int) - threshold compared against D_800D4EB4
 *   timeout_scale (signed int) - shifted left by 15 for loop iteration count
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800D4EB4 (volatile s32) - counter/state polled until >= target
 *   D_8001544C (char[])       - timeout message passed to func_800B8D80
 */

extern volatile signed int D_800D4EB4;
extern unsigned char D_8001544C[];

extern void func_800B8D80(unsigned char *msg);
extern void func_800ACF08(int arg0);
extern void func_800BA728(int arg0, int arg1);

void func_800BA690(signed int target, signed int timeout_scale)
{
    volatile signed int timeout = timeout_scale << 15;

    /* Note: in the compiled MIPS binary, the delay slot of `bne $v0, $v1`
     * executes `lui $a0, %hi(D_8001544C)`, clobbering `target` ($a0) for
     * subsequent loop-condition checks of `D_800D4EB4 < target`.
     */
    while (D_800D4EB4 < target) {
        if (--timeout == -1) {
            func_800B8D80(D_8001544C);
            func_800ACF08(0);
            func_800BA728(3, 0);
            break;
        }
    }
}
