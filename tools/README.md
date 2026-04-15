# Xiaord Background Converter

These tools turn normal JPG or PNG pictures into firmware background files for the 240x240 round display.

You do not need to know programming to use them. The short version is:

1. Put your pictures in a folder.
2. Run one PowerShell command.
3. Check the preview PNG files.
4. Commit and push the generated files.
5. Enable only one background in your keyboard `.conf` file.

## What The Tools Create

For each background slot, the converter creates two files:

- `bg4.png`, `bg5.png`, or `bg6.png`: a preview image you can open and check.
- `bg4.c`, `bg5.c`, or `bg6.c`: the firmware file ZMK compiles into the dongle.

The firmware currently works best with one full-size custom photo enabled at a time.

## Before You Start

Make sure the repo is open in VS Code or PowerShell at the module folder, for example:

```text
C:\Users\YOUR_NAME\git\zmk-module-xiaord
```

If you are in VS Code:

1. Open the `zmk-module-xiaord` folder.
2. Click `Terminal`.
3. Click `New Terminal`.
4. Use the commands below.

## Easy Method: Convert A Folder Of Pictures

Put up to three JPG or PNG pictures in this folder:

```text
%USERPROFILE%\OneDrive\Pictures\Dongle Pictures
```

Then run:

```powershell
powershell -ExecutionPolicy Bypass -File tools/convert_backgrounds.ps1
```

The tool will use the first three pictures it finds by filename and create:

```text
src/display/ui/bg/bg4.png
src/display/ui/bg/bg4.c
src/display/ui/bg/bg5.png
src/display/ui/bg/bg5.c
src/display/ui/bg/bg6.png
src/display/ui/bg/bg6.c
```

Open the `.png` files to check how the pictures are cropped.

## Use A Different Picture Folder

If your pictures are somewhere else, run the tool with `-SourceDir`:

```powershell
powershell -ExecutionPolicy Bypass -File tools/convert_backgrounds.ps1 -SourceDir "$env:USERPROFILE\Pictures\Dongle Pictures"
```

You can also use a full path, like:

```powershell
powershell -ExecutionPolicy Bypass -File tools/convert_backgrounds.ps1 -SourceDir "D:\My Pictures\Dongle Backgrounds"
```

## Choose Which Background To Use

In your keyboard `.conf` file, enable only one custom background at a time. For example, to use `bg4`:

```conf
CONFIG_XIAORD_BG_1=n
CONFIG_XIAORD_BG_2=n
CONFIG_XIAORD_BG_3=n
CONFIG_XIAORD_BG_4=y
CONFIG_XIAORD_BG_5=n
CONFIG_XIAORD_BG_6=n
```

For cleaner pictures, hide the home screen clock/date:

```conf
CONFIG_XIAORD_REMOVE_DATE_TIME=y
```

The RTC still keeps time while the clock/date is hidden.

## Fix The Crop

The round screen hides the corners, so the important part of the image should be near the center.

Open this file:

```text
tools/convert_backgrounds.ps1
```

Find this section:

```powershell
$CropPresets = @(
    @{ Bg = 4; CenterX = "0.54"; CenterY = "0.50"; Zoom = "1.00" },
    @{ Bg = 5; CenterX = "0.50"; CenterY = "0.42"; Zoom = "1.00" },
    @{ Bg = 6; CenterX = "0.50"; CenterY = "0.42"; Zoom = "1.00" }
)
```

Change these numbers:

- `CenterX`: smaller moves the crop left, larger moves it right.
- `CenterY`: smaller moves the crop up, larger moves it down.
- `Zoom`: larger zooms in closer, smaller shows more of the picture.

Run the converter again after changing the numbers.

## Convert One Picture Manually

Most people should use the folder converter above. This command is useful when you want to convert exactly one file into exactly one slot:

```powershell
python tools/convert_xiaord_bg.py src/display/ui/bg/source/background1.jpg 4 --center-x 0.50 --center-y 0.50 --zoom 1.00
```

The `4` means it will create `bg4.png` and `bg4.c`. Use `5` for `bg5`, or `6` for `bg6`.

## If Python Says Pillow Is Missing

Run this once:

```powershell
python -m pip install --user pillow
```

Then run the converter again.

## After Converting

After you are happy with the preview PNG:

1. Commit the generated `.png` and `.c` files.
2. Push the module repo to GitHub.
3. Re-run the keyboard firmware build.
4. Flash the new UF2 to the dongle.