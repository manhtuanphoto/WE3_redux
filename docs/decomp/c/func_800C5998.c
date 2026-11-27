/*
 * func_800C5998
 *
 * Purpose:
 *   PS1 BIOS B0 vector trampoline: B(4Eh) - write_card_sector(port, sector, src).
 *   Loads $t2 = 0xB0, $t1 = 0x4E and tail-jumps to 0x000000B0 to start an
 *   asynchronous 128-byte sector write to a Memory Card.
 *
 * Arguments:
 *   port   (int): Memory Card port/channel ($a0)
 *   sector (int): 128-byte sector index ($a1)
 *   src    (unsigned char *): Source 128-byte buffer ($a2)
 *
 * Return value:
 *   (int) 1 if write command started successfully, 0 on failure ($v0).
 */

int func_800C5998(int port, int sector, unsigned char *src)
{
    /* BIOS call: $t2 = 0xB0, $t1 = 0x4E (write_card_sector), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x4E and tail-jump to 0x000000B0 without inline asm. */
    (void)port;
    (void)sector;
    (void)src;
    return 0;
}
