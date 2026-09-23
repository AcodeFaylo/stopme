#!/usr/bin/env python3
"""Regenerate the stopme raster brand assets from their SVG sources.

Every PNG, ICO and ICNS under ui/data/images that carries the application
icon, the Windows installer wizard bitmaps and the macOS disk image
background are derived from an SVG in the same tree; this script is the
only thing that should ever write them.

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
TOOLKITS = ROOT / "ui" / "app" / "toolkits"

NORMAL = IMAGES / "workrave-normal.svg"
QUIET = IMAGES / "workrave-quiet.svg"
SUSPENDED = IMAGES / "workrave-suspended.svg"
SAD = IMAGES / "stopme-panda-sad.svg"
MASCOT = IMAGES / "stopme-panda.svg"
WORDMARK = IMAGES / "workrave-text.svg"

MAGENTA = (0xFF, 0x3D, 0x8B)
ELECTRIC_BLUE = (0x55, 0x66, 0xFF)
MIST = (0xEE, 0xF1, 0xF8)

ICON_THEME_SIZES = (16, 24, 32, 48, 64, 96, 128)
ICO_SIZES = (16, 24, 32, 48, 64, 128, 256)

# ICNS element types that take a PNG payload, keyed by pixel size.
ICNS_TYPES = {32: b"ic11", 64: b"ic12", 128: b"ic07", 256: b"ic08",
              512: b"ic09", 1024: b"ic10"}

# Inno Setup wizard bitmaps: the 100% size and the size its documentation
# recommends for 200%. Setup picks whichever fits the display's DPI best.
WIZARD_IMAGE_SIZES = ((164, 314), (328, 604))
WIZARD_SMALL_SIZES = ((55, 55), (110, 106))


def render(svg: Path, size: int) -> Image.Image:
    """Rasterise `svg` into a transparent square of `size` pixels."""
    png = cairosvg.svg2png(url=str(svg), output_width=size, output_height=size)
    return Image.open(__import__("io").BytesIO(png)).convert("RGBA")


def gradient(width: int, height: int) -> Image.Image:
    """The brand's 135 degree magenta to electric blue gradient."""
    canvas = Image.new("RGB", (width, height))
    pixels = canvas.load()
    span = max(width + height - 2, 1)
    for y in range(height):
        for x in range(width):
            t = (x + y) / span
            pixels[x, y] = tuple(round(a + (b - a) * t)
                                 for a, b in zip(MAGENTA, ELECTRIC_BLUE))
    return canvas.convert("RGBA")


def white_wordmark(width: int) -> Image.Image:
    """The stopme wordmark in white, for use on top of the gradient."""
    import io

    svg = WORDMARK.read_text().replace('stroke="url(#stopmeBrand)"', 'stroke="#FFFFFF"')
    height = round(width * 88 / 258)
    png = cairosvg.svg2png(bytestring=svg.encode(), output_width=width, output_height=height)
    return Image.open(io.BytesIO(png)).convert("RGBA")


def glow(size: int, alpha: int) -> Image.Image:
    """A soft white disc; the inset gives the blur room to fade out."""
    from PIL import ImageDraw, ImageFilter

    mask = Image.new("L", (size, size), 0)
    inset = round(size * 0.16)
    ImageDraw.Draw(mask).ellipse((inset, inset, size - 1 - inset, size - 1 - inset), fill=alpha)
    mask = mask.filter(ImageFilter.GaussianBlur(size * 0.07))
    disc = Image.new("RGBA", (size, size), (255, 255, 255, 0))
    disc.putalpha(mask)
    return disc


