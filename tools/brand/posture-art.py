#!/usr/bin/env python3
"""Emit the sit/stand reminder pandas as plain SVG.

stopme-panda-standing.svg and stopme-panda-sitting.svg show the mascot's
head on a standing body (on its hind legs, arms up) and on a body sitting on
a stool. Both put the head in the same place at the same size, so the
reminder card does not jump when it alternates between them.

Like tail-geometry.py this emits explicit closed polygons, not thick strokes
under a <mask>: Qt's SVG renderer draws these images in the reminder card
and supports neither masks nor SVG2 href on <use>.

    python3 tools/brand/posture-art.py [output-directory]

It writes into ui/data/images by default; run render-brand-rasters.py
afterwards for the PNGs the Gtk reminder window shows.
"""

from __future__ import annotations

import math
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

OUTLINE = "#241A14"
ORANGE = "#F4722B"
BROWN = "#6B3B22"
CREAM = "#EFE3D8"
BLUE = "#5566FF"
BLUE_DARK = "#3A47C9"

STROKE = 6.0          # outline width, as in stopme-panda.svg
HEAD_SCALE = 0.64     # mascot head -> reminder head
HEAD_DX = 128 - 150 * HEAD_SCALE
HEAD_DY = 6 - 13 * HEAD_SCALE


# ── Swept bands (tail, limbs) ─────────────────────────────────────────────────

def bez(p, t):
    (x0, y0), (x1, y1), (x2, y2), (x3, y3) = p
    m = 1 - t
    return (m**3 * x0 + 3 * m * m * t * x1 + 3 * m * t * t * x2 + t**3 * x3,
            m**3 * y0 + 3 * m * m * t * y1 + 3 * m * t * t * y2 + t**3 * y3)


def dbez(p, t):
    (x0, y0), (x1, y1), (x2, y2), (x3, y3) = p
    m = 1 - t
    return (3 * m * m * (x1 - x0) + 6 * m * t * (x2 - x1) + 3 * t * t * (x3 - x2),
            3 * m * m * (y1 - y0) + 6 * m * t * (y2 - y1) + 3 * t * t * (y3 - y2))


class Sweep:
    """A band of constant half-width swept along a chain of cubic Beziers."""

    def __init__(self, segs, half):
        self.segs, self.half = segs, half
        self.table, total, prev = [], 0.0, bez(segs[0], 0)
        for i in range(2001):
            u = i / 2000 * len(segs)
            si = min(int(u), len(segs) - 1)
            p = bez(segs[si], u - si)
            if i:
                total += math.dist(p, prev)
            self.table.append((total, si, u - si, p))
            prev = p
        self.length = total

    def at(self, s):
        s = max(0.0, min(self.length, s))
        lo, hi = 0, len(self.table) - 1
        while lo < hi:
            mid = (lo + hi) // 2
            if self.table[mid][0] < s:
                lo = mid + 1
            else:
                hi = mid
        _, si, t, p = self.table[lo]
        dx, dy = dbez(self.segs[si], t)
        n = math.hypot(dx, dy)
        return p, (dx / n, dy / n)

    def off(self, s, d):
        (x, y), (tx, ty) = self.at(s)
        return (x - ty * d, y + tx * d)

    def side(self, a, b, d, step=4.0):
        n = max(2, int(abs(b - a) / step) + 1)
        return [self.off(a + (b - a) * i / n, d) for i in range(n + 1)]

    def cap(self, s, start):
        """Round end cap at `s`, from the +half side round to the -half side."""
        (x, y), (tx, ty) = self.at(s)
        base = math.atan2(tx, -ty)      # direction of the +half normal
        sign = 1 if start else -1
        return [(x + self.half * math.cos(base + sign * math.pi * k / 12),
                 y + self.half * math.sin(base + sign * math.pi * k / 12))
                for k in range(13)]

    def outline(self, round_start=True):
        pts = self.side(0, self.length, self.half)
        pts += self.cap(self.length, start=False)[1:-1]
        pts += self.side(self.length, 0, -self.half)
        if round_start:
            pts += self.cap(0, start=True)[1:-1]
        return closed(pts)

    def band(self, a, b, round_end=False):
        pts = self.side(a, b, self.half)
        if round_end:
            pts += self.cap(b, start=False)[1:-1]
        pts += self.side(b, a, -self.half)
        return closed(pts)


def closed(points):
    head, *rest = points
    return (f"M {head[0]:.1f} {head[1]:.1f} L "
            + " L ".join(f"{x:.1f} {y:.1f}" for x, y in rest) + " Z")


