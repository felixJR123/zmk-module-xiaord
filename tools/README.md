# Xiaord Background Converter

The module can convert one JPG or PNG into a 240x240 LVGL RGB565 firmware
background. Most users do not need to run the converter manually; the ZMK build
can do it automatically from the keyboard config repo.

## Recommended GitHub Actions Setup

Put one PNG image in the keyboard config repo:

```text
config/xiaord-bg/01-background.png
```

Then enable the custom slot in the keyboard `.conf`:

```conf
CONFIG_XIAORD_BG_1=n
CONFIG_XIAORD_BG_2=n
CONFIG_XIAORD_BG_3=n
CONFIG_XIAORD_BG_4=y
```

The default ZMK GitHub Actions checkout places the keyboard config at
`/tmp/zmk-config/config`, so the module automatically looks for:

```text
/tmp/zmk-config/config/xiaord-bg
```

If the folder has more than one image, the first filename in sorted order is
used. Name the preferred image with a leading number:

```text
01-background.jpg
```

PNG files work in the default GitHub Actions environment without extra
dependencies. JPG/JPEG files require Pillow, which the default runner may not
have. If the image is missing or conversion fails, the build falls back to
`BG_1` instead of failing.

In the build log, look for lines like:

```text
XIAORD_BG_4: looking for background image in /tmp/zmk-config/config/xiaord-bg
XIAORD_BG_4: converting /tmp/zmk-config/config/xiaord-bg/01-background.png
XIAORD_BG_4: generated .../src/display/ui/bg/bg4.c
```

## Privacy

Pictures are stored in the keyboard config repo.

Keep that repo private if the image is personal, such as family photos. If the
image is not private, the keyboard config repo can be public.

## Custom Folder

To use a different folder in the keyboard config repo:

```conf
CONFIG_XIAORD_BG_4=y
CONFIG_XIAORD_BG_4_SOURCE_DIR="my-background-folder"
```

Relative paths are resolved from the keyboard config repo.

For a local Windows-only build, an absolute path also works:

```conf
CONFIG_XIAORD_BG_4=y
CONFIG_XIAORD_BG_4_SOURCE_DIR="C:/Users/YOUR_NAME/Pictures/Dongle Backgrounds"
```

## Preview Or Convert Manually

To preview the crop before building, clone this module locally and run:

```powershell
powershell -ExecutionPolicy Bypass -File tools/convert_backgrounds.ps1 -SourceDir "$env:USERPROFILE\Pictures\Dongle Backgrounds"
```

The converter writes ignored local files:

```text
src/display/ui/bg/bg4.png
src/display/ui/bg/bg4.c
```

Open `bg4.png` to check the crop. Do not commit these generated files if they
contain a private image.

To convert one image directly:

```powershell
python tools/convert_xiaord_bg.py path/to/background.png --center-x 0.50 --center-y 0.50 --zoom 1.00
```

If Python says Pillow is missing:

```powershell
python -m pip install --user pillow
```

## Adjust Crop

The round screen hides the corners, so the important part should be near the
center. In `tools/convert_backgrounds.ps1`, adjust:

```powershell
$CenterX = "0.54"
$CenterY = "0.50"
$Zoom = "1.00"
```

- `CenterX`: smaller moves the crop left, larger moves it right.
- `CenterY`: smaller moves the crop up, larger moves it down.
- `Zoom`: larger zooms in closer, smaller shows more of the picture.

## Preparing a microSD Card

Use `xiaord_sd_backgrounds.py` when you want many runtime backgrounds on the
XIAO Round Display microSD card instead of one compiled `BG_4` image.

You can use a local clone of this module, or download only these files into one
folder:

```text
xiaord_sd_backgrounds.py
convert_xiaord_bg.py
SD_BACKGROUND_STEPS.txt
```

```powershell
python tools\xiaord_sd_backgrounds.py E:\ --source C:\Pictures\xiaord
```

Quote paths that contain spaces:

```powershell
python xiaord_sd_backgrounds.py E:\ --source "C:\Users\fhern\Pictures\Xiaord Backgrounds"
```

The tool creates this card layout:

```text
xiaord-bg/
  raw/        original JPG/PNG files
  converted/  bg001.rgb565, bg002.rgb565, ...
  tools/      converter scripts copied onto the SD card
```

After the first setup, you can add new pictures to `E:\xiaord-bg\raw` and run
the copied tool from the card:

```powershell
python E:\xiaord-bg\tools\xiaord_sd_backgrounds.py E:\
```

The firmware expects the converted files in `/SD:/xiaord-bg/converted` when
`CONFIG_XIAORD_BG_SD=y`.

Keep one compiled background enabled as a fallback. For example, this uses SD
backgrounds when available and falls back to `BG_4`:

```conf
CONFIG_XIAORD_BG_1=n
CONFIG_XIAORD_BG_2=n
CONFIG_XIAORD_BG_3=n
CONFIG_XIAORD_BG_4=y
CONFIG_XIAORD_BG_SD=y
CONFIG_XIAORD_BG_SD_VOLUME_NAME="SD"
```

`CONFIG_XIAORD_BG_SD_MAX_FILES=999` is only the maximum number of files to
index. It is fine if the SD card has fewer pictures.

If the firmware still falls back to the compiled background while the SD card is
inserted, format the card as FAT32, confirm the converted files are in
`xiaord-bg/converted`, and keep `CONFIG_XIAORD_BG_SD_VOLUME_NAME="SD"` unless
your board registers the SD disk with a different name. Insert the card before
the keyboard boots.
