/*
 * func_800BA7C8 (and 3 trailing unlabelled dispatch stubs)
 *
 * Purpose:
 *   func_800BA7C8: Invokes the interrupt/DMA callback registration slot at
 *   *(D_800D4E74 + 0x14) with channel/index 4 and `arg0` (e.g. DMACallback(4, fn)
 *   for SPU DMA).
 *
 *   Three unlabelled trailing functions follow `jr $ra` inside func_800BA7C8.s,
 *   forwarding their arguments directly to vtable slots +0x14, +0x10, and +0x18
 *   of D_800D4E74 respectively.
 *
 * Arguments:
 *   func_800BA7C8: arg0 (void *) - callback/argument forwarded as 2nd parameter
 *   Trailing stubs: arg0..arg3 forwarded unchanged
 *
 * Return value:
 *   signed int: return value from the invoked D_800D4E74 function pointer
 *
 * Globals read:
 *   D_800D4E74 (u32, R): system/BIOS callback vtable pointer (+0x10, +0x14, +0x18)
 */

extern unsigned int D_800D4E74;

signed int func_800BA7C8(void *arg0)
{
    return (*(signed int (**)(signed int, void *))(D_800D4E74 + 0x14))(4, arg0);
}

/* UNCLEAR: unlabelled function #1 following jr $ra in func_800BA7C8.s */
signed int sub_800BA7C8_stub1(signed int arg0, signed int arg1)
{
    return (*(signed int (**)(signed int, signed int))(D_800D4E74 + 0x14))(arg0, arg1);
}

/* UNCLEAR: unlabelled function #2 following jr $ra in func_800BA7C8.s */
signed int sub_800BA7C8_stub2(signed int arg0, signed int arg1)
{
    return (*(signed int (**)(signed int, signed int))(D_800D4E74 + 0x10))(arg0, arg1);
}

/* UNCLEAR: unlabelled function #3 following jr $ra in func_800BA7C8.s */
signed int sub_800BA7C8_stub3(signed int arg0, signed int arg1)
{
    return (*(signed int (**)(signed int, signed int))(D_800D4E74 + 0x18))(arg0, arg1);
}
