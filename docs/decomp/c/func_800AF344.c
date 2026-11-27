/*
 * func_800AF344
 *
 * Purpose:
 *   Builds a PS1 GPU DR_MODE primitive packet (PsyQ SetDrawMode equivalent).
 *   Sets the packet length byte (+0x03) to 2 words and encodes the GPU E1
 *   (texpage/dither/draw-mode) command word at +0x04 via func_800AF800 and the
 *   GPU E2 (texture window) command word at +0x08 via func_800AF96C.
 *
 * Arguments:
 *   prim  (unsigned char *): output DR_MODE GPU primitive buffer
 *   dfe   (unsigned int):    draw-to-display-area enable flag (passed as arg0 to func_800AF800)
 *   dtd   (unsigned int):    dither enable flag
 *   tpage (unsigned int):    16-bit texture page / mode attribute (masked with 0xFFFF)
 *   tw    (unsigned char *): pointer to texture window RECT (5th stack arg at +0x30)
 *
 * Return value:
 *   None (void)
 */

#define B(p, o) (*(unsigned char *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

extern unsigned int func_800AF800(unsigned int dfe, unsigned int dtd, unsigned short tpage);
extern unsigned int func_800AF96C(unsigned char *tw);

void func_800AF344(unsigned char *prim, unsigned int dfe, unsigned int dtd, unsigned int tpage, unsigned char *tw)
{
    B(prim, 0x03) = 2;
    W(prim, 0x04) = func_800AF800(dfe, dtd, (unsigned short)(tpage & 0xFFFFU));
    W(prim, 0x08) = func_800AF96C(tw);
}
