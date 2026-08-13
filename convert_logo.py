#!/usr/bin/env python3
"""
convert_logo.py
---------------
Wandelt ein PNG/JPG-Logo in eine C-Header-Datei um, die direkt
in den TFT_eSPI-Sketch eingebunden werden kann (RGB565, big-endian = 0,
little-endian = 1 je nach Bedarf).

Aufruf:
    python3 convert_logo.py logo.png ESP_QR_Display/logo.h

Voraussetzungen:
    pip install Pillow
"""

import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("Bitte zuerst 'pip install Pillow' ausfuehren.")
    sys.exit(1)

TARGET_W = 240
TARGET_H = 240


def rgb_to_565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def main() -> int:
    if len(sys.argv) < 3:
        print("Aufruf: python3 convert_logo.py <eingabe.png> <ausgabe.h>")
        return 1

    src = Path(sys.argv[1])
    dst = Path(sys.argv[2])

    if not src.exists():
        print(f"Datei nicht gefunden: {src}")
        return 1

    img = Image.open(src).convert("RGBA")

    # Auf 240x240 anpassen (mit schwarzem Hintergrund fuer Transparenz)
    img.thumbnail((TARGET_W, TARGET_H), Image.LANCZOS)
    canvas = Image.new("RGB", (TARGET_W, TARGET_H), (0, 0, 0))
    ox = (TARGET_W - img.width) // 2
    oy = (TARGET_H - img.height) // 2
    canvas.paste(img, (ox, oy), img if img.mode == "RGBA" else None)

    pixels = list(canvas.getdata())
    print(f"Konvertiere {len(pixels)} Pixel ({TARGET_W}x{TARGET_H}) ...")

    lines = []
    lines.append("// Automatisch erzeugt von convert_logo.py")
    lines.append("// Quelle: " + src.name)
    lines.append("#pragma once")
    lines.append("#include <stdint.h>")
    lines.append("")
    lines.append(f"#define LOGO_WIDTH  {TARGET_W}")
    lines.append(f"#define LOGO_HEIGHT {TARGET_H}")
    lines.append("")
    lines.append(f"const uint16_t logo_data[{TARGET_W * TARGET_H}] PROGMEM = {{")

    chunk = []
    for i, (r, g, b) in enumerate(pixels):
        chunk.append(f"0x{rgb_to_565(r, g, b):04X}")
        if len(chunk) == 12:
            lines.append("    " + ", ".join(chunk) + ",")
            chunk = []
    if chunk:
        lines.append("    " + ", ".join(chunk))
    lines.append("};")
    lines.append("")

    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_text("\n".join(lines), encoding="utf-8")
    print(f"Geschrieben: {dst}  ({dst.stat().st_size/1024:.1f} kB)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
