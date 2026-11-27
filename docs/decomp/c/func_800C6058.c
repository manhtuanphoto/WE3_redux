/*
 * func_800C6058
 *
 * Purpose:
 *   PS1 BIOS B0 vector trampoline: B(4Fh) - read_card_sector(port, sector, dst).
 *   Loads $t2 = 0xB0, $t1 = 0x4F and tail-jumps to 0x000000B0 to start an
 *   asynchronous 128-byte sector read from a Memory Card.
 *
 * Arguments:
 *   port   (int): Memory Card port/channel ($a0)
 *   sector (int): 128-byte sector index ($a1)
 *   dst    (unsigned char *): Destination 128-byte buffer ($a2)
 *
 * Return value:
 *   (int) 1 if read command started successfully, 0 on failure ($v0).
 */

int func_800C6058(int port, int sector, unsigned char *dst)
{
    /* BIOS call: $t2 = 0xB0, $t1 = 0x4F (read_card_sector), jr $t2 */
    /* UNCLEAR: standard C cannot set $t1 = 0x4F and tail-jump to 0x000000B0 without inline asm. */
    (void)port;
    (void)sector;
    (void)dst;
    return 0;
}
