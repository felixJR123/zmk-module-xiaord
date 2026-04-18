#!/usr/bin/env python3
"""Prepare a microSD card for Xiaord runtime backgrounds.

The card layout created by this tool is:

    xiaord-bg/
      raw/        original JPG/PNG files
      converted/  bg001.rgb565, bg002.rgb565, ...
      tools/      this converter and its helper

Examples:
    python tools/xiaord_sd_backgrounds.py E:\\ --source C:\\Pictures\\xiaord
    python E:\\xiaord-bg\\tools\\xiaord_sd_backgrounds.py E:\\
"""

from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

from convert_xiaord_bg import SIZE, convert_source, write_rgb565_file

IMAGE_EXTS = {".jpg", ".jpeg", ".png"}
RAW_DIR = Path("xiaord-bg/raw")
CONVERTED_DIR = Path("xiaord-bg/converted")
TOOLS_DIR = Path("xiaord-bg/tools")


def image_sources(path: Path) -> list[Path]:
    if path.is_file():
        return [path] if path.suffix.lower() in IMAGE_EXTS else []

    sources = [p for p in path.iterdir() if p.is_file() and p.suffix.lower() in IMAGE_EXTS]
    return sorted(sources, key=lambda p: p.name.lower())


def copy_tooling(sd_root: Path) -> None:
    tools_dir = sd_root / TOOLS_DIR
    tools_dir.mkdir(parents=True, exist_ok=True)

    this_file = Path(__file__).resolve()
    helper = this_file.with_name("convert_xiaord_bg.py")
    steps = this_file.with_name("SD_BACKGROUND_STEPS.txt")
    shutil.copy2(this_file, tools_dir / this_file.name)
    if helper.exists():
        shutil.copy2(helper, tools_dir / helper.name)
    if steps.exists():
        shutil.copy2(steps, tools_dir / steps.name)
    else:
        readme = tools_dir / "README.txt"
        readme.write_text(
            "\n".join(
                [
                    "Xiaord SD background tools",
                    "",
                    "Put JPG or PNG files in ../raw, then run:",
                    "  python xiaord_sd_backgrounds.py <sd-root>",
                    "",
                    f"Converted files are written to ../converted as {SIZE}x{SIZE} RGB565.",
                    "",
                ]
            ),
            encoding="utf-8",
        )


def prepare_sd(
    sd_root: Path,
    source: Path | None,
    center_x: float,
    center_y: float,
    zoom: float,
    keep_existing: bool,
) -> int:
    sd_root = sd_root.resolve()
    raw_dir = sd_root / RAW_DIR
    converted_dir = sd_root / CONVERTED_DIR
    raw_dir.mkdir(parents=True, exist_ok=True)
    converted_dir.mkdir(parents=True, exist_ok=True)
    copy_tooling(sd_root)

    if source:
        for src in image_sources(source):
            dest = raw_dir / src.name
            if src.resolve() != dest.resolve():
                shutil.copy2(src, dest)

    sources = image_sources(raw_dir)
    if not sources:
        print(f"No JPG/PNG files found in {raw_dir}", file=sys.stderr)
        return 1

    if not keep_existing:
        for old in converted_dir.glob("bg*.rgb565"):
            old.unlink()

    for idx, src in enumerate(sources, start=1):
        out = converted_dir / f"bg{idx:03d}.rgb565"
        pixels = convert_source(src, center_x, center_y, zoom)
        write_rgb565_file(out, pixels)
        print(f"{src.name} -> {out.relative_to(sd_root)}")

    print(f"Prepared {len(sources)} background(s) in {converted_dir}")
    return 0


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("sd_root", type=Path, help="Mounted SD card root, such as E:\\")
    parser.add_argument("--source", type=Path, help="Optional folder or image to copy into xiaord-bg/raw first")
    parser.add_argument("--center-x", type=float, default=0.5, help="Crop center X, 0.0 left to 1.0 right")
    parser.add_argument("--center-y", type=float, default=0.5, help="Crop center Y, 0.0 top to 1.0 bottom")
    parser.add_argument("--zoom", type=float, default=1.0, help="Crop zoom, larger is closer")
    parser.add_argument("--keep-existing", action="store_true", help="Do not delete old converted bg*.rgb565 files")
    args = parser.parse_args()

    raise SystemExit(
        prepare_sd(
            args.sd_root,
            args.source,
            args.center_x,
            args.center_y,
            args.zoom,
            args.keep_existing,
        )
    )


if __name__ == "__main__":
    main()
