/*
 * func_800873DC
 *
 * Purpose:
 *   Updates the scoreboard/HUD goal-scorer and goal-minute display sprites for a
 *   side (up to 5 visible entries starting at startIdx). If the side has 0
 *   recorded events, clears its 3 HUD slots via func_8006FA84; otherwise writes
 *   scorer name texture UVs, converts stoppage-time boundary minutes (45/90/105/
 *   120 -> -1), formats 3-digit minute tiles, and terminates the sprite buffers
 *   with 0xFF.
 *
 * Arguments:
 *   side     (unsigned int) - team/side index (low byte used)
 *   startIdx (unsigned int) - starting event index in the side's event list (low byte used)
 *
 * Return value:
 *   void
 *
 * Reads / Writes:
 *   Globals:
 *     D_800F0FFA: u16 per-side event count (stride 18 bytes) (R)
 *     D_800FF748 + 0x88FD..0x88FF: per-side event entries (96 bytes per side,
 *       3 bytes per entry: +0/+1 scorer IDs or 0xFF, +2 goal minute) (R/W)
 *   UI buffers via func_8001D004:
 *     side + 0x94 (24-byte stride per visible slot, 12-byte tail terminator) (W)
 *     side + 0x96 (48-byte stride per visible slot, 12-byte tail terminator) (W)
 *     side + 0x9C (12-byte tail terminator) (W)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x2D, 0x5A, 0x69, 0x78 = 45, 90, 105, 120 (halftime/fulltime/ET boundary minutes)
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))

extern unsigned char D_800F0FFA[];
extern unsigned char D_800FF748[];

extern void func_8006FA84(int, int);
extern unsigned char* func_8001D004(int);

void func_800873DC(unsigned int side, unsigned int startIdx) {
    unsigned int sideByte = side & 0xFF;
    unsigned int count = H(D_800F0FFA, sideByte * 18);
    unsigned int visIdx;
    unsigned int curIdx;
    unsigned int baseIdx;
    unsigned int side96;
    unsigned int side128;
    unsigned short slotId94;
    unsigned short slotId96;
    unsigned int limit;
    unsigned int termIdx;
    unsigned char* buf;

    if (count == 0) {
        func_8006FA84(sideByte + 9, 0);
        func_8006FA84(sideByte + 0xB, 0);
        func_8006FA84(sideByte + 0xE, 0);
        return;
    }

    visIdx = 0;
    baseIdx = startIdx & 0xFF;
    curIdx = startIdx;
    side96 = sideByte * 96;
    side128 = sideByte << 7;
    slotId94 = (unsigned short)(sideByte + 0x94);
    slotId96 = (unsigned short)(sideByte + 0x96);

    limit = (count < 5) ? (baseIdx + count) : (baseIdx + 5);
    while ((curIdx & 0xFF) < limit) {
        unsigned int entryOff;
        unsigned char* entryPtr;
        unsigned int id0;
        unsigned int id1;
        unsigned char minute;
        unsigned char* slot24;
        unsigned char* slot48;
        unsigned int hundreds;
        unsigned int ones;
        unsigned int tens;

        buf = func_8001D004(slotId94);
        entryOff = side96 + (curIdx & 0xFF) * 3;
        entryPtr = D_800FF748 + entryOff;
        id0 = B(entryPtr, 0x88FD);
        slot24 = buf + (visIdx & 0xFF) * 24;

        if (id0 == 0xFF) {
            B(slot24, 0x1C) = 0xF0;
            B(slot24, 0x27) = 0x80;
            B(slot24, 0x1B) = 0;
            B(slot24, 0x28) = 0xD0;
        } else {
            B(slot24, 0x1B) = (unsigned char)(((id0 / 11u) << 6) + side128);
            B(slot24, 0x1C) = (unsigned char)((id0 % 11u) << 4);
            id1 = B(entryPtr, 0x88FE);
            if (id1 == 0xFF) {
                B(slot24, 0x27) = 0x80;
                B(slot24, 0x28) = 0xD0;
            } else {
                B(slot24, 0x27) = (unsigned char)(((id1 / 11u) << 6) + side128);
                B(slot24, 0x28) = (unsigned char)((id1 % 11u) << 4);
            }
        }

        minute = B(D_800FF748, side96 + (curIdx & 0xFF) * 3 + 0x88FF);
        if (minute == 0x2D || minute == 0x5A || minute == 0x69 || minute == 0x78) {
            B(D_800FF748, side96 + (curIdx & 0xFF) * 3 + 0x88FF) = (unsigned char)(minute - 1);
        }

        buf = func_8001D004(slotId96);
        minute = B(D_800FF748, side96 + (curIdx & 0xFF) * 3 + 0x88FF);
        hundreds = (unsigned int)minute / 100u;
        ones = (unsigned int)minute % 10u;
        tens = ((unsigned int)minute % 100u) / 10u;

        slot48 = buf + ((curIdx & 0xFF) - baseIdx) * 48;
        if (hundreds == 0) {
            B(slot48, 0x0A) = 0x5B;
        } else {
            B(slot48, 0x0A) = 0x9C;
            B(slot48, 0x03) = (unsigned char)(hundreds << 3);
        }

        B(slot48, 0x0F) = (unsigned char)((tens & 0xFF) << 3);
        B(slot48, 0x1B) = (unsigned char)((ones & 0xFF) << 3);

        curIdx++;
        visIdx++;

        count = H(D_800F0FFA, sideByte * 18);
        limit = (count < 5) ? (baseIdx + count) : (baseIdx + 5);
    }

    count = H(D_800F0FFA, sideByte * 18);
    if (count != 0) {
        termIdx = (count < 5) ? (count * 2 - 1) : 9;
        buf = func_8001D004(sideByte + 0x94);
        B(buf, (termIdx & 0xFF) * 12 + 0x18) = 0xFF;

        count = H(D_800F0FFA, sideByte * 18);
        termIdx = (count < 5) ? (count * 4 - 1) : 0x13;
        buf = func_8001D004(sideByte + 0x9C);
        B(buf, (termIdx & 0xFF) * 12) = 0xFF;

        count = H(D_800F0FFA, sideByte * 18);
        termIdx = (count < 5) ? (count * 4 - 1) : 0x13;
        buf = func_8001D004(sideByte + 0x96);
        B(buf, (termIdx & 0xFF) * 12) = 0xFF;
    }
}
