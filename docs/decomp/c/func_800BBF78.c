#include <stdint.h>

/* Array of pointers to sequence track arrays; channel struct stride is 0xB0 (176 bytes) */
extern uint8_t *D_8010C2A8[];

/* Audio/SEQ event handler function pointers */
extern int32_t (*D_800E7808)(int16_t seq_idx, int16_t ch_idx, uint32_t arg2, uint32_t arg3);
extern int32_t (*D_800E780C)(int16_t seq_idx, int16_t ch_idx, uint32_t arg2);
extern void    (*D_800E7810)(int16_t seq_idx, int16_t ch_idx);
extern int32_t (*D_800E7814)(int16_t seq_idx, int16_t ch_idx, uint32_t arg2);
extern int32_t (*D_800E7818)(int16_t seq_idx, int16_t ch_idx, uint32_t arg2);

extern void func_800BBD34(int16_t seq_idx, int16_t ch_idx, uint8_t event_byte);
extern uint32_t func_800BC328(int16_t seq_idx, int16_t ch_idx);

/*
 * PS1 SEQ / MIDI channel stream event decoder.
 * Reads the next byte from channel `ch_idx` (stride 0xB0) of sequence `seq_idx`
 * in D_8010C2A8, handles loop/end checks (0x98 & 0x401, end-of-track 0x2F),
 * updates running status (chan+0x16) and MIDI channel (chan+0x17), and
 * dispatches Note On (0x90), Control Change (0xB0), Program Change (0xC0),
 * Pitch Bend (0xE0), and Meta/SysEx (0xF0 / 0xFF) via D_800E7808..D_800E7818.
 */
int32_t func_800BBF78(int16_t seq_idx, int16_t ch_idx) {
    uint8_t *ch = D_8010C2A8[seq_idx] + (int32_t)ch_idx * 0xB0;
    uint8_t *stream = *(uint8_t **)(ch + 0x00);
    uint8_t byte0 = *stream++;
    int32_t eot_flag = 0;

    *(uint8_t **)(ch + 0x00) = stream;

    if ((*(uint32_t *)(ch + 0x98) & 0x401) == 0x401) {
        uint8_t *loop_ptr = *(uint8_t **)(ch + 0x10);
        if (stream == loop_ptr + 1) {
            func_800BBD34(seq_idx, ch_idx, loop_ptr[1]);
            return -1;
        }
    }

    if (byte0 & 0x80) {
        /* New MIDI status byte */
        uint8_t status = byte0 & 0xF0;
        ch[0x17] = byte0 & 0x0F;

        switch (status) {
            case 0x90: {
                uint8_t note;
                uint8_t vel;
                stream = *(uint8_t **)(ch + 0x00);
                ch[0x16] = status;
                note = *stream++;
                vel = *stream++;
                *(uint8_t **)(ch + 0x00) = stream;
                *(uint32_t *)(ch + 0x90) = func_800BC328(seq_idx, ch_idx);
                D_800E7808(seq_idx, ch_idx, note, vel);
                return eot_flag;
            }
            case 0xB0: {
                uint8_t param;
                stream = *(uint8_t **)(ch + 0x00);
                ch[0x16] = status;
                param = *stream++;
                *(uint8_t **)(ch + 0x00) = stream;
                return D_800E7818(seq_idx, ch_idx, param);
            }
            case 0xC0: {
                uint8_t prog;
                stream = *(uint8_t **)(ch + 0x00);
                ch[0x16] = status;
                prog = *stream++;
                *(uint8_t **)(ch + 0x00) = stream;
                return D_800E780C(seq_idx, ch_idx, prog);
            }
            case 0xE0: {
                stream = *(uint8_t **)(ch + 0x00);
                ch[0x16] = status;
                *(uint8_t **)(ch + 0x00) = stream + 1;
                D_800E7810(seq_idx, ch_idx);
                return eot_flag;
            }
            case 0xF0: {
                uint8_t meta;
                stream = *(uint8_t **)(ch + 0x00);
                ch[0x16] = 0xFF;
                meta = *stream++;
                *(uint8_t **)(ch + 0x00) = stream;
                if (meta == 0x2F) {
                    eot_flag = 1;
                    func_800BBD34(seq_idx, ch_idx, 0x2F);
                    return eot_flag;
                }
                return D_800E7814(seq_idx, ch_idx, meta);
            }
            default:
                return eot_flag;
        }
    } else {
        /* Running status */
        uint8_t running = ch[0x16];
        switch (running) {
            case 0x90: {
                uint8_t vel;
                stream = *(uint8_t **)(ch + 0x00);
                vel = *stream++;
                *(uint8_t **)(ch + 0x00) = stream;
                *(uint32_t *)(ch + 0x90) = func_800BC328(seq_idx, ch_idx);
                D_800E7808(seq_idx, ch_idx, byte0, vel);
                return eot_flag;
            }
            case 0xB0:
                return D_800E7818(seq_idx, ch_idx, byte0);
            case 0xC0:
                return D_800E780C(seq_idx, ch_idx, byte0);
            case 0xE0:
                D_800E7810(seq_idx, ch_idx);
                return eot_flag;
            case 0xFF:
                if (byte0 == 0x2F) {
                    eot_flag = 1;
                    func_800BBD34(seq_idx, ch_idx, 0x2F);
                    return eot_flag;
                }
                return D_800E7814(seq_idx, ch_idx, byte0);
            default:
                return eot_flag;
        }
    }
}
