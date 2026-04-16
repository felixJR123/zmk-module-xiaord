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

For a cleaner photo background, hide the home screen date and time:

```conf
CONFIG_XIAORD_REMOVE_DATE_TIME=y
```

## Replacing Custom Backgrounds

The easiest method is documented in `tools/README.md`. It includes fork, clone, picture-folder selection, commit, and push steps.

To open a folder picker and choose any folder containing your pictures, run this from the repo root:

```powershell
powershell -ExecutionPolicy Bypass -File tools/convert_backgrounds.ps1 -ChooseFolder
```

The converter uses the first one to three JPG/PNG images it finds and writes `bg4`, `bg5`, and `bg6` as needed. If you only choose one picture, it only updates `bg4`.

To use a typed path instead of the folder picker:

```powershell
powershell -ExecutionPolicy Bypass -File tools/convert_backgrounds.ps1 -SourceDir "$env:USERPROFILE\Pictures\Dongle Backgrounds"
```

Or convert one photo at a time:

```powershell
python -m pip install pillow
python tools/convert_xiaord_bg.py src/display/ui/bg/source/background1.jpg 4 --center-x 0.50 --center-y 0.52 --zoom 1.20
python tools/convert_xiaord_bg.py src/display/ui/bg/source/background2.jpg 5 --center-x 0.50 --center-y 0.48 --zoom 1.15
python tools/convert_xiaord_bg.py src/display/ui/bg/source/background3.jpg 6 --center-x 0.50 --center-y 0.50 --zoom 1.20
```

The round screen hides the corners. Use these options to keep the main subject in the center:

- `--center-x`: move the crop left/right. Smaller moves left, larger moves right.
- `--center-y`: move the crop up/down. Smaller moves up, larger moves down.
- `--zoom`: larger values crop tighter around the main subject.

After running the script, commit the generated `bg4.png`, `bg4.c`, `bg5.png`, `bg5.c`, `bg6.png`, and `bg6.c` files that changed.
