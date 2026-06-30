#!/usr/bin/env python3
"""
png2h.py - Convert a PNG into a C header for the ST7789V display driver.

The generated array is consumed by:

    gfx_draw_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                   const uint16_t *data);   /* src/display_hw.h */

Pixel format: RGB565, one uint16_t per pixel, row-major (top-left first).

ENDIANNESS (important):
    gfx_draw_image() streams the array as raw bytes: st_write_data((const u8*)data, ...).
    The PIC24 is little-endian, so a stored word 0xHHLL goes on the wire as LL, HH.
    The ST7789 expects the RGB565 *high* byte first. Therefore the words written to
    the header are BYTE-SWAPPED versions of the logical RGB565 value, so that the
    little-endian memory layout puts the high byte on the wire first. This matches
    what st_spi_write_color() sends for fills/pixels.

    Use --no-swap only if you feed the data through a 16-bit / MODE16 SPI path
    (e.g. ST_CONF_USE_SPI16_COLOR_FILL), where the word is sent as-is.

RGB565 has no alpha: transparent pixels are composited over --bg (default black).

Usage:
    python tools/png2h.py logo.png
    python tools/png2h.py logo.png -o src/img_logo.h -n img_logo
    python tools/png2h.py logo.png --bg 0xFFFF --resize 120x120

Requires: Pillow  (python -m pip install Pillow)
"""

import argparse
import os
import re
import sys

try:
    from PIL import Image
except ImportError:
    sys.exit("error: Pillow not installed. Run: python -m pip install Pillow")

DISP_W = 240
DISP_H = 240

# XC16 places `const` in the auto_psv `.const` section, a single page shared by
# ALL const data (fonts, color tables, every image). Hard ceiling 32 KB.
PSV_CONST_LIMIT = 32768
# Warn margin: leave room for fonts/tables that share the same section.
PSV_WARN_BYTES = 24576  # 24 KB


def rgb888_to_565(r, g, b):
    """Convert 8-bit RGB to RGB565 with rounding (logical value, not byte-swapped)."""
    r5 = (r * 31 + 127) // 255
    g6 = (g * 63 + 127) // 255
    b5 = (b * 31 + 127) // 255
    return (r5 << 11) | (g6 << 5) | b5


def parse_color(text):
    """Parse a 0xRGB565 word or #RRGGBB hex into a logical RGB565 value."""
    t = text.strip().lower()
    if t.startswith("#"):
        h = t[1:]
        if len(h) != 6:
            raise ValueError("#RRGGBB needs 6 hex digits")
        r, g, b = int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16)
        return rgb888_to_565(r, g, b)
    return int(t, 0) & 0xFFFF


def parse_resize(text):
    m = re.fullmatch(r"(\d+)x(\d+)", text.strip().lower())
    if not m:
        raise ValueError("--resize must be WxH, e.g. 120x90")
    return int(m.group(1)), int(m.group(2))


def sanitize_ident(name):
    """Make a C identifier from a filename stem."""
    ident = re.sub(r"[^0-9a-zA-Z_]", "_", name)
    if ident and ident[0].isdigit():
        ident = "img_" + ident
    return ident or "image"


def main():
    ap = argparse.ArgumentParser(
        description="Convert a PNG into an RGB565 C header for gfx_draw_image()."
    )
    ap.add_argument("png", help="input PNG path")
    ap.add_argument("-o", "--out", help="output .h path (default: <png>.h next to PNG)")
    ap.add_argument("-n", "--name", help="C array/identifier base (default: from filename)")
    ap.add_argument("--bg", default="0x0000",
                    help="background for transparent pixels: 0xRGB565 or #RRGGBB (default 0x0000)")
    ap.add_argument("--resize", help="resize to WxH before converting, e.g. 120x90")
    ap.add_argument("--no-swap", action="store_true",
                    help="store logical RGB565 words (for 16-bit SPI path); default byte-swaps")
    ap.add_argument("--cols", type=int, default=12, help="words per line in output (default 12)")
    args = ap.parse_args()

    if not os.path.isfile(args.png):
        sys.exit("error: no such file: " + args.png)

    bg565 = parse_color(args.bg)
    bg_r = ((bg565 >> 11) & 0x1F) * 255 // 31
    bg_g = ((bg565 >> 5) & 0x3F) * 255 // 63
    bg_b = (bg565 & 0x1F) * 255 // 31

    img = Image.open(args.png)
    if args.resize:
        w, h = parse_resize(args.resize)
        img = img.resize((w, h), Image.LANCZOS)

    # Composite over background to flatten alpha, then take pure RGB.
    img = img.convert("RGBA")
    bg = Image.new("RGBA", img.size, (bg_r, bg_g, bg_b, 255))
    img = Image.alpha_composite(bg, img).convert("RGB")

    w, h = img.size
    if w == 0 or h == 0:
        sys.exit("error: image has zero dimension")
    if w > DISP_W or h > DISP_H:
        sys.stderr.write(
            "warning: %dx%d exceeds %dx%d display; gfx_draw_image() will reject "
            "off-screen placement.\n" % (w, h, DISP_W, DISP_H))

    nbytes = w * h * 2
    if nbytes > PSV_CONST_LIMIT:
        sys.stderr.write(
            "warning: %d bytes (%dx%d) exceeds the XC16 auto_psv .const limit of "
            "%d B (32 KB). This WILL fail to link: '.const exceeds 32K'. Shrink with "
            "--resize, or move to a space(prog) flash-stream path.\n"
            % (nbytes, w, h, PSV_CONST_LIMIT))
    elif nbytes > PSV_WARN_BYTES:
        sys.stderr.write(
            "warning: %d bytes (%dx%d) is large. .const (32 KB) is shared by fonts "
            "and all other const, so this may overflow once linked. Shrink with "
            "--resize if the link reports '.const exceeds 32K'.\n"
            % (nbytes, w, h))

    name = sanitize_ident(args.name or os.path.splitext(os.path.basename(args.png))[0])
    guard = name.upper() + "_H"

    px = img.load()
    words = []
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y]
            v = rgb888_to_565(r, g, b)
            if not args.no_swap:
                v = ((v & 0x00FF) << 8) | ((v >> 8) & 0xFF)
            words.append(v)

    out_path = args.out or (os.path.splitext(args.png)[0] + ".h")

    lines = []
    lines.append("/* Auto-generated by tools/png2h.py - do not edit by hand. */")
    lines.append("/* Source: %s  (%dx%d, RGB565%s) */"
                 % (os.path.basename(args.png), w, h,
                    "" if args.no_swap else ", byte-swapped for gfx_draw_image"))
    lines.append("#ifndef %s" % guard)
    lines.append("#define %s" % guard)
    lines.append("")
    lines.append("#include <stdint.h>")
    lines.append("")
    lines.append("#define %s_WIDTH  %d" % (name.upper(), w))
    lines.append("#define %s_HEIGHT %d" % (name.upper(), h))
    lines.append("")
    lines.append("static const uint16_t %s[%d] = {" % (name, len(words)))
    for i in range(0, len(words), args.cols):
        chunk = words[i:i + args.cols]
        lines.append("    " + ", ".join("0x%04X" % v for v in chunk) + ",")
    lines.append("};")
    lines.append("")
    lines.append("#endif /* %s */" % guard)
    lines.append("")

    with open(out_path, "w", newline="\n") as f:
        f.write("\n".join(lines))

    sys.stderr.write("wrote %s: %s[%d] (%dx%d)%s\n"
                     % (out_path, name, len(words), w, h,
                        "" if args.no_swap else " byte-swapped"))


if __name__ == "__main__":
    main()
