/*
 * func_800B7B74
 *
 * Purpose:
 *   Waits for or polls the completion of the current PS1 CD-ROM command
 *   (equivalent to libcd `CdSync`). Sets up a timeout (`func_800BA518(-1) + 0x3C0`
 *   and `0x3C0000` iterations), services pending CD interrupts via
 *   func_800BA88C / func_800B7618 (invoking D_800D3950 data-ready callback on
 *   bit 2 and D_800D394C sync callback on bit 1 while preserving bank bits in
 *   *D_800D3C14), and returns when D_800D3C2C[0] reaches status 2 (`CdlComplete`)
 *   or 5 (`CdlDiskError`), resetting it to 2 and copying 8 result bytes from
 *   D_800E71C8 into `result` if non-NULL.
 *
 * Arguments:
 *   mode   (int)             - 0 to block until complete/timeout, non-zero to poll once
 *   result (unsigned char *) - optional 8-byte buffer to receive CD command result
 *
 * Return value:
 *   int - 2 (`CdlComplete`) or 5 (`CdlDiskError`) when finished, 0 if `mode != 0`
 *         and still in progress, or -1 on timeout.
 *
 * Global fields read/written:
 *   D_800D394C (void (*)(unsigned int, unsigned char *)) - sync callback
 *   D_800D3950 (void (*)(unsigned int, unsigned char *)) - ready callback
 *   D_800D396D (u8)     - current CD command ID index into D_800D3974
 *   D_800D3974 (u32[])  - CD command name table (for timeout log)
 *   D_800D39F4 (u32[])  - CD status name table (for timeout log)
 *   D_800D3C14 (u8 *)   - pointer to CD-ROM status/bank register (0x1F801800)
 *   D_800D3C2C (u8[2])  - current sync status [0] and ready status [1]
 *   D_800E71C8 (u8[8])  - sync result buffer
 *   D_800E71D0 (u8[8])  - ready result buffer
 *   D_800E71E0 (s32)    - timeout deadline
 *   D_800E71E4 (s32)    - poll iteration counter
 *   D_800E71E8 (char *) - timeout context string (D_80015350)
 */

extern void (*D_800D394C)(unsigned int status, unsigned char *res);
extern void (*D_800D3950)(unsigned int status, unsigned char *res);
extern unsigned char D_800D396D;
extern unsigned int D_800D3974[];
extern unsigned int D_800D39F4[];
extern volatile unsigned char *D_800D3C14;
extern unsigned char D_800D3C2C[2];
extern unsigned char D_800E71C8[8];
extern unsigned char D_800E71D0[8];
extern int D_800E71E0;
extern int D_800E71E4;
extern char *D_800E71E8;
extern char D_800152C8[];
extern char D_800152D8[];
extern char D_80015350[];

extern int func_800BA518(int arg);
extern void func_800B8D80(const char *msg);
extern void func_800AD698(const char *fmt, ...);
extern void func_800B8550(void);
extern int func_800BA88C(void);
extern int func_800B7618(void);

int func_800B7B74(int mode, unsigned char *result)
{
    D_800E71E0 = func_800BA518(-1) + 0x3C0;
    D_800E71E4 = 0;
    D_800E71E8 = D_80015350;

    for (;;) {
        int timed_out = 0;

        if (D_800E71E0 < func_800BA518(-1) || D_800E71E4++ > 0x3C0000) {
            func_800B8D80(D_800152C8);
            func_800AD698(D_800152D8, D_800E71E8,
                          D_800D3974[D_800D396D],
                          D_800D39F4[D_800D3C2C[0]],
                          D_800D39F4[D_800D3C2C[1]]);
            func_800B8550();
            timed_out = -1;
        }

        if (timed_out != 0) {
            return -1;
        }

        if (func_800BA88C() != 0) {
            unsigned char bank = *D_800D3C14 & 0x3;
            int ev;

            while ((ev = func_800B7618()) != 0) {
                if ((ev & 4) != 0 && D_800D3950 != 0) {
                    D_800D3950(D_800D3C2C[1], D_800E71D0);
                }
                if ((ev & 2) != 0 && D_800D394C != 0) {
                    D_800D394C(D_800D3C2C[0], D_800E71C8);
                }
            }

            *D_800D3C14 = bank;
        }

        {
            unsigned int status = D_800D3C2C[0];
            if (status == 2 || status == 5) {
                D_800D3C2C[0] = 2;
                if (result != 0) {
                    int i;
                    for (i = 0; i < 8; i++) {
                        result[i] = D_800E71C8[i];
                    }
                }
                return (int)status;
            }
        }

        if (mode != 0) {
            return 0;
        }
    }
}