def tail(segs, half=15.0, band=14.0, first=14.0):
    """The banded tail: orange with brown rings, drawn behind the body."""
    sweep = Sweep(segs, half)
    parts = [f'<path d="{sweep.outline()}" fill="{ORANGE}"/>',
             f'<g fill="{BROWN}" stroke="none">']
    a = first
    while a < sweep.length:
        b = min(a + band, sweep.length)
        parts.append(f'  <path d="{sweep.band(a, b, round_end=b >= sweep.length)}"/>')
        a += 2 * band
    parts.append('</g>')
    # Redraw the outline over the rings.
    parts.append(f'<path d="{sweep.outline()}" fill="none"/>')
    return "\n".join(parts)


def limb(segs, half=11.0):
    """An arm or leg: a brown tube with round ends."""
    return f'<path d="{Sweep(segs, half).outline()}" fill="{BROWN}"/>'


# ── The mascot's head, from stopme-panda.svg ─────────────────────────────────

def head():
    inner = 4.5 / HEAD_SCALE
    return f"""<g transform="translate({HEAD_DX:.2f} {HEAD_DY:.2f}) scale({HEAD_SCALE})" stroke-width="{STROKE / HEAD_SCALE:.2f}">
  <path d="M 108 78 C 90 58 84 32 93 22 C 102 13 124 23 139 44 Z" fill="{ORANGE}"/>
  <path d="M 110 70 C 98 54 94 34 100 28 C 107 21 122 33 133 50 Z" fill="{CREAM}" stroke-width="{inner:.2f}"/>
  <path d="M 192 78 C 210 58 216 32 207 22 C 198 13 176 23 161 44 Z" fill="{ORANGE}"/>
  <path d="M 190 70 C 202 54 206 34 200 28 C 193 21 178 33 167 50 Z" fill="{CREAM}" stroke-width="{inner:.2f}"/>
  <path d="M 150 34 C 198 34 220 67 220 104 C 220 137 191 157 150 157 C 109 157 80 137 80 104 C 80 67 102 34 150 34 Z" fill="{ORANGE}"/>
  <path d="M 104 103 C 118 99 131 105 138 116 C 142 110 158 110 162 116 C 169 105 182 99 196 103 C 211 108 217 126 209 138 C 201 150 183 157 150 157 C 117 157 99 150 91 138 C 83 126 89 108 104 103 Z" fill="#FFFFFF" stroke-width="{inner:.2f}"/>
  <g stroke="none" fill="#FFFFFF">
    <ellipse cx="119" cy="68" rx="11" ry="7.5" transform="rotate(-18 119 68)"/>
    <ellipse cx="181" cy="68" rx="11" ry="7.5" transform="rotate(18 181 68)"/>
  </g>
  <g stroke="none">
    <ellipse cx="121" cy="96" rx="15.5" ry="16.5" fill="#1B1210"/>
    <ellipse cx="179" cy="96" rx="15.5" ry="16.5" fill="#1B1210"/>
    <circle cx="127" cy="89" r="5" fill="#FFFFFF"/>
    <circle cx="185" cy="89" r="5" fill="#FFFFFF"/>
  </g>
  <path d="M 140 117 C 140 113 144 111 150 111 C 156 111 160 113 160 117 C 160 123 155 129 150 129 C 145 129 140 123 140 117 Z" fill="#33211A" stroke="none"/>
  <g fill="none" stroke-width="{inner:.2f}">
    <path d="M 150 129 L 150 134"/>
    <path d="M 150 134 C 146 144 134 144 131 135"/>
    <path d="M 150 134 C 154 144 166 144 169 135"/>
  </g>
</g>"""


def foot(cx, top, width=36.0, height=22.0):
    """A hind foot seen from the front, with two toe lines."""
    l, r, b = cx - width / 2, cx + width / 2, top + height
    return (f'<path d="M {cx:.1f} {top:.1f} C {r - 4:.1f} {top:.1f} {r:.1f} {top + 7:.1f} {r:.1f} {b - 8:.1f} '
            f'C {r:.1f} {b - 3:.1f} {r - 4:.1f} {b:.1f} {r - 10:.1f} {b:.1f} L {l + 10:.1f} {b:.1f} '
            f'C {l + 4:.1f} {b:.1f} {l:.1f} {b - 3:.1f} {l:.1f} {b - 8:.1f} '
            f'C {l:.1f} {top + 7:.1f} {l + 4:.1f} {top:.1f} {cx:.1f} {top:.1f} Z" fill="{BROWN}"/>\n'
            f'<g fill="none" stroke-width="4">'
            f'<path d="M {cx - 6:.1f} {b - 9:.1f} L {cx - 6:.1f} {b - 1:.1f}"/>'
            f'<path d="M {cx + 6:.1f} {b - 9:.1f} L {cx + 6:.1f} {b - 1:.1f}"/></g>')


