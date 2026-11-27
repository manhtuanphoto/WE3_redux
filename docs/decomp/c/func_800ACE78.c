/*
 * func_800ACE78 (and adjacent BIOS B0 syscall stubs 0x800ACE84..0x800ACED8)
 *
 * Purpose:
 *   Table of 9 packed PS1 BIOS B0 vector trampolines (12 bytes each). Each stub
 *   sets $t1 to the BIOS B0 function index and jumps directly to 0x000000B0.
 *   Because asm_compact packs unlabelled contiguous stubs under one label, all
 *   9 entries appear inside func_800ACE78.s:
 *     - 0x800ACE78 (func_800ACE78): B0:0x32 (open)
 *     - 0x800ACE84: B0:0x33 (lseek)
 *     - 0x800ACE90: B0:0x34 (read)
 *     - 0x800ACE9C: B0:0x35 (write)
 *     - 0x800ACEA8: B0:0x36 (close)
 *     - 0x800ACEB4: B0:0x42 (firstfile)
 *     - 0x800ACEC0: B0:0x43 (nextfile)
 *     - 0x800ACECC: B0:0x45 (erase)
 *     - 0x800ACED8: B0:0x51 (/* UNCLEAR: PS1 BIOS B0:0x51 function */)
 *
 * Arguments / Return value:
 *   Forwarded unchanged to the PS1 BIOS B0 handler in $a0..$a3 and $v0.
 */

#define BIOS_B0_STUB(fn_id) \
    ((int (*)(unsigned int, unsigned int, unsigned int, unsigned int))0x000000B0)

static int call_b0(unsigned int id, unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3)
{
    register unsigned int t1 __asm__("$9") = id;
    register unsigned int t2 __asm__("$10") = 0xB0;
    int ret;
    __asm__ volatile("jr %1" : "=v0"(ret) : "r"(t2), "r"(t1), "r"(a0), "r"(a1), "r"(a2), "r"(a3));
    return ret;
}

int func_800ACE78(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3)
{
    return call_b0(0x32, a0, a1, a2, a3); /* B0:0x32 open */
}

int func_800ACE84(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3)
{
    return call_b0(0x33, a0, a1, a2, a3); /* B0:0x33 lseek */
}

int func_800ACE90(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3)
{
    return call_b0(0x34, a0, a1, a2, a3); /* B0:0x34 read */
}

int func_800ACE9C(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3)
{
    return call_b0(0x35, a0, a1, a2, a3); /* B0:0x35 write */
}

int func_800ACEA8(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3)
{
    return call_b0(0x36, a0, a1, a2, a3); /* B0:0x36 close */
}

int func_800ACEB4(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3)
{
    return call_b0(0x42, a0, a1, a2, a3); /* B0:0x42 firstfile */
}

int func_800ACEC0(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3)
{
    return call_b0(0x43, a0, a1, a2, a3); /* B0:0x43 nextfile */
}

int func_800ACECC(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3)
{
    return call_b0(0x45, a0, a1, a2, a3); /* B0:0x45 erase */
}

int func_800ACED8(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3)
{
    /* UNCLEAR: PS1 BIOS B0:0x51 function */
    return call_b0(0x51, a0, a1, a2, a3);
}
