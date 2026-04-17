# Xiaord Background Converter

These tools turn normal JPG or PNG pictures into firmware background files for the 240x240 round display.

You do not need to know programming to use them. The short version is:

1. Clone this repo to your computer.
2. Put one private picture in a local folder.
3. Run one PowerShell command and choose that folder.
4. Check the preview PNG file.
5. Enable the local custom background in your keyboard `.conf` file.
6. Build from the computer or private build environment that has the picture.

## What The Tools Create

The converter creates two local files:

- `src/display/ui/bg/bg4.png`: a preview image you can open and check.
- `src/display/ui/bg/bg4.c`: the firmware file ZMK compiles into the dongle.

These files are ignored by Git. Do not commit them if they contain personal pictures. If `CONFIG_XIAORD_BG_4=y` but `bg4.c` cannot be found or generated, the firmware falls back to `BG_1` so public builds still compile.

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

Create a folder anywhere you like, then put one JPG or PNG picture in it.

Example folders:

```text
C:\Users\YOUR_NAME\Pictures\Dongle Backgrounds
D:\Keyboard Pictures
Desktop\Dongle Pictures
```

If the folder has more than one picture, the converter uses the first one in filename order. Name the one you want first, for example:

```text
01-background.jpg
```

## Step 5: Run The Easy Converter

From the VS Code terminal, run:

```powershell
powershell -ExecutionPolicy Bypass -File tools/convert_backgrounds.ps1 -ChooseFolder
```

A folder picker will open. Choose the folder that contains your pictures.

The converter will create:

```text
src/display/ui/bg/bg4.png
src/display/ui/bg/bg4.c
```

Open `bg4.png` to check how the picture is cropped.

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

In your keyboard `.conf` file, enable the local custom background:

```conf
CONFIG_XIAORD_BG_1=n
CONFIG_XIAORD_BG_2=n
CONFIG_XIAORD_BG_3=n
CONFIG_XIAORD_BG_4=y
```

For an easier local build, point ZMK at the private folder. If `bg4.c` is not already generated, the build will convert the first JPG/PNG in that folder automatically:

```conf
CONFIG_XIAORD_BG_4=y
CONFIG_XIAORD_BG_4_SOURCE_DIR="C:/Users/YOUR_NAME/Pictures/Dongle Backgrounds"
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
$CenterX = "0.54"
$CenterY = "0.50"
$Zoom = "1.00"
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

The `4` is optional now because `bg4` is the only custom slot:

```powershell
python tools/convert_xiaord_bg.py src/display/ui/bg/source/background1.jpg --center-x 0.50 --center-y 0.50 --zoom 1.00
```

## If Python Says Pillow Is Missing

The PowerShell converter tries to install Pillow automatically. If you run the Python converter manually and it says Pillow is missing, run this once:

```powershell
python -m pip install --user pillow
```

Then run the converter again.

## Step 6: Keep The Generated Files Private

Do not commit `src/display/ui/bg/bg4.png` or `src/display/ui/bg/bg4.c` if they contain a personal picture. They are ignored by this repo on purpose.

## Step 7: Build Your Keyboard Firmware

Build from the computer or private build environment that has the local picture files. Public builds that do not have the picture will fall back to `BG_1` instead of failing.
