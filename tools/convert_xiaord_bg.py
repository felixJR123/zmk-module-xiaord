#!/usr/bin/env python3
"""Convert a photo into a Xiaord 240x240 LVGL RGB565 background.

Requires Pillow:
    python -m pip install pillow

Example:
    python tools/convert_xiaord_bg.py src/display/ui/bg/source/family1.jpg 4 --center-x 0.50 --center-y 0.48 --zoom 1.15
"""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageOps

SIZE = 240


def clamp(value: float, lo: float, hi: float) -> float:
    return max(lo, min(hi, value))


def crop_square(img: Image.Image, center_x: float, center_y: float, zoom: float) -> Image.Image:
    width, height = img.size
    crop_size = min(width, height) / zoom
    crop_size = clamp(crop_size, 1, min(width, height))

    cx = clamp(center_x, 0.0, 1.0) * width
    cy = clamp(center_y, 0.0, 1.0) * height

    left = clamp(cx - crop_size / 2, 0, width - crop_size)
    top = clamp(cy - crop_size / 2, 0, height - crop_size)
    right = left + crop_size
    bottom = top + crop_size

    return img.crop((round(left), round(top), round(right), round(bottom)))


def rgb565_bytes(img: Image.Image) -> bytes:
    out = bytearray()
    rgb = img.convert("RGB")
    pixels = rgb.get_flattened_data() if hasattr(rgb, "get_flattened_data") else rgb.getdata()
    for r, g, b in pixels:
        value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        out.append(value & 0xFF)
        out.append((value >> 8) & 0xFF)
    return bytes(out)


def write_c_file(path: Path, bg_num: int, data: bytes) -> None:
    attr = f"LV_ATTRIBUTE_IMAGE_BG{bg_num}"
    symbol = f"bg{bg_num}_map"
    img = f"img_bg_{bg_num}"

    lines = [
        "#ifdef __has_include",
        "    #if __has_include(\"lvgl.h\")",
        "        #ifndef LV_LVGL_H_INCLUDE_SIMPLE",
        "            #define LV_LVGL_H_INCLUDE_SIMPLE",
        "        #endif",
        "    #endif",
        "#endif",
        "",
        "#if defined(LV_LVGL_H_INCLUDE_SIMPLE)",
        "    #include \"lvgl.h\"",
        "#else",
        "    #include \"lvgl/lvgl.h\"",
        "#endif",
        "",
        "#ifndef LV_ATTRIBUTE_MEM_ALIGN",
        "#define LV_ATTRIBUTE_MEM_ALIGN",
        "#endif",
        "",
        f"#ifndef {attr}",
        f"#define {attr}",
        "#endif",
        "",
        f"const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST {attr} uint8_t {symbol}[] = {{",
    ]

    for i in range(0, len(data), 16):
        chunk = data[i : i + 16]
        lines.append("  " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")

    lines.extend(
        [
            "};",
            "",
            f"const lv_image_dsc_t {img} = {{",
            "  .header.cf = LV_COLOR_FORMAT_RGB565,",
            "  .header.magic = LV_IMAGE_HEADER_MAGIC,",
            f"  .header.w = {SIZE},",
            f"  .header.h = {SIZE},",
            f"  .data_size = {SIZE * SIZE} * 2,",
            f"  .data = {symbol},",
            "};",
            "",
        ]
    )
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path, help="Input photo")
    parser.add_argument("bg_num", type=int, choices=range(1, 10), help="Background number, e.g. 4")
    parser.add_argument("--center-x", type=float, default=0.5, help="Crop center X, 0.0 left to 1.0 right")
    parser.add_argument("--center-y", type=float, default=0.5, help="Crop center Y, 0.0 top to 1.0 bottom")
    parser.add_argument("--zoom", type=float, default=1.0, help="Crop zoom, larger is closer")
    parser.add_argument("--out-dir", type=Path, default=Path("src/display/ui/bg"))
    args = parser.parse_args()

    img = Image.open(args.source)
    img = ImageOps.exif_transpose(img)
    square = crop_square(img, args.center_x, args.center_y, args.zoom)
    final = square.resize((SIZE, SIZE), Image.Resampling.LANCZOS)

    args.out_dir.mkdir(parents=True, exist_ok=True)
    png_path = args.out_dir / f"bg{args.bg_num}.png"
    c_path = args.out_dir / f"bg{args.bg_num}.c"

    final.save(png_path)
    write_c_file(c_path, args.bg_num, rgb565_bytes(final))

    print(f"Wrote {png_path}")
    print(f"Wrote {c_path}")


if __name__ == "__main__":
    main()