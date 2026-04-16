# Xiaord Background Converter

These tools turn normal JPG or PNG pictures into firmware background files for the 240x240 round display.

You do not need to know programming to use them. The short version is:

1. Fork this repo on GitHub.
2. Clone your fork to your computer.
3. Put your pictures in any folder you like.
4. Run one PowerShell command and choose that folder.
5. Check the preview PNG files.
6. Commit and push the generated files.
7. Enable one background in your keyboard `.conf` file.

## What The Tools Create

For each background slot, the converter creates two files:

- `bg4.png`, `bg5.png`, or `bg6.png`: a preview image you can open and check.
- `bg4.c`, `bg5.c`, or `bg6.c`: the firmware file ZMK compiles into the dongle.

The firmware currently works best with one full-size custom photo enabled at a time. You can keep `bg5` and `bg6` available, but enable only one custom background in your keyboard config for the safest build.

## Step 1: Fork The Repo

A fork is your own copy of the GitHub repo.

1. Open the `zmk-module-xiaord` repo page on GitHub.
2. Click `Fork` near the top right.
3. Leave the default options selected.
4. Click `Create fork`.

After this, GitHub will make a copy under your account.

## Step 2: Clone Your Fork

A clone is the copy that lives on your computer.

### Easy Way: GitHub Desktop

1. Install GitHub Desktop from `https://desktop.github.com/`.
2. Sign in with your GitHub account.
3. Open your fork in the web browser.
4. Click the green `Code` button.
5. Click `Open with GitHub Desktop`.
6. Choose where to save it, then click `Clone`.

### Command Line Way

Open PowerShell and run this, replacing `YOUR_GITHUB_NAME` with your GitHub username:

```powershell
git clone https://github.com/YOUR_GITHUB_NAME/zmk-module-xiaord.git
cd zmk-module-xiaord
```

## Step 3: Open The Repo In VS Code

1. Open VS Code.
2. Click `File`.
3. Click `Open Folder`.
4. Choose the `zmk-module-xiaord` folder you cloned.
5. Click `Terminal`.
6. Click `New Terminal`.

The terminal should be opened inside the repo folder. It will look something like this:

```text
C:\Users\YOUR_NAME\git\zmk-module-xiaord
```

## Step 4: Put Your Pictures In A Folder

Create a folder anywhere you like, then put one to three JPG or PNG pictures in it.

Example folders:

```text
C:\Users\YOUR_NAME\Pictures\Dongle Backgrounds
D:\Keyboard Pictures
Desktop\Dongle Pictures
```

The converter uses the pictures in filename order. If you want a specific order, name them like this:

```text
01-background.jpg
02-background.jpg
03-background.jpg
```

## Step 5: Run The Easy Converter

From the VS Code terminal, run:

```powershell
powershell -ExecutionPolicy Bypass -File tools/convert_backgrounds.ps1 -ChooseFolder
```

A folder picker will open. Choose the folder that contains your pictures.

The converter will create files for the first one to three pictures it finds:

```text
src/display/ui/bg/bg4.png
src/display/ui/bg/bg4.c
src/display/ui/bg/bg5.png
src/display/ui/bg/bg5.c
src/display/ui/bg/bg6.png
src/display/ui/bg/bg6.c
```

If you only have one picture, it will only update `bg4.png` and `bg4.c`.

Open the generated `.png` preview files to check how the pictures are cropped.

## Optional: Use A Typed Folder Path

If you prefer typing the folder path instead of using the folder picker, use `-SourceDir`:

```powershell
powershell -ExecutionPolicy Bypass -File tools/convert_backgrounds.ps1 -SourceDir "$env:USERPROFILE\Pictures\Dongle Backgrounds"
```

You can also use a full path, like:

```powershell
powershell -ExecutionPolicy Bypass -File tools/convert_backgrounds.ps1 -SourceDir "D:\Keyboard Pictures"
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

The PowerShell converter tries to install Pillow automatically. If you run the Python converter manually and it says Pillow is missing, run this once:

```powershell
python -m pip install --user pillow
```

Then run the converter again.

## Step 6: Commit And Push The Generated Files

### Easy Way: VS Code

1. Click the `Source Control` icon on the left side of VS Code.
2. You should see changed files like `bg4.png` and `bg4.c`.
3. Type a message like `Update custom background`.
4. Click `Commit`.
5. Click `Sync Changes` or `Push`.

### Command Line Way

```powershell
git status
git add src/display/ui/bg/bg4.png src/display/ui/bg/bg4.c src/display/ui/bg/bg5.png src/display/ui/bg/bg5.c src/display/ui/bg/bg6.png src/display/ui/bg/bg6.c
git commit -m "Update custom backgrounds"
git push
```

If you only converted one picture, it is okay if only `bg4.png` and `bg4.c` changed.

## Step 7: Build Your Keyboard Firmware

After pushing the module changes:

1. Go to your keyboard config repo on GitHub.
2. Re-run the firmware build workflow.
3. Download the new UF2 artifact.
4. Flash it to the dongle.