def document(title, comment, body):
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<!-- {comment}
     Brand art, GPL-3.0-or-later. Generated by tools/brand/posture-art.py. -->
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 256 256" width="256" height="256">
  <title>{title}</title>
  <g stroke="{OUTLINE}" stroke-width="{STROKE:g}" stroke-linejoin="round" stroke-linecap="round">
{body}
  </g>
</svg>
"""


# ── Standing: on its hind legs, arms up, tail down to the ground ─────────────

def shadow(cx, cy, rx):
    return f'<ellipse cx="{cx}" cy="{cy}" rx="{rx}" ry="5" fill="{OUTLINE}" fill-opacity="0.14" stroke="none"/>'


def standing():
    parts = [
        shadow(118, 247, 92),
        "<!-- Tail: behind the body, down to the ground and curling up -->",
        tail([((108, 190), (84, 214), (62, 240), (40, 236)),
              ((40, 236), (22, 231), (16, 212), (24, 196))]),
        "<!-- Arms, raised -->",
        limb([((104, 124), (92, 108), (76, 92), (58, 76))], half=12),
        limb([((152, 124), (164, 108), (180, 92), (198, 76))], half=12),
        "<!-- Legs -->",
        limb([((112, 186), (111, 204), (110, 218), (110, 230))], half=13),
        limb([((144, 186), (145, 204), (146, 218), (146, 230))], half=13),
        "<!-- Body -->",
        f'<path d="M 128 90 C 153 90 164 108 167 130 C 171 156 171 180 162 194 C 155 204 142 207 128 207 '
        f'C 114 207 101 204 94 194 C 85 180 85 156 89 130 C 92 108 103 90 128 90 Z" fill="{BROWN}"/>',
        "<!-- Feet -->",
        foot(106, 224),
        foot(150, 224),
        "<!-- Head -->",
        head(),
    ]
    return document("stopme: stand up",
                    "stopme sit/stand reminder: the panda standing up.",
                    "\n".join(parts))


# ── Sitting: on a stool, paws on its knees ───────────────────────────────────

def sitting():
    stool = (
        "<!-- Stool -->\n"
        f'<g fill="{BLUE}">'
        f'<path d="M 74 190 L 64 246 L 78 246 L 88 190 Z"/>'
        f'<path d="M 182 190 L 192 246 L 178 246 L 168 190 Z"/>'
        "</g>\n"
        f'<path d="M 70 224 L 186 224 L 187 232 L 69 232 Z" fill="{BLUE_DARK}"/>\n'
        f'<path d="M 56 184 C 56 178 60 174 66 174 L 190 174 C 196 174 200 178 200 184 '
        f'L 200 188 C 200 194 196 198 190 198 L 66 198 C 60 198 56 194 56 188 Z" fill="{BLUE}"/>'
    )
    parts = [
        shadow(128, 247, 84),
        "<!-- Tail: hanging down behind the stool -->",
        tail([((104, 156), (70, 160), (44, 180), (38, 206)),
              ((38, 206), (33, 228), (38, 242), (56, 242))]),
        stool,
        "<!-- Body, sitting on the seat -->",
        f'<path d="M 128 90 C 156 90 170 110 173 136 C 175 156 176 172 168 180 C 160 187 144 188 128 188 '
        f'C 112 188 96 187 88 180 C 80 172 81 156 83 136 C 86 110 100 90 128 90 Z" fill="{BROWN}"/>',
        "<!-- Arms, paws on the seat -->",
        limb([((100, 118), (90, 136), (84, 154), (82, 170))], half=11),
        limb([((156, 118), (166, 136), (172, 154), (174, 170))], half=11),
        "<!-- Legs, hanging over the edge of the seat -->",
        limb([((112, 178), (111, 194), (110, 206), (110, 214))], half=13),
        limb([((144, 178), (145, 194), (146, 206), (146, 214))], half=13),
        foot(108, 210),
        foot(148, 210),
        "<!-- Head -->",
        head(),
    ]
    return document("stopme: sit down",
                    "stopme sit/stand reminder: the panda sitting on a stool.",
                    "\n".join(parts))


def main() -> None:
    dest = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "ui" / "data" / "images"
    for name, svg in (("stopme-panda-standing.svg", standing()),
                      ("stopme-panda-sitting.svg", sitting())):
        (dest / name).write_text(svg)
        print(f"  {dest / name}")


if __name__ == "__main__":
    main()
