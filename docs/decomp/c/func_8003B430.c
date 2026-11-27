/*
 * func_8003B430
 *
 * Purpose:
 *   Scans predicted ball positions 2..8 frames ahead (0.067..0.267 s) to find an
 *   opportunistic first-time shot window: verifies path, alignment, and foot
 *   contact velocity, computes the actor's 3D interception velocity (+0x3CA/+3CC/+3CE),
 *   and if validated by func_8003CEE4, starts shot animation 0x20 and returns 1.
 *
 * Arguments:
 *   actor (u8*) - pointer to the 1000-byte player actor struct
 *
 * Return value:
 *   int - 1 if a first-time shot window was initiated, 0 otherwise
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x036 (s16[]) predicted ball x positions (frames 0..8+) (R)
 *     +0x04E (s16[]) predicted ball y heights (frames 0..8+) (R)
 *     +0x066 (s16[]) predicted ball z positions (frames 0..8+) (R)
 *     +0x0F0/+0x0F2/+0x0F4 (s16) projected contact x/y/z (R/W)
 *     +0x2F4 (u8*) pointer to ball actor (R)
 *   Actor:
 *     +0x002 (s16) x position (R)
 *     +0x006 (s16) z position (R)
 *     +0x026 (u16) model rotation ((0x1C0 - bodyHeading) % 256 * 16) (W)
 *     +0x392 (u8)  team slot 0..10 (R)
 *     +0x398 (u8)  side 0/1 (R)
 *     +0x3A0 (u32) speed (R)
 *     +0x3A4 (u8)  move heading (R)
 *     +0x3AA (u8)  body heading (R/W)
 *     +0x3AB (u8)  target/ball heading (W)
 *     +0x3AC (u8)  mirror/foot flag (R)
 *     +0x3B2 (u8)  shot aim heading (W)
 *     +0x3B4 (u8)  max speed (R)
 *     +0x3CA (s16) computed x step velocity (W/R)
 *     +0x3CC (s16) computed y step velocity (W/R)
 *     +0x3CE (s16) computed z step velocity (W/R)
 *     +0x3D1 (u8)  cleared to 0 (W)
 *   Ball (Scratchpad+0x2F4):
 *     +0x3A0 (u32) ball speed << 8 (R)
 *   Globals:
 *     D_8010C32F (u8[]) player stat table indexed by side*264 + slot*12 (low nibble) (R)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   frames 2..8 = 0.067..0.267 s lookahead
 *   0x90000 = height/clearance parameter to func_80052A68
 *   0x14400 = parameter to func_8003CEE4
 *   0x31 = 49/256 (~68.9 deg); 0x1D = 29/256 (~40.8 deg); 0x21 = 33/256 (~46.4 deg)
 *   0x10 = 16 raw/frame = 1.92 m/s minimum required horizontal step speed
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_8010C32F[];

extern unsigned int func_80052A68(unsigned char*, unsigned int, unsigned int);
extern unsigned int func_8003D4E8(unsigned char*, unsigned int);
extern unsigned int func_8005B36C(int, int, int, int);
extern void func_8005C2AC(unsigned char*, unsigned int, unsigned int, unsigned int, unsigned int);
extern unsigned int func_800B1128(int);
extern int func_8001E694(unsigned int, unsigned int);
extern int func_8001E5A4(unsigned int, unsigned int);
extern void func_8005C454(unsigned char*, int, int);
extern unsigned int func_8003CEE4(unsigned char*, unsigned int, int, unsigned int, unsigned int);
extern void func_8005BA30(unsigned char*, unsigned int, unsigned int);
extern unsigned int func_8005B7CC(unsigned int, unsigned int, unsigned int);

int func_8003B430(unsigned char* actor) {
    int i;

    for (i = 0; i < 7; i++) {
        unsigned int frameIdx = (unsigned int)(i + 2) & 0xFF;
        unsigned int aimAngle;
        unsigned int angleToBall;
        unsigned int bodyAngle;
        unsigned int footToBallAngle;
        unsigned int diff;
        int dx, dz;
        unsigned int reqSpeed;
        unsigned int framesToContact;
        unsigned int row;
        unsigned int stat;
        unsigned int moveAngle;
        unsigned int moveDiff2;
        unsigned int capSpeed;
        unsigned int maxStep;
        unsigned int dirByte;
        unsigned int speedFlag;
        unsigned int newHeading;
        int rotTurn;

        if ((func_80052A68(actor, frameIdx, 0x90000U) & 0xFF) != 0) {
            continue;
        }

        aimAngle = func_8003D4E8(actor, (unsigned int)(i + 3) & 0xFF);
        angleToBall = func_8005B36C(
            S(actor, 2), S(actor, 6),
            S(Scratchpad, 0x36 + frameIdx * 2), S(Scratchpad, 0x66 + frameIdx * 2)
        );

        bodyAngle = B(actor, 0x3AA);
        diff = (angleToBall - bodyAngle) & 0xFF;
        if ((diff < 0x81) ? (diff >= 0x31) : (((bodyAngle - angleToBall) & 0xFF) >= 0x31)) {
            continue;
        }

        diff = (aimAngle - angleToBall) & 0xFF;
        if ((diff < 0x81) ? (diff >= 0x1D) : (((angleToBall - aimAngle) & 0xFF) >= 0x1D)) {
            continue;
        }

        func_8005C2AC(actor, 0x20, 0, angleToBall & 0xFF, B(actor, 0x3AC));

        footToBallAngle = func_8005B36C(
            S(Scratchpad, 0xF0), S(Scratchpad, 0xF4),
            S(Scratchpad, 0x36 + frameIdx * 2), S(Scratchpad, 0x66 + frameIdx * 2)
        );

        diff = (footToBallAngle - angleToBall) & 0xFF;
        if ((diff < 0x81) ? (diff >= 0x21) : (((angleToBall - footToBallAngle) & 0xFF) >= 0x21)) {
            continue;
        }

        dx = S(Scratchpad, 0xF0) - S(Scratchpad, 0x36 + frameIdx * 2);
        dz = S(Scratchpad, 0xF4) - S(Scratchpad, 0x66 + frameIdx * 2);
        framesToContact = frameIdx + 1;
        reqSpeed = func_800B1128(dx * dx + dz * dz) / framesToContact;
        if (reqSpeed < 0x10U) {
            return 0;
        }

        row = (unsigned int)B(actor, 0x392) * 12 + (unsigned int)B(actor, 0x398) * 264;
        stat = B(D_8010C32F, row) & 0xF;
        moveAngle = B(actor, 0x3A4);
        diff = (moveAngle - angleToBall) & 0xFF;
        if (diff < 0x81) {
            moveDiff2 = diff << 1;
        } else {
            moveDiff2 = ((angleToBall - moveAngle) & 0xFF) << 1;
        }

        capSpeed = (stat + 0x10U) + ((unsigned int)((int)W(actor, 0x3A0) * (int)(0x100U - moveDiff2)) >> 8);
        maxStep = (unsigned int)B(actor, 0x3B4) + 0x10U;
        if (capSpeed >= maxStep) {
            capSpeed = maxStep;
        }
        if (reqSpeed >= capSpeed) {
            reqSpeed = capSpeed;
        }

        dirByte = footToBallAngle & 0xFF;
        S(actor, 0x3CA) = (short)func_8001E694(dirByte, reqSpeed);
        S(actor, 0x3CE) = (short)func_8001E5A4(dirByte, reqSpeed);

        S(actor, 0x3CC) = (short)((S(Scratchpad, 0x4E + frameIdx * 2) - S(Scratchpad, 0xF2)) / (int)framesToContact);

        row = (unsigned int)B(actor, 0x392) * 12 + (unsigned int)B(actor, 0x398) * 264;
        stat = B(D_8010C32F, row) & 0xF;
        func_8005C454(actor + 0x3CC, 0x14, -8 - ((int)stat >> 1));

        S(Scratchpad, 0xF0) += (short)(S(actor, 0x3CA) * (int)framesToContact);
        S(Scratchpad, 0xF2) += (short)(S(actor, 0x3CC) * (int)framesToContact);
        S(Scratchpad, 0xF4) += (short)(S(actor, 0x3CE) * (int)framesToContact);

        speedFlag = 0;
        if (i == 6) {
            speedFlag = (W(actor, 0x3A0) < (W(W(Scratchpad, 0x2F4), 0x3A0) >> 8)) ? 1U : 0U;
        }

        if ((func_8003CEE4(actor, frameIdx, 1, speedFlag, 0x14400U) & 0xFF) != 0) {
            func_8005BA30(actor, 0x20, 0);
            B(actor, 0x3D1) = 0;
            newHeading = func_8005B7CC(dirByte, B(actor, 0x3AA), 12);
            B(actor, 0x3AA) = (unsigned char)newHeading;
            rotTurn = 0x1C0 - (int)(newHeading & 0xFF);
            H(actor, 0x26) = (unsigned short)((rotTurn % 256) * 16);
            B(actor, 0x3AB) = (unsigned char)angleToBall;
            B(actor, 0x3B2) = (unsigned char)aimAngle;
            return 1;
        }
    }

    return 0;
}
