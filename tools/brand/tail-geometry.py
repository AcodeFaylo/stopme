#!/usr/bin/env python3
"""Emit the mascot tail as plain SVG paths.

The tail is a constant-width band swept along a Bezier centreline. Drawing it
with a thick stroke and a <mask> would be shorter, but Qt's SVG renderer — which
draws stopme-panda.svg in the about and update dialogs — supports neither masks
nor SVG2 href on <use>, so the outline and each colour band are emitted as
explicit closed polygons instead.

    python3 tools/brand/tail-geometry.py

Paste the printed paths into ui/data/images/stopme-panda.svg.
"""

import math

SEGS = [((66, 112), (28, 152), (30, 202), (70, 222)),
        ((70, 222), (82, 228), (94, 230), (104, 227))]
HALF = 19.0     # half the tail width
BAND = 19.0     # length of one dark band, along the tail
STEP = 4.0      # polyline resolution


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


_table, _total, _prev = [], 0.0, bez(SEGS[0], 0)
for _i in range(2001):
    _u = _i / 2000 * len(SEGS)
    _si = min(int(_u), len(SEGS) - 1)
    _p = bez(SEGS[_si], _u - _si)
    if _i:
        _total += math.dist(_p, _prev)
    _table.append((_total, _si, _u - _si, _p))
    _prev = _p
LENGTH = _total


def at(s):
    """Point and unit tangent at arc length `s`."""
    s = max(0.0, min(LENGTH, s))
    lo, hi = 0, len(_table) - 1
    while lo < hi:
        mid = (lo + hi) // 2
        if _table[mid][0] < s:
            lo = mid + 1
        else:
            hi = mid
    _, si, t, p = _table[lo]
    dx, dy = dbez(SEGS[si], t)
    n = math.hypot(dx, dy)
    return p, (dx / n, dy / n)


def off(s, d):
    """Point `d` to the side of the centreline at arc length `s`."""
    (x, y), (tx, ty) = at(s)
    return (x - ty * d, y + tx * d)


def cap(s, backwards):
    """Semicircular end cap at `s`, from the -HALF side round to +HALF."""
    (x, y), (tx, ty) = at(s)
    base = math.atan2(tx, -ty)          # direction of the +HALF normal
    sign = -1 if backwards else 1
    return [(x + HALF * math.cos(base + sign * math.pi * k / 12),
             y + HALF * math.sin(base + sign * math.pi * k / 12))
            for k in range(13)]


def side(a, b, d):
    n = max(2, int(abs(b - a) / STEP) + 1)
    return [off(a + (b - a) * i / n, d) for i in range(n + 1)]


def closed(points):
    head, *rest = points
    return (f"M {head[0]:.1f} {head[1]:.1f} L "
            + " L ".join(f"{x:.1f} {y:.1f}" for x, y in rest) + " Z")


def outline():
    return closed(side(0, LENGTH, HALF)
                  + cap(LENGTH, backwards=True)[::-1]
                  + side(LENGTH, 0, -HALF)
                  + cap(0, backwards=True))


def band(a, b):
    return closed(side(a, b, HALF) + side(b, a, -HALF))


if __name__ == "__main__":
    print(f"<!-- centreline length {LENGTH:.1f} -->")
    print(f'TAIL="{outline()}"')
    for k in range(4):
        a = 20 + 2 * BAND * k
        print(f'BAND{k}="{band(a, min(a + BAND, LENGTH))}"')
