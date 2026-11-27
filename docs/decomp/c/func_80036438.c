/*
 * func_80036438
 *
 * Purpose:
 *   Initiates a diving/intercept GK or defender save/lunge action (anim 0x2B)
 *   when the ball is not rising (ball vy >= 0). Computes the intercept heading
 *   via func_8003D4E8(actor, 9), picks left/right foot/side (+-90 deg = +-0x40)
 *   relative to the ball angle, calls func_8005C2AC(actor, 0x2B, ...), computes
 *   lunge XZ/Y velocity toward the predicted ball intercept point (Scratchpad
 *   +0x46/+0x52/+0x76 vs +0xF0/+0xF2/+0xF4), applies gravity/lift bias based on
 *   nearest-opponent pressure (func_800494C0) and player stat (+0x3B8), advances
 *   the predicted target by D_8001105C frames, and tests contact via
 *   func_8003CEE4(actor, 8, 1, 1, 0x14400). On success, starts anim 0x2B,
 *   updates headings/rotation, and returns 1.
 *
 * Arguments:
 *   actor: (unsigned char*) player/GK actor
 *
 * Return value:
 *   (int) 1 if the action/lunge was triggered, 0 otherwise
 *
 * Reads / Writes:
 *   Scratchpad (0x1F800000):
 *     +0x046 (s16) ball/target x at step (R)
 *     +0x052+ (s16[]) ball y trajectory table (R)
 *     +0x076 (s16) ball/target z at step (R)
 *     +0x0F0 (s16) predicted target x (R/W)
 *     +0x0F2 (s16) predicted target y (R/W)
 *     +0x0F4 (s16) predicted target z (R/W)
 *     +0x2F4 (u8*) pointer to ball actor (R)
 *   actor:
 *     +0x002 (s16) x position (R)
 *     +0x006 (s16) z position (R)
 *     +0x026 (s16) render rotation ((0x1C0 - bodyHeading) % 256 * 16) (W)
 *     +0x392 (u8)  team slot (R)
 *     +0x398 (u8)  side (0/1) (R)
 *     +0x3A4 (u8)  move heading (R/W)
 *     +0x3AA (u8)  body heading (R/W)
 *     +0x3AB (u8)  saved target heading (W)
 *     +0x3AC (u8)  mirror/foot side (W)
 *     +0x3B0 (u8)  saved body heading (W)
 *     +0x3B8 (u8)  stat*10 (R)
 *     +0x3CA (s16) lunge vx (W/R)
 *     +0x3CC (s16) lunge vy (W/R)
 *     +0x3CE (s16) lunge vz (W/R)
 *     +0x3D1 (u8)  (W = 0)
 *
 * Magic numbers (250 raw = 1 m, 30 frames/s):
 *   0x2B    = 43 animation ID
 *   0x40    = 64 angle units (90 deg)
 *   0xC3FF  = 50175 = ~224^2 raw^2 (~0.90 m) opponent pressure threshold
 *   0x14400 = 82944 = 288^2 raw^2 (1.152 m) check radius in func_8003CEE4
 *   6       = frame divisor for lunge XZ speed
 */

#define B(p,o)  (*(unsigned char*)((p)+(o)))
#define H(p,o)  (*(unsigned short*)((p)+(o)))
#define S(p,o)  (*(signed short*)((p)+(o)))
#define W(p,o)  (*(unsigned int*)((p)+(o)))
#define SW(p,o) (*(signed int*)((p)+(o)))
#define Scratchpad ((unsigned char*)0x1F800000)

extern unsigned char D_8010C32F[];
extern unsigned char D_8001105C[];

extern unsigned int func_8003D4E8(unsigned char* actor, int mode);
extern unsigned int func_8005B36C(int x1, int z1, int x2, int z2);
extern void func_8005C2AC(unsigned char* actor, int anim, int a2, unsigned int heading, int sideFlag);
extern unsigned int func_800B1128(unsigned int sqDist);
extern int func_8001E694(unsigned int angle, int scale);
extern int func_8001E5A4(unsigned int angle, int scale);
extern unsigned int func_800494C0(unsigned char* actor);
extern unsigned int func_8001E730(int val);
extern void func_8005C454(unsigned char* velYPtr, int a1, int lift);
extern unsigned int func_8003CEE4(unsigned char* actor, int a1, int a2, int a3, int distSq);
extern void func_8005BA30(unsigned char* actor, int anim, int frame);
extern unsigned int func_8005B7CC(unsigned int target, unsigned int cur, int step);

