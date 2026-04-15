# Xiaord Background Images

The display backgrounds are compiled into firmware as 240x240 LVGL RGB565 C files.

## Config Behavior

Each image has its own Kconfig option:

```conf
CONFIG_XIAORD_BG_1=y
CONFIG_XIAORD_BG_2=n
CONFIG_XIAORD_BG_3=n
CONFIG_XIAORD_BG_4=n
CONFIG_XIAORD_BG_5=n
CONFIG_XIAORD_BG_6=n
```

The firmware uses one static background. If more than one background is enabled, the module chooses the first available image in this priority: `BG_4`, `BG_5`, `BG_6`, then `BG_1`, `BG_2`, `BG_3`.

```conf
CONFIG_XIAORD_BG_1=n
CONFIG_XIAORD_BG_2=n
CONFIG_XIAORD_BG_3=n
CONFIG_XIAORD_BG_4=y
CONFIG_XIAORD_BG_5=n
CONFIG_XIAORD_BG_6=n
```

The firmware reliably fits one full-size 240x240 RGB565 photo background. Keep only one `CONFIG_XIAORD_BG_*` option enabled in the keyboard config for the safest build.

## Replacing Family Photos

Put the original photos in this folder:

```text
src/display/ui/bg/source/
```

Generate firmware assets with the helper script. By default it looks in `%USERPROFILE%\OneDrive\Pictures\Dongle Pictures`, takes the first three JPG/PNG images, and writes `bg4`, `bg5`, and `bg6`:

```powershell
powershell -ExecutionPolicy Bypass -File tools/convert_family_backgrounds.ps1
```

To use a different folder without hard-coding a Windows username:

```powershell
powershell -ExecutionPolicy Bypass -File tools/convert_family_backgrounds.ps1 -SourceDir "$env:USERPROFILE\Pictures\Dongle Pictures"
```

Or convert one photo at a time:

```powershell
python -m pip install pillow
python tools/convert_xiaord_bg.py src/display/ui/bg/source/family1.jpg 4 --center-x 0.50 --center-y 0.52 --zoom 1.20
python tools/convert_xiaord_bg.py src/display/ui/bg/source/family2.jpg 5 --center-x 0.50 --center-y 0.48 --zoom 1.15
python tools/convert_xiaord_bg.py src/display/ui/bg/source/family3.jpg 6 --center-x 0.50 --center-y 0.50 --zoom 1.20
```

The round screen hides the corners. Use these options to keep faces in the center:

- `--center-x`: move the crop left/right. Smaller moves left, larger moves right.
- `--center-y`: move the crop up/down. Smaller moves up, larger moves down.
- `--zoom`: larger values crop tighter around the people.

After running the script, commit the generated `bg4.png`, `bg4.c`, `bg5.png`, `bg5.c`, `bg6.png`, and `bg6.c` files.