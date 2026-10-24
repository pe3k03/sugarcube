# SugarCube font rasterizer.
# Copyright (c) 2026 Peter Štieber. PolyForm Noncommercial 1.0.0. See LICENSE-SUGARCUBE.
# Required Notice: Copyright (c) 2026 Peter Štieber. This SugarCube file may not be sold or used commercially.
# Press Start 2P itself stays under the SIL Open Font License. See src/fonts/OFL.txt.

"""Rasterize Press Start 2P (SIL OFL) into 8x8 GFX bitmaps."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

root = Path(__file__).resolve().parents[1]
ttf = root / "src" / "fonts" / "PressStart2P-Regular.ttf"
out = root / "src" / "fonts" / "PressStart2P8.h"

font = ImageFont.truetype(str(ttf), 8)
lines = [
    "// Press Start 2P Regular 8px bitmaps.",
    "// Font by Cody 'CodeMan38' Kalama, SIL Open Font License 1.1.",
    "// https://github.com/google/fonts/tree/main/ofl/pressstart2p",
    "#pragma once",
    "#include <Arduino.h>",
    "static const uint8_t kPressStart2P[95][8] PROGMEM = {",
]
for code in range(32, 127):
    ch = chr(code)
    img = Image.new("1", (8, 8), 0)
    draw = ImageDraw.Draw(img)
    draw.text((0, 0), ch, font=font, fill=1)
    rows = []
    for y in range(8):
        bits = 0
        for x in range(8):
            if img.getpixel((x, y)):
                bits |= 0x80 >> x
        rows.append(f"0x{bits:02X}")
    comment = ch if ch.isalnum() or ch in ".:-+/<> " else hex(code)
    lines.append(f"    {{{', '.join(rows)}}}, // {code} {comment}")
lines.append("};")
lines.append("")
lines.append("inline const uint8_t *pressStart2PGlyph(char c) {")
lines.append("    unsigned char uc = static_cast<unsigned char>(c);")
lines.append("    if (uc < 32 || uc > 126) uc = 32;")
lines.append("    return kPressStart2P[uc - 32];")
lines.append("}")
lines.append("")
out.write_text("\n".join(lines), encoding="utf-8")
print("wrote", out, "bytes", out.stat().st_size)