int func_80036438(unsigned char* actor) {
    unsigned char* ball = (unsigned char*)W(Scratchpad, 0x2F4);
    int statBias;
    unsigned int targetHeading;
    unsigned int ballAngle;
    unsigned int sideFlag;
    unsigned int bodyHeading;
    unsigned int predAng;
    unsigned int speed;
    int maxSpeed;
    int dx, dz;
    int angDiff;
    unsigned char frames;
    unsigned int pressure;
    int lift;
    unsigned int ret;
    int turn;

    if (SW(ball, 0xC) < 0) {
        return 0;
    }

    statBias = (B(D_8010C32F, B(actor, 0x392) * 12 + B(actor, 0x398) * 264) & 0xF) - 6;
    targetHeading = func_8003D4E8(actor, 9);
    ball = (unsigned char*)W(Scratchpad, 0x2F4);
    ballAngle = func_8005B36C(S(actor, 0x2), S(actor, 0x6), S(ball, 0x2), S(ball, 0x6));

    sideFlag = ((ballAngle - targetHeading) & 0xFFU) < 0x81U;
    if ((sideFlag & 0xFFU) != 0) {
        bodyHeading = targetHeading + 0x40U;
    } else {
        bodyHeading = targetHeading - 0x40U;
    }

    func_8005C2AC(actor, 0x2B, 0, bodyHeading & 0xFFU, (int)sideFlag);

    predAng = func_8005B36C(S(Scratchpad, 0xF0), S(Scratchpad, 0xF4), S(Scratchpad, 0x46), S(Scratchpad, 0x76));
    dx = S(Scratchpad, 0xF0) - S(Scratchpad, 0x46);
    dz = S(Scratchpad, 0xF4) - S(Scratchpad, 0x76);
    speed = func_800B1128((unsigned int)(dx * dx + dz * dz)) / 6U;

    angDiff = (int)((B(actor, 0x3A4) - bodyHeading) & 0xFFU);
    if (angDiff < 0x81) {
        angDiff = angDiff >> 4;
    } else {
        angDiff = (int)((bodyHeading - B(actor, 0x3A4)) & 0xFFU) >> 4;
    }
    maxSpeed = (statBias + 0x20) - angDiff;
    if (maxSpeed < (int)speed) {
        speed = (unsigned int)maxSpeed;
    }

    predAng &= 0xFFU;
    S(actor, 0x3CA) = (signed short)func_8001E694(predAng, (int)speed);
    S(actor, 0x3CE) = (signed short)func_8001E5A4(predAng, (int)speed);

    frames = B(D_8001105C, 0);
    S(actor, 0x3CC) = (signed short)((S(Scratchpad, 0x52 + frames * 2) - S(Scratchpad, 0xF2)) / (int)frames);

    pressure = func_800494C0(actor);
    if (pressure <= 0xC3FFU && (func_8001E730(B(actor, 0x3B8) - 0x3C) & 0xFFU) == 0) {
        lift = (-statBias) + 1;
    } else {
        lift = (-statBias) - 3;
    }
    func_8005C454(actor + 0x3CC, 0x10, (signed short)lift);

    frames = B(D_8001105C, 0);
    S(Scratchpad, 0xF0) = (signed short)(S(Scratchpad, 0xF0) + S(actor, 0x3CA) * frames);
    S(Scratchpad, 0xF2) = (signed short)(S(Scratchpad, 0xF2) + S(actor, 0x3CC) * frames);
    S(Scratchpad, 0xF4) = (signed short)(S(Scratchpad, 0xF4) + S(actor, 0x3CE) * frames);

    ret = func_8003CEE4(actor, 8, 1, 1, 0x14400);
    if ((ret & 0xFFU) == 0) {
        return 0;
    }

    func_8005BA30(actor, 0x2B, 0);
    B(actor, 0x3D1) = 0;
    B(actor, 0x3AC) = (unsigned char)sideFlag;
    B(actor, 0x3AA) = (unsigned char)func_8005B7CC(bodyHeading & 0xFFU, B(actor, 0x3AA), 0xC);
    B(actor, 0x3A4) = (unsigned char)predAng;
    turn = 0x1C0 - (int)B(actor, 0x3AA);
    S(actor, 0x26) = (signed short)((turn % 256) * 16);
    B(actor, 0x3B0) = (unsigned char)bodyHeading;
    B(actor, 0x3AB) = (unsigned char)targetHeading;
    return 1;
}
