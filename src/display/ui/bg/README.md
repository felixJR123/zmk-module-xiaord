# Xiaord Background Images

The built-in backgrounds are compiled from this module. The custom background
slot, `CONFIG_XIAORD_BG_4`, is generated during the keyboard build from an image
stored in the keyboard config repo.

## Default Keyboard Repo Layout

For the default ZMK GitHub Actions workflow, place one PNG here:

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

If the folder has more than one image, the first filename in sorted order is
used. Prefix the one you want with `01-`.

PNG works in GitHub Actions without extra dependencies. JPG/JPEG files require
Pillow, which the default runner may not have.

If no image is found or conversion fails, the firmware falls back to `BG_1`.

## Privacy

Pictures live in the keyboard config repo, not this public module. Keep the
keyboard config repo private if the image is private or sensitive.
If the image is meant to be public, the keyboard config repo can be public.

## Custom Folder

To use a different folder inside the keyboard config repo:

```conf
CONFIG_XIAORD_BG_4=y
CONFIG_XIAORD_BG_4_SOURCE_DIR="my-background-folder"
```

Relative paths are resolved from the keyboard config repo.

## Runtime microSD Backgrounds

To store many pictures on the XIAO Round Display microSD card, use SD mode
instead of compiling photos into the firmware:

```conf
CONFIG_XIAORD_BG_1=y
CONFIG_XIAORD_BG_2=n
CONFIG_XIAORD_BG_3=n
CONFIG_XIAORD_BG_4=n
CONFIG_XIAORD_BG_SD=y
CONFIG_XIAORD_BG_SD_VOLUME_NAME="SD"
CONFIG_XIAORD_BG_SD_ROTATE_MS=60000
CONFIG_XIAORD_BG_SD_RETRY_MS=5000
```

Keep one compiled background enabled as the fallback. Use `BG_4=y` instead of
`BG_1=y` if you want your custom compiled image as the fallback.

Prepare the card with:

```powershell
python tools\xiaord_sd_backgrounds.py E:\ --source C:\Path\To\Pictures
```

You can also download only the converter files instead of cloning the full
module. Keep these files together in one folder:

```text
xiaord_sd_backgrounds.py
convert_xiaord_bg.py
SD_BACKGROUND_STEPS.txt
```

Use quotes around paths with spaces:

```powershell
python xiaord_sd_backgrounds.py E:\ --source "C:\Path With Spaces\Xiaord Backgrounds"
```

This creates:

```text
xiaord-bg/
  raw/        original JPG/PNG files
  converted/  bg001.rgb565, bg002.rgb565, ...
  tools/      converter scripts copied onto the SD card
```

The firmware reads from `/SD:/xiaord-bg/converted` by default. Slide left/right
cycles pictures when `CONFIG_XIAORD_BG_SD_GESTURES=y`.

If the SD card is inserted but no SD backgrounds load, format it as FAT32,
confirm the files are in `xiaord-bg/converted`, and keep
`CONFIG_XIAORD_BG_SD_VOLUME_NAME="SD"` unless your board uses a different disk
name.

`CONFIG_XIAORD_BG_SD_MAX_FILES=999` is only a maximum. Fewer pictures are fine;
the firmware cycles whatever converted files it finds.
