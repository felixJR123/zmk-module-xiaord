#!/usr/bin/env python3
"""Convert a photo into a Xiaord 240x240 LVGL RGB565 background.

PNG files work with Python's standard library. JPG/JPEG files require Pillow:
    python -m pip install pillow

Example:
    python tools/convert_xiaord_bg.py src/display/ui/bg/source/background1.jpg --center-x 0.50 --center-y 0.48 --zoom 1.15
"""

from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path

SIZE = 240
PNG_SIG = b"\x89PNG\r\n\x1a\n"


def clamp(value: float, lo: float, hi: float) -> float:
    return max(lo, min(hi, value))


def crop_square_box(width: int, height: int, center_x: float, center_y: float, zoom: float) -> tuple[int, int, int]:
    crop_size = min(width, height) / zoom
    crop_size = clamp(crop_size, 1, min(width, height))

    cx = clamp(center_x, 0.0, 1.0) * width
    cy = clamp(center_y, 0.0, 1.0) * height

    left = round(clamp(cx - crop_size / 2, 0, width - crop_size))
    top = round(clamp(cy - crop_size / 2, 0, height - crop_size))
    size = round(crop_size)

    return left, top, size


def rgb565_bytes(pixels: list[tuple[int, int, int]]) -> bytes:
    out = bytearray()
    for r, g, b in pixels:
        value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        out.append(value & 0xFF)
        out.append((value >> 8) & 0xFF)
    return bytes(out)