def wizard_image(width: int, height: int) -> Image.Image:
    """Welcome/finish page panel: wordmark above the mascot on the gradient."""
    canvas = gradient(width, height)
    mark = white_wordmark(round(width * 0.70))
    top = round(height * 0.085)
    canvas.alpha_composite(mark, ((width - mark.width) // 2, top))

    side = round(width * 0.86)
    below = top + mark.height
    y = below + (height - below - side) // 2
    halo = glow(round(side * 1.35), 80)
    canvas.alpha_composite(halo, ((width - halo.width) // 2, y + (side - halo.height) // 2))
    canvas.alpha_composite(render(MASCOT, side), ((width - side) // 2, y))
    return canvas.convert("RGB")


def wizard_small(width: int, height: int) -> Image.Image:
    """Inner page corner tile: the icon's panda face on the gradient."""
    canvas = gradient(width, height)
    side = round(min(width, height) * 0.84)
    canvas.alpha_composite(render(NORMAL, side), ((width - side) // 2, (height - side) // 2))
    return canvas.convert("RGB")


def write_wizard_bitmaps(dest: Path) -> None:
    """24-bit BMPs, the format every Inno Setup version reads."""
    for index, (width, height) in enumerate(WIZARD_IMAGE_SIZES):
        name = "WizModernImage.bmp" if index == 0 else f"WizModernImage-{width}x{height}.bmp"
        wizard_image(width, height).save(dest / name)
        print(f"  {(dest / name).relative_to(ROOT)}  {width}x{height}")
    for index, (width, height) in enumerate(WIZARD_SMALL_SIZES):
        name = "WizModernSmall.bmp" if index == 0 else f"WizModernSmall-{width}x{height}.bmp"
        wizard_small(width, height).save(dest / name)
        print(f"  {(dest / name).relative_to(ROOT)}  {width}x{height}")


def dmg_background() -> Image.Image:
    """Finder window of the macOS disk image: the wordmark above two rings
    joined by an arrow, drawn at 2x (1200x800 pixels for a 600x400 point
    window). The rings sit where dmg.applescript places the app and the
    Applications alias, so move both together."""
    from PIL import ImageDraw

    width, height, scale = 1200, 800, 2  # drawn at 2x again for smooth edges
    canvas = Image.new("RGBA", (width * scale, height * scale), (255, 255, 255, 255))
    draw = ImageDraw.Draw(canvas)

    radius, ring = 192, 8
    for cx in (293, 907):
        box = [(cx - radius) * scale, (487 - radius) * scale,
               (cx + radius) * scale, (487 + radius) * scale]
        draw.ellipse(box, outline=MIST, width=ring * scale)

    # Arrow from the app to Applications, in the brand gradient.
    arrow = Image.new("L", canvas.size, 0)
    shape = ImageDraw.Draw(arrow)
    shape.rectangle([486 * scale, 476 * scale, 700 * scale, 498 * scale], fill=255)
    shape.polygon([(699 * scale, 458 * scale), (728 * scale, 487 * scale),
                   (699 * scale, 516 * scale)], fill=255)
    left, right = 486 * scale, 728 * scale
    fill = Image.new("RGBA", canvas.size)
    pixels = fill.load()
    for x in range(left, right + 1):
        t = (x - left) / (right - left)
        colour = tuple(round(a + (b - a) * t) for a, b in zip(MAGENTA, ELECTRIC_BLUE)) + (255,)
        for y in range(458 * scale, 516 * scale + 1):
            pixels[x, y] = colour
    canvas.paste(fill, (0, 0), arrow)

    canvas = canvas.resize((width, height), Image.LANCZOS)
    mark = render_wordmark(420)
    canvas.alpha_composite(mark, ((width - mark.width) // 2, 62))
    return canvas.convert("RGB")


def render_wordmark(width: int) -> Image.Image:
    """The stopme wordmark in its brand gradient."""
    import io

    height = round(width * 88 / 258)
    png = cairosvg.svg2png(url=str(WORDMARK), output_width=width, output_height=height)
    return Image.open(io.BytesIO(png)).convert("RGBA")


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

    print("windows installers")
    for toolkit in ("gtkmm", "qt"):
        write_wizard_bitmaps(TOOLKITS / toolkit / "dist" / "windows")

    print("macos disk image")
    dest = TOOLKITS / "qt" / "dist" / "macos" / "dmg_background.png"
    # 144 dpi tells Finder the image is 2x, so it fills the 600x400 window.
    dmg_background().save(dest, dpi=(144, 144))
    print(f"  {dest.relative_to(ROOT)}  1200x800 @144dpi")


if __name__ == "__main__":
    main()
