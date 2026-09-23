#!/usr/bin/env python3
"""Check that the Mach-O files in a macOS build run on the Macs it targets.

Walks the given files and directories (an app bundle, or the prefix that
macos-deps.sh installs into) and checks every Mach-O file and static
library in them:

- it contains code for --arch;
- none of its code needs a newer macOS than --min-macos;
- with --bundle, it only loads libraries from the system or from inside
  the bundle, and every library it loads from the bundle is there.

The build machine runs a newer macOS than the one targeted, so an app that
starts there can still fail to start on an older Mac; this catches that.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

MACHO_MAGICS = {
    b"\xfe\xed\xfa\xce", b"\xce\xfa\xed\xfe",  # 32-bit
    b"\xfe\xed\xfa\xcf", b"\xcf\xfa\xed\xfe",  # 64-bit
    b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca",  # universal
}
ARCHIVE_MAGIC = b"!<arch>\n"
SYSTEM_PREFIXES = ("/System/Library/", "/usr/lib/")


def kind(path: Path) -> str | None:
    try:
        with path.open("rb") as f:
            head = f.read(8)
    except OSError:
        return None
    if head == ARCHIVE_MAGIC:
        return "archive"
    if head[:4] in MACHO_MAGICS:
        return "macho"
    return None


def tool(*args: str) -> str:
    return subprocess.run(args, check=True, capture_output=True, text=True).stdout


def version(text: str) -> tuple[int, ...]:
    return tuple(int(part) for part in text.split("."))


def minimum_versions(path: Path, arch: str) -> list[str]:
    """The minimum macOS of every object in the file (one per archive member)."""
    found = []
    wanted = None
    for line in tool("otool", "-arch", arch, "-l", str(path)).splitlines():
        words = line.split()
        if words[:2] == ["cmd", "LC_BUILD_VERSION"]:
            wanted = "minos"
        elif words[:2] == ["cmd", "LC_VERSION_MIN_MACOSX"]:
            wanted = "version"
        elif wanted and len(words) == 2 and words[0] == wanted:
            found.append(words[1])
            wanted = None
    return found


def dependencies(path: Path, arch: str) -> list[str]:
    own_id = tool("otool", "-arch", arch, "-D", str(path)).splitlines()[1:]
    deps = []
    for line in tool("otool", "-arch", arch, "-L", str(path)).splitlines()[1:]:
        name = line.strip().split(" (compatibility")[0]
        if name and name not in own_id:
            deps.append(name)
    return deps


def resolve(dep: str, path: Path, bundle: Path) -> Path | None:
    """Where a bundle-relative load command points; None for system libraries."""
    if dep.startswith("@rpath/"):
        # macdeployqt points every rpath at Contents/Frameworks.
        return bundle / "Contents" / "Frameworks" / dep[len("@rpath/"):]
    if dep.startswith("@executable_path/"):
        return bundle / "Contents" / "MacOS" / dep[len("@executable_path/"):]
    if dep.startswith("@loader_path/"):
        return path.parent / dep[len("@loader_path/"):]
    return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--arch", required=True)
    parser.add_argument("--min-macos", required=True)
    parser.add_argument("--bundle", action="store_true",
                        help="the paths are app bundles: also check what they load")
    parser.add_argument("paths", nargs="+", type=Path)
    args = parser.parse_args()

    limit = version(args.min_macos)
    problems = []
    checked = 0

    for root in args.paths:
        files = [root] if root.is_file() else sorted(
            Path(dirpath) / name for dirpath, _, names in os.walk(root) for name in names)
        for path in files:
            if path.is_symlink():
                continue
            what = kind(path)
            if what is None:
                continue
            checked += 1
            where = path.relative_to(root) if root.is_dir() else path

            archs = tool("lipo", "-archs", str(path)).split()
            if args.arch not in archs:
                problems.append(f"{where}: no {args.arch} code (has {' '.join(archs)})")
                continue

            mins = minimum_versions(path, args.arch)
            if not mins:
                problems.append(f"{where}: no minimum macOS version recorded")
            too_new = sorted({m for m in mins if version(m) > limit}, key=version)
            if too_new:
                problems.append(f"{where}: needs macOS {too_new[-1]}, target is {args.min_macos}")

            if args.bundle and what == "macho":
                for dep in dependencies(path, args.arch):
                    if dep.startswith(SYSTEM_PREFIXES):
                        continue
                    target = resolve(dep, path, root)
                    if target is None:
                        problems.append(f"{where}: loads {dep} from outside the bundle")
                    elif not target.exists():
                        problems.append(f"{where}: loads {dep}, which is not in the bundle")

    print(f"Checked {checked} Mach-O files for {args.arch}, macOS {args.min_macos} and later.")
    for problem in problems:
        print(f"error: {problem}")
    if checked == 0:
        print("error: found no Mach-O files")
        return 1
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
