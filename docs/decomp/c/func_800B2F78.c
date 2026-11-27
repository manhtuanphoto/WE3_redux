/*
 * func_800B2F78
 *
 * Purpose:
 *   Entry wrapper that sets up arguments $a3 and $a2 and falls directly through
 *   into func_800B2F80 (recursive quad/polygon subdivision using GTE `rtpt`).
 *   Initializes $a3 = (unsigned char *)arg1 + 0x78 (work buffer) and $a2 = 0
 *   (initial recursion depth), then falls through without a jump/return.
 *
 * Arguments:
 *   arg0 (void *): Passed through in $a0 (returned by func_800B2F80)
 *   arg1 (void *): Passed through in $a1 (contains max depth at +0x0, screen limits at +0x4/+0x8, work area at +0x78)
 *
 * Return value:
 *   void *: Returns arg0 (from func_800B2F80)
 */

extern void *func_800B2F80(void *arg0, void *arg1, int depth, void *work_buf);

void *func_800B2F78(void *arg0, void *arg1)
{
    /* Assembly falls directly through into func_800B2F80 with $a2 = 0 and $a3 = (u8 *)arg1 + 0x78 */
    return func_800B2F80(arg0, arg1, 0, (void *)((unsigned char *)arg1 + 0x78));
}
