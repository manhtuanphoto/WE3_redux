"""Extract and decode PSX SPU ADPCM (VAGp) commentary clips from Winning Eleven 2002 BIN.

Reads Mode 2 Form 1 sectors (2352 bytes/sector, 2048 bytes user data at +24)
directly from the BIN image, ignoring broken ISO9660 TOCs inside .RA archives.
"""

import os
import struct
import wave

BIN_PATH = r"K:\PSX\Winning Eleven 2002 - European Deluxe 2002-03.bin"
OUT_ROOT = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "assets",
    "audio",
    "commentary",
    "en",
)

# Standard PSX SPU ADPCM filter coefficients
VAG_FILTERS = [
    (0.0, 0.0),
    (60.0 / 64.0, 0.0),
    (115.0 / 64.0, -52.0 / 64.0),
    (98.0 / 64.0, -55.0 / 64.0),
    (122.0 / 64.0, -60.0 / 64.0),
]

RA_ARCHIVES = [
    ("J10", 30000, 20480000 // 2048),
    ("J11", 40000, 22528000 // 2048),
    ("J12", 51000, 18432000 // 2048),
]

# Representative WE2002 commentary bank prefix -> WE3 Redux event folder
BANK_MAP = {
    "01": "kickoff",
    "05": "goal",
    "07": "goal",
    "09": "shot",
    "11": "shot",
    "19": "save",
    "20": "save",
    "25": "foul",
    "26": "free_kick",
    "27": "yellow_card",
    "28": "red_card",
    "35": "corner",
    "36": "throw_in",
    "37": "offside",
    "40": "penalty",
    "47": "halftime",
    "48": "fulltime",
}


def decode_vag_adpcm(adpcm_bytes: bytes) -> bytes:
    """Decode PSX 4-bit ADPCM blocks (16 bytes -> 28 s16 samples) into PCM."""
    s_1 = 0.0
    s_2 = 0.0
    pcm = bytearray()
    n_blocks = len(adpcm_bytes) // 16
    for b in range(n_blocks):
        blk = adpcm_bytes[b * 16 : (b + 1) * 16]
        predict_nr = (blk[0] >> 4) & 0x0F
        shift_factor = blk[0] & 0x0F
        flags = blk[1]
        if predict_nr >= len(VAG_FILTERS):
            predict_nr = 0
        f0, f1 = VAG_FILTERS[predict_nr]
        for i in range(14):
            byte_val = blk[2 + i]
            for nib in (byte_val & 0x0F, (byte_val >> 4) & 0x0F):
                if nib & 0x08:
                    nib -= 16
                sample = float(nib << (12 - shift_factor))
                sample += s_1 * f0 + s_2 * f1
                s_2 = s_1
                s_1 = sample
                clamped = max(-32768, min(32767, int(round(sample))))
                pcm.extend(struct.pack("<h", clamped))
        if flags & 0x01:
            break
    return bytes(pcm)


def write_wav(out_path: str, pcm: bytes, sample_rate: int = 22050) -> None:
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with wave.open(out_path, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        wf.writeframes(pcm)


def read_ra_sectors(fh, lba: int, n_sectors: int) -> bytes:
    buf = bytearray(n_sectors * 2048)
    for s in range(n_sectors):
        fh.seek((lba + s) * 2352 + 24)
        buf[s * 2048 : (s + 1) * 2048] = fh.read(2048)
    return bytes(buf)


def extract_starter_pack(max_per_event: int = 3) -> int:
    if not os.path.exists(BIN_PATH):
        print(f"BIN not found: {BIN_PATH}")
        return 0

    counts = {}
    total_written = 0
    with open(BIN_PATH, "rb") as fh:
        for tag, lba, n_sec in RA_ARCHIVES:
            raw = read_ra_sectors(fh, lba, n_sec)
            for s in range(n_sec):
                off = s * 2048
                if raw[off : off + 4] != b"VAGp":
                    continue
                data_sz, sr = struct.unpack(">II", raw[off + 12 : off + 20])
                name = raw[off + 32 : off + 48].split(b"\x00")[0].decode("ascii", errors="replace").strip()
                prefix = name[:2]
                ev = BANK_MAP.get(prefix)
                if not ev:
                    continue
                if counts.get(ev, 0) >= max_per_event:
                    continue
                if data_sz < 16 or off + 48 + data_sz > len(raw):
                    continue
                pcm = decode_vag_adpcm(raw[off + 48 : off + 48 + data_sz])
                safe_name = "".join(c if c.isalnum() or c in "._-" else "_" for c in name)
                out_wav = os.path.join(OUT_ROOT, ev, f"we2002_{tag}_{safe_name}.wav")
                write_wav(out_wav, pcm, sr if 8000 <= sr <= 48000 else 22050)
                counts[ev] = counts.get(ev, 0) + 1
                total_written += 1

    print(f"Extracted {total_written} WE2002 commentary WAVs across {len(counts)} events: {counts}")
    return total_written


if __name__ == "__main__":
    extract_starter_pack()
