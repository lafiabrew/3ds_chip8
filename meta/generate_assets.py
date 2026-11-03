#!/usr/bin/env python3
import math
import os
import struct
import wave
import zlib
from generate_bnr_shared import draw_banner_text_centered, write_cbmd_bnr  # type: ignore

STYLE = 'chip8'
BORDER = (95, 250, 155)
BG = (10, 22, 16)
B_TITLE = 'CHIP-8 STUDIO'
B_SUB = 'VM & HEX DISASSEMBLER'
FREQS = (659.25, 783.99, 987.77)


def write_png(path: str, width: int, height: int, rgba_bytes: bytes) -> None:
    def chunk(tag: bytes, data: bytes) -> bytes:
        return (
            struct.pack(">I", len(data))
            + tag
            + data
            + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
        )

    raw_rows = bytearray()
    stride = width * 4
    for y in range(height):
        raw_rows.append(0)
        raw_rows.extend(rgba_bytes[y * stride : (y + 1) * stride])

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    compressed = zlib.compress(bytes(raw_rows), 9)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", ihdr))
        f.write(chunk(b"IDAT", compressed))
        f.write(chunk(b"IEND", b""))


def draw_emblem_pixel(x: float, y: float, cx: float, cy: float, scale: float):
    dx = (x - cx) / scale
    dy = (y - cy) / scale
    r2 = dx * dx + dy * dy
    if STYLE == "grid":
        if abs(dx) <= 14 and abs(dy) <= 14:
            gx = int((dx + 14) // 7)
            gy = int((dy + 14) // 7)
            if int(abs(dx + 14)) % 7 == 0 or int(abs(dy + 14)) % 7 == 0:
                return (40, 75, 130, 255)
            return (95, 245, 165, 255) if ((gx + gy) % 2 == 0) else (24, 38, 68, 255)
    elif STYLE == "sand":
        if abs(dx) <= 14 and abs(dy) <= 14:
            if dy > 4:
                return (65, 170, 255, 255)
            if abs(dx) <= (dy + 10):
                return (255, 215, 75, 255)
    elif STYLE == "chip8":
        if abs(dx) <= 15 and abs(dy) <= 12:
            if abs(abs(dx) - 9) <= 2 and abs(dy) <= 6:
                return (95, 255, 165, 255)
            if abs(dx) <= 2 and abs(dy) <= 2:
                return (255, 235, 95, 255)
            return (18, 42, 30, 255)
    elif STYLE == "cards":
        if -12 <= dx <= 6 and -14 <= dy <= 10:
            if abs(dx + 3) + abs(dy + 2) <= 5:
                return (220, 40, 52, 255)
            return (245, 248, 255, 255)
        if -4 <= dx <= 14 and -8 <= dy <= 15:
            return (225, 232, 245, 255)
    elif STYLE == "dex":
        if abs(dx) <= 13 and abs(dy) <= 15:
            if -10 <= dx <= -3 and -12 <= dy <= -5:
                return (80, 225, 255, 255)
            if abs(dx) <= 9 and 0 <= dy <= 10:
                return (85, 245, 145, 255)
            return (225, 42, 58, 255)
    elif STYLE == "crate":
        if abs(dx) <= 14 and abs(dy) <= 14:
            if abs(dx) >= 11 or abs(dy) >= 11 or abs(dx - dy) <= 2.2:
                return (185, 115, 42, 255)
            return (245, 185, 80, 255)
    elif STYLE == "wheel":
        if r2 <= 16.0 * 16.0:
            if r2 <= 4.0 * 4.0:
                return (255, 215, 75, 255)
            if r2 >= 13.5 * 13.5:
                return (215, 160, 50, 255)
            ang = int((math.atan2(dy, dx) + math.pi) * 6.0 / math.pi)
            if ang == 0:
                return (45, 200, 95, 255)
            return (215, 42, 52, 255) if (ang % 2 == 1) else (28, 32, 42, 255)
    elif STYLE == "tile":
        if abs(dx) <= 12 and abs(dy) <= 15:
            if dx > 9 or dy > 12:
                return (45, 155, 105, 255)
            if abs(dx) <= 3 and abs(dy) <= 8:
                return (220, 48, 58, 255)
            return (248, 244, 228, 255)
    return None


def generate_icon(path: str) -> None:
    w, h = 48, 48
    buf = bytearray(w * h * 4)
    for y in range(h):
        for x in range(w):
            idx = (y * w + x) * 4
            if x < 2 or x >= w - 2 or y < 2 or y >= h - 2:
                buf[idx : idx + 4] = bytes((*BORDER, 255))
                continue
            em = draw_emblem_pixel(x, y, 23.5, 23.5, 1.0)
            buf[idx : idx + 4] = bytes(em if em is not None else (*BG, 255))
    write_png(path, w, h, bytes(buf))


def generate_banner(path: str) -> bytes:
    w, h = 256, 128
    buf = bytearray(w * h * 4)
    for y in range(h):
        for x in range(w):
            idx = (y * w + x) * 4
            r = min(255, BG[0] + (y * 16) // h)
            g = min(255, BG[1] + (y * 20) // h)
            b = min(255, BG[2] + (y * 24) // h)
            em = draw_emblem_pixel(x, y, 128.0, 42.0, 1.8)
            buf[idx : idx + 4] = bytes(em if em is not None else (r, g, b, 255))

    draw_banner_text_centered(buf, w, h, 86, B_TITLE, 2, (*BORDER, 255))
    draw_banner_text_centered(buf, w, h, 108, B_SUB, 1, (225, 240, 255, 255))
    raw = bytes(buf)
    write_png(path, w, h, raw)
    return raw


def generate_audio(path: str) -> None:
    sample_rate = 22050
    duration_s = 0.8
    n_samples = int(sample_rate * duration_s)
    with wave.open(path, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        frames = bytearray()
        for i in range(n_samples):
            t = i / sample_rate
            freq = FREQS[0] if t < 0.25 else (FREQS[1] if t < 0.5 else FREQS[2])
            env = max(0.0, 1.0 - (t / duration_s))
            val = int(12000 * env * math.sin(2.0 * math.pi * freq * t))
            frames.extend(struct.pack("<h", val))
        wf.writeframes(bytes(frames))


def main() -> None:
    meta_dir = os.path.dirname(os.path.abspath(__file__))
    generate_icon(os.path.join(meta_dir, "icon.png"))
    banner_rgba = generate_banner(os.path.join(meta_dir, "banner.png"))
    generate_audio(os.path.join(meta_dir, "audio.wav"))
    write_cbmd_bnr(os.path.join(meta_dir, "banner.bnr"), FREQS, banner_rgba)
    print("Generated", B_TITLE, "assets")


if __name__ == "__main__":
    main()