def paeth_predictor(a: int, b: int, c: int) -> int:
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def read_png_rgb(path: Path) -> tuple[int, int, list[tuple[int, int, int]]]:
    data = path.read_bytes()
    if not data.startswith(PNG_SIG):
        raise ValueError("not a PNG file")

    pos = len(PNG_SIG)
    width = height = color_type = bit_depth = interlace = None
    idat = bytearray()

    while pos < len(data):
        length = struct.unpack(">I", data[pos : pos + 4])[0]
        pos += 4
        chunk_type = data[pos : pos + 4]
        pos += 4
        chunk = data[pos : pos + length]
        pos += length + 4  # skip CRC

        if chunk_type == b"IHDR":
            width, height, bit_depth, color_type, _compression, _filter, interlace = struct.unpack(">IIBBBBB", chunk)
        elif chunk_type == b"IDAT":
            idat.extend(chunk)
        elif chunk_type == b"IEND":
            break

    if width is None or height is None or color_type is None or bit_depth is None or interlace is None:
        raise ValueError("missing PNG IHDR")
    if bit_depth != 8 or color_type not in (2, 6) or interlace != 0:
        raise ValueError("only non-interlaced 8-bit RGB/RGBA PNG files are supported without Pillow")

    channels = 4 if color_type == 6 else 3
    stride = width * channels
    raw = zlib.decompress(bytes(idat))
    rows: list[bytearray] = []
    src = 0

    for _y in range(height):
        filter_type = raw[src]
        src += 1
        row = bytearray(raw[src : src + stride])
        src += stride
        prev = rows[-1] if rows else bytearray(stride)

        for x in range(stride):
            left = row[x - channels] if x >= channels else 0
            up = prev[x]
            up_left = prev[x - channels] if x >= channels else 0
            if filter_type == 1:
                row[x] = (row[x] + left) & 0xFF
            elif filter_type == 2:
                row[x] = (row[x] + up) & 0xFF
            elif filter_type == 3:
                row[x] = (row[x] + ((left + up) // 2)) & 0xFF
            elif filter_type == 4:
                row[x] = (row[x] + paeth_predictor(left, up, up_left)) & 0xFF
            elif filter_type != 0:
                raise ValueError(f"unsupported PNG filter {filter_type}")

        rows.append(row)

    pixels: list[tuple[int, int, int]] = []
    for row in rows:
        for x in range(0, stride, channels):
            pixels.append((row[x], row[x + 1], row[x + 2]))

    return width, height, pixels


def crop_resize_rgb(
    width: int,
    height: int,
    pixels: list[tuple[int, int, int]],
    center_x: float,
    center_y: float,
    zoom: float,
) -> list[tuple[int, int, int]]:
    left, top, crop_size = crop_square_box(width, height, center_x, center_y, zoom)
    out: list[tuple[int, int, int]] = []

    for y in range(SIZE):
        src_y = top + min(crop_size - 1, int(y * crop_size / SIZE))
        row_base = src_y * width
        for x in range(SIZE):
            src_x = left + min(crop_size - 1, int(x * crop_size / SIZE))
            out.append(pixels[row_base + src_x])

    return out


def png_chunk(chunk_type: bytes, data: bytes) -> bytes:
    import binascii

    crc = binascii.crc32(chunk_type)
    crc = binascii.crc32(data, crc) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + chunk_type + data + struct.pack(">I", crc)


def write_png_rgb(path: Path, pixels: list[tuple[int, int, int]]) -> None:
    rows = bytearray()
    for y in range(SIZE):
        rows.append(0)
        for r, g, b in pixels[y * SIZE : (y + 1) * SIZE]:
            rows.extend((r, g, b))

    ihdr = struct.pack(">IIBBBBB", SIZE, SIZE, 8, 2, 0, 0, 0)
    path.write_bytes(
        PNG_SIG
        + png_chunk(b"IHDR", ihdr)
        + png_chunk(b"IDAT", zlib.compress(bytes(rows), 9))
        + png_chunk(b"IEND", b"")
    )


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


def write_rgb565_file(path: Path, pixels: list[tuple[int, int, int]]) -> None:
    path.write_bytes(rgb565_bytes(pixels))


def convert_with_pillow(source: Path, center_x: float, center_y: float, zoom: float) -> list[tuple[int, int, int]] | None:
    try:
        from PIL import Image, ImageOps
    except ModuleNotFoundError:
        return None

    img = Image.open(source)
    img = ImageOps.exif_transpose(img)
    left, top, crop_size = crop_square_box(img.size[0], img.size[1], center_x, center_y, zoom)
    square = img.crop((left, top, left + crop_size, top + crop_size))
    final = square.resize((SIZE, SIZE), Image.Resampling.LANCZOS).convert("RGB")
    return list(final.get_flattened_data() if hasattr(final, "get_flattened_data") else final.getdata())


def convert_source(source: Path, center_x: float, center_y: float, zoom: float) -> list[tuple[int, int, int]]:
    pixels = convert_with_pillow(source, center_x, center_y, zoom)
    if pixels is not None:
        return pixels

    if source.suffix.lower() == ".png":
        width, height, png_pixels = read_png_rgb(source)
        return crop_resize_rgb(width, height, png_pixels, center_x, center_y, zoom)

    raise RuntimeError("Pillow is required for JPG/JPEG files. Use a PNG file for GitHub Actions builds without pip.")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path, help="Input photo")
    parser.add_argument("bg_num", type=int, nargs="?", default=4, choices=[4], help="Background number; only bg4 is supported")
    parser.add_argument("--center-x", type=float, default=0.5, help="Crop center X, 0.0 left to 1.0 right")
    parser.add_argument("--center-y", type=float, default=0.5, help="Crop center Y, 0.0 top to 1.0 bottom")
    parser.add_argument("--zoom", type=float, default=1.0, help="Crop zoom, larger is closer")
    parser.add_argument("--out-dir", type=Path, default=Path("src/display/ui/bg"))
    parser.add_argument("--rgb565-out", type=Path, help="Optional raw RGB565 output path for SD-card backgrounds")
    args = parser.parse_args()

    pixels = convert_source(args.source, args.center_x, args.center_y, args.zoom)

    args.out_dir.mkdir(parents=True, exist_ok=True)
    png_path = args.out_dir / f"bg{args.bg_num}.png"
    c_path = args.out_dir / f"bg{args.bg_num}.c"

    write_png_rgb(png_path, pixels)
    write_c_file(c_path, args.bg_num, rgb565_bytes(pixels))

    print(f"Wrote {png_path}")
    print(f"Wrote {c_path}")

    if args.rgb565_out:
        args.rgb565_out.parent.mkdir(parents=True, exist_ok=True)
        write_rgb565_file(args.rgb565_out, pixels)
        print(f"Wrote {args.rgb565_out}")


if __name__ == "__main__":
    main()
