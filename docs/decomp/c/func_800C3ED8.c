/*
 * func_800C3ED8
 *
 * Purpose:
 *   Checks or waits for SPU DMA transfer completion (PsyQ
 *   `SpuIsTransferCompleted(flag)` equivalent).
 *   - Returns 1 immediately if `D_800D511C == 1` or `D_800D55C0 == 1`.
 *   - Otherwise tests event `D_800D5114` via `func_800ACE48`.
 *   - If `flag == 1` (wait), polls `func_800ACE48(D_800D5114)` until non-zero,
 *     sets `D_800D55C0 = 1`, and returns 1.
 *   - If `flag != 1` (peek), sets `D_800D55C0 = 1` when `func_800ACE48` returns
 *     1, and returns the result of `func_800ACE48`.
 *
 * Arguments:
 *   flag (int) - 1 = blocking wait (SPU_TRANSFER_WAIT), else non-blocking poll
 *
 * Return value:
 *   (int) 1 if transfer completed, or result of `func_800ACE48` when polling.
 *
 * Global fields read/written:
 *   D_800D5114 (u32) SPU DMA completion event handle
 *   D_800D511C (u32) transfer mode / immediate-complete flag
 *   D_800D55C0 (u32) cached transfer-completed flag
 */

extern unsigned int D_800D5114;
extern unsigned int D_800D511C;
extern unsigned int D_800D55C0;

extern int func_800ACE48(unsigned int event);

int func_800C3ED8(int flag)
{
    int status;

    if (D_800D511C == 1U || D_800D55C0 == 1U) {
        return 1;
    }

    status = func_800ACE48(D_800D5114);
    if (flag == 1) {
        if (status == 0) {
            do {
                status = func_800ACE48(D_800D5114);
            } while (status == 0);
        }
        status = 1;
        D_800D55C0 = 1U;
    } else if (status == 1) {
        D_800D55C0 = 1U;
    }

    return status;
}
