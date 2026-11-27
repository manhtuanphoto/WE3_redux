/*
 * func_800AE110
 *
 * Purpose:
 *   Debug-prints fields of an 18-byte display/draw environment struct (`env`,
 *   PsyQ DumpDispEnv-like) by calling the function pointer `D_800CEAC0` (printf)
 *   four times with format strings `D_80014DDC`, `D_80014DF8`, `D_80014E14`,
 *   and `D_80014E20`.
 *
 * Arguments:
 *   env (unsigned char *) - struct containing:
 *     +0x00..+0x06 (4x s16) rect 1 (e.g. disp x, y, w, h)
 *     +0x08..+0x0E (4x s16) rect 2 (e.g. screen x, y, w, h)
 *     +0x10 (u8), +0x11 (u8) mode flags (e.g. isinter, isrgb24)
 *
 * Return value:
 *   None (void)
 *
 * Global fields read:
 *   D_800CEAC0 (printf-like function pointer)
 *   D_80014DDC, D_80014DF8, D_80014E14, D_80014E20 (format strings)
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define S(p, o) (*(signed short *)((p) + (o)))

typedef void (*debug_printf_fn_t)(const char *fmt, ...);

extern debug_printf_fn_t D_800CEAC0;
extern const char D_80014DDC[];
extern const char D_80014DF8[];
extern const char D_80014E14[];
extern const char D_80014E20[];

void func_800AE110(unsigned char *env)
{
    D_800CEAC0(D_80014DDC, S(env, 0x00), S(env, 0x02), S(env, 0x04), S(env, 0x06));
    D_800CEAC0(D_80014DF8, S(env, 0x08), S(env, 0x0A), S(env, 0x0C), S(env, 0x0E));
    D_800CEAC0(D_80014E14, B(env, 0x10));
    D_800CEAC0(D_80014E20, B(env, 0x11));
}
