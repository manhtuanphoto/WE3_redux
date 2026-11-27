/*
 * func_800B5D58
 *
 * Purpose:
 *   Unpacks a primitive/geometry packet header from `src` into `dst`.
 *   Copies the first 32-bit word (`hdr`) to dst+0x00. If bit 3 (`(hdr >> 3) & 1`)
 *   is set, reads a word-aligned byte offset `(src[1] >> 2) << 2` to locate a
 *   secondary block, copies 4 halfwords from `src+8` into dst+0x10..+0x16 and
 *   saves the updated pointer to dst+0x18, then copies 4 halfwords from the
 *   secondary block (+4) into dst+0x04..+0x0A and saves its updated pointer to
 *   dst+0x0C. If bit 3 is clear, skips 2 words after `hdr` and copies 4 halfwords
 *   into dst+0x04..+0x0A, saving the updated pointer to dst+0x0C.
 *
 * Arguments:
 *   src (u8*) - input packet/descriptor stream
 *   dst (u8*) - output unpacked struct (at least 0x1C bytes when bit 3 is set)
 *
 * Return value:
 *   None (void)
 */

#define H(p, o) (*(unsigned short *)((p) + (o)))
#define W(p, o) (*(unsigned int *)((p) + (o)))

void func_800B5D58(unsigned char *src, unsigned char *dst)
{
    unsigned int hdr = W(src, 0x00);

    W(dst, 0x00) = hdr;
    src += 4;

    if (((hdr >> 3) & 1U) != 0) {
        unsigned int byteOff = (W(src, 0x00) >> 2) << 2;
        unsigned char *sec = src + byteOff;

        src += 4;
        H(dst, 0x10) = H(src, 0x00);
        H(dst, 0x12) = H(src, 0x02);
        src += 4;
        H(dst, 0x14) = H(src, 0x00);
        H(dst, 0x16) = H(src, 0x02);
        src += 4;
        W(dst, 0x18) = (unsigned int)src;

        sec += 4;
        H(dst, 0x04) = H(sec, 0x00);
        H(dst, 0x06) = H(sec, 0x02);
        sec += 4;
        H(dst, 0x08) = H(sec, 0x00);
        H(dst, 0x0A) = H(sec, 0x02);
        sec += 4;
        W(dst, 0x0C) = (unsigned int)sec;
    } else {
        src += 8;
        H(dst, 0x04) = H(src, 0x00);
        H(dst, 0x06) = H(src, 0x02);
        src += 4;
        H(dst, 0x08) = H(src, 0x00);
        H(dst, 0x0A) = H(src, 0x02);
        src += 4;
        W(dst, 0x0C) = (unsigned int)src;
    }
}
