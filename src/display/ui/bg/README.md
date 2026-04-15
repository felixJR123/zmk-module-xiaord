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

If exactly one background is enabled, it stays static. If more than one is enabled, the dongle rotates through only the enabled images.

```conf
CONFIG_XIAORD_BG_4=y
CONFIG_XIAORD_BG_5=y
CONFIG_XIAORD_BG_6=y
CONFIG_XIAORD_BG_ROTATE_INTERVAL_MIN=5
```

## Replacing Family Photos

Put the original photos in this folder:

```text
src/display/ui/bg/source/
```

Generate firmware assets with:

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