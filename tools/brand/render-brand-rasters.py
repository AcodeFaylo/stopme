#!/usr/bin/env python3
"""Regenerate the stopme raster brand assets from their SVG sources.

Every PNG, ICO and ICNS under ui/data/images that carries the application
icon is derived from an SVG in the same tree; this script is the only thing
that should ever write them.

    pip install cairosvg pillow
    python3 tools/brand/render-brand-rasters.py

Run it from the repository root after changing any of the source SVGs.
"""

from __future__ import annotations

import struct
import sys
from pathlib import Path

try:
    import cairosvg
    from PIL import Image
except ImportError:  # pragma: no cover - dependency hint only
    sys.exit("needs cairosvg and pillow: pip install cairosvg pillow")

ROOT = Path(__file__).resolve().parents[2]
IMAGES = ROOT / "ui" / "data" / "images"

NORMAL = IMAGES / "workrave-normal.svg"
QUIET = IMAGES / "workrave-quiet.svg"
SUSPENDED = IMAGES / "workrave-suspended.svg"
SAD = IMAGES / "stopme-panda-sad.svg"
MASCOT = IMAGES / "stopme-panda.svg"

ICON_THEME_SIZES = (16, 24, 32, 48, 64, 96, 128)
ICO_SIZES = (16, 24, 32, 48, 64, 128, 256)

# ICNS element types that take a PNG payload, keyed by pixel size.
ICNS_TYPES = {32: b"ic11", 64: b"ic12", 128: b"ic07", 256: b"ic08",
              512: b"ic09", 1024: b"ic10"}


def render(svg: Path, size: int) -> Image.Image:
    """Rasterise `svg` into a transparent square of `size` pixels."""
    png = cairosvg.svg2png(url=str(svg), output_width=size, output_height=size)
    return Image.open(__import__("io").BytesIO(png)).convert("RGBA")


def write_png(svg: Path, dest: Path, size: int) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    render(svg, size).save(dest)
    print(f"  {dest.relative_to(ROOT)}  {size}x{size}")


def write_ico(svg: Path, dest: Path) -> None:
    largest = render(svg, max(ICO_SIZES))
    largest.save(dest, format="ICO", sizes=[(s, s) for s in ICO_SIZES])
    print(f"  {dest.relative_to(ROOT)}  {sorted(ICO_SIZES)}")


def write_icns(svg: Path, dest: Path) -> None:
    """Write an ICNS container with PNG payloads.

    Pillow's ICNS writer is not available on every platform, so the (very
    small) container format is assembled by hand.
    """
    import io

    chunks = []
    for size, kind in sorted(ICNS_TYPES.items()):
        buf = io.BytesIO()
        render(svg, size).save(buf, format="PNG")
        data = buf.getvalue()
        chunks.append(kind + struct.pack(">I", len(data) + 8) + data)

    body = b"".join(chunks)
    dest.write_bytes(b"icns" + struct.pack(">I", len(body) + 8) + body)
    print(f"  {dest.relative_to(ROOT)}  {sorted(ICNS_TYPES)}")


def write_mascot(dest: Path, width: int, height: int) -> None:
    """Fit the full mascot, which is square, into a non-square frame."""
    side = min(width, height)
    canvas = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    canvas.alpha_composite(render(MASCOT, side),
                           ((width - side) // 2, (height - side) // 2))
    canvas.save(dest)
    print(f"  {dest.relative_to(ROOT)}  {width}x{height}")


def main() -> None:
    print("tray / timer-box icons")
    write_png(NORMAL, IMAGES / "workrave-icon-medium.png", 24)
    write_png(QUIET, IMAGES / "workrave-quiet-icon-medium.png", 24)
    write_png(SUSPENDED, IMAGES / "workrave-suspended-icon-medium.png", 24)

    print("about / update dialog mascot")
    write_mascot(IMAGES / "workrave.png", 123, 92)

    print("break windows and break warnings")
    for name in ("micro-break.png", "rest-break.png", "daily-limit.png"):
        write_png(NORMAL, IMAGES / name, 64)
    write_png(NORMAL, IMAGES / "prelude-hint.png", 48)
    write_png(SAD, IMAGES / "prelude-hint-sad.png", 48)

    print("hicolor icon theme")
    for size in ICON_THEME_SIZES:
        write_png(NORMAL, IMAGES / "workrave" / f"{size}x{size}" / "workrave.png", size)

    print("windows")
    write_ico(NORMAL, IMAGES / "windows" / "workrave-normal.ico")
    write_ico(QUIET, IMAGES / "windows" / "workrave-quiet.ico")
    write_ico(SUSPENDED, IMAGES / "windows" / "workrave-suspended.ico")

    print("macos")
    write_icns(NORMAL, IMAGES / "macos" / "workrave.icns")


if __name__ == "__main__":
    main()
