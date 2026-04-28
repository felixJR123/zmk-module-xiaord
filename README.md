# THIS VERSION STILL NOT WORKING IT COMPILES BUT SCREEN DON'T TURN ON YET

# zmk-module-xiaord

A ZMK module for the Seeed XIAO Round Display. Adds a touch-enabled circular display as a companion device for your keyboard.

## Hardware

- [Seeed Studio XIAO Round Display](https://wiki.seeedstudio.com/get_start_round_display/) (display + touchpad + RTC + microSD)
- Tested with XIAO BLE (nRF52840)

### Backlight Hardware Switch

On the back of the XIAO Round Display, set `KE` switch 2 to the `ON` position.
The display may still work with the switch off, but firmware backlight control
depends on that switch being enabled.

## Features

- Peripheral battery level shown on the display
- BLE connection status display and management
- ZMK behaviors triggered by touch input
- Optional current time display via RTC

### Home Screen

- **Clock** — current time from the RTC
- **Peripheral battery** — battery level of the split keyboard peripheral
- **Output status** — current output (USB / Bluetooth) and active BT profile

### Home Screen Shortcut Buttons

Up to 12 buttons can be placed around the edge of the screen, indexed clockwise from 12 o'clock (position 0). Default assignments:

| Position | Icon (`ICON_*`) | Action |
|----------|-----------------|--------|
| 0 (12 o'clock) | `ICON_UPLOAD` (U+F093 ) | Enter bootloader |
| 1 (1 o'clock) | `ICON_IMAGE` (U+F03E ) | PrintScreen |
| 2 (2 o'clock) | `ICON_VOLUME_MAX` (U+F028 ) | Volume up |
| 3 (3 o'clock) | `ICON_MUTE` (U+F026 ) | Mute |
| 4 (4 o'clock) | `ICON_VOLUME_MID` (U+F027 ) | Volume down |
| 5 (5 o'clock) | `ICON_NEXT` (U+F051 ) | Next track |
| 6 (6 o'clock) | `ICON_PLAY` (U+F04B ) | Play/Pause |
| 7 (7 o'clock) | `ICON_PREV` (U+F048 ) | Previous track |
| 8 (8 o'clock) | `ICON_WARNING` (U+F071 ) | Ctrl+Alt+Del |
| 9 (9 o'clock) | `ICON_USB` (U+F287 ) | Toggle output (USB/BLE) |
| 10 (10 o'clock) | `ICON_BLUETOOTH` (U+F293 ) | Go to BT management screen |
| 11 (11 o'clock) | `ICON_SETTINGS` (U+F013 ) | Go to clock settings screen |

Touch gestures on the photo/home screen:

| Gesture | Action |
|---------|--------|
| Center single tap | Show/hide the shortcut button ring (`INPUT_VIRTUAL_GESTURE_TAP`) |
| Center double tap | Mute/unmute (`INPUT_VIRTUAL_GESTURE_DOUBLE_TAP`) |
| Clockwise slide | Volume up (`INPUT_VIRTUAL_GESTURE_CW`) |
| Counterclockwise slide | Volume down (`INPUT_VIRTUAL_GESTURE_CCW`) |
| Slide up | Up arrow (`INPUT_VIRTUAL_GESTURE_SLIDE_UP`) |
| Slide down | Down arrow (`INPUT_VIRTUAL_GESTURE_SLIDE_DOWN`) |
| Slide left | Left arrow (`INPUT_VIRTUAL_GESTURE_SLIDE_LEFT`) |
| Slide right | Right arrow (`INPUT_VIRTUAL_GESTURE_SLIDE_RIGHT`) |
| 12 o'clock tap zone | Show/hide date and time (`INPUT_VIRTUAL_GESTURE_TOUCH_12`) |
| 3 o'clock tap zone | Unassigned (`INPUT_VIRTUAL_GESTURE_TOUCH_3`) |
| 6 o'clock tap zone | Show/hide home screen information (`INPUT_VIRTUAL_GESTURE_TOUCH_6`) |
| 9 o'clock tap zone | Unassigned (`INPUT_VIRTUAL_GESTURE_TOUCH_9`) |

The center zone is reserved for tap and double-tap. The 12, 3, 6, and 9 o'clock
zones are separate home-screen touch zones that can be remapped like the other
gestures. By default, 12 o'clock toggles only the date/time labels, while
6 o'clock toggles all home information: date/time, output status, and battery
widgets. If all information is hidden, 12 o'clock can still bring back just the
time for a quick glance.

Clockwise and counterclockwise slides require deliberate circular motion. If
rotation still feels too easy to trigger, tune the `CONFIG_XIAORD_GESTURE_*`
options in the `.conf` reference below.

Home screen gestures and touch zones are regular ZMK behavior bindings you can
override in your dongle overlay. Center single tap fires
`INPUT_VIRTUAL_GESTURE_TAP`, whose default binding is `&xiaord_menu` (toggle
the shortcut ring). Defaults:

```dts
&virtual_gesture_behavior {
    codes = <
        INPUT_VIRTUAL_GESTURE_TAP
        INPUT_VIRTUAL_GESTURE_DOUBLE_TAP
        INPUT_VIRTUAL_GESTURE_CW
        INPUT_VIRTUAL_GESTURE_CCW
        INPUT_VIRTUAL_GESTURE_SLIDE_UP
        INPUT_VIRTUAL_GESTURE_SLIDE_DOWN
        INPUT_VIRTUAL_GESTURE_SLIDE_LEFT
        INPUT_VIRTUAL_GESTURE_SLIDE_RIGHT
        INPUT_VIRTUAL_GESTURE_TOUCH_12
        INPUT_VIRTUAL_GESTURE_TOUCH_3
        INPUT_VIRTUAL_GESTURE_TOUCH_6
        INPUT_VIRTUAL_GESTURE_TOUCH_9
    >;
    bindings = <
        &xiaord_menu      /* single tap   — show/hide shortcut ring */
        &kp C_MUTE        /* double tap   — mute/unmute */
        &kp C_VOL_UP      /* clockwise slide */
        &kp C_VOL_DN      /* counterclockwise slide */
        &kp UP_ARROW      /* slide up */
        &kp DOWN_ARROW    /* slide down */
        &kp LEFT_ARROW    /* slide left */
        &kp RIGHT_ARROW   /* slide right */
        &xiaord_home_datetime /* 12 o'clock tap zone */
        &none             /* 3 o'clock tap zone */
        &xiaord_home_info /* 6 o'clock tap zone */
        &none             /* 9 o'clock tap zone */
    >;
};
```

Built-in Xiaord behaviors:

| Behavior | Action |
|----------|--------|
| `&xiaord_menu` | Toggle the home screen shortcut button ring |
| `&xiaord_home_datetime` | Toggle only the home screen date/time labels |
| `&xiaord_home_info` | Toggle all home screen information widgets over the background |

Any of these can be bound to a gesture or home button.

Any ZMK behavior binding that is valid in a binding array works here, including
key presses, layer changes, and macros. For example:

```dts
&virtual_gesture_behavior {
    codes = <
        INPUT_VIRTUAL_GESTURE_TAP
        INPUT_VIRTUAL_GESTURE_DOUBLE_TAP
        INPUT_VIRTUAL_GESTURE_CW
        INPUT_VIRTUAL_GESTURE_CCW
        INPUT_VIRTUAL_GESTURE_SLIDE_UP
        INPUT_VIRTUAL_GESTURE_SLIDE_DOWN
        INPUT_VIRTUAL_GESTURE_SLIDE_LEFT
        INPUT_VIRTUAL_GESTURE_SLIDE_RIGHT
        INPUT_VIRTUAL_GESTURE_TOUCH_12
        INPUT_VIRTUAL_GESTURE_TOUCH_3
        INPUT_VIRTUAL_GESTURE_TOUCH_6
        INPUT_VIRTUAL_GESTURE_TOUCH_9
    >;
    bindings = <
        &xiaord_menu      /* single tap */
        &macro_my_action  /* double tap */
        &mo 1
        &tog 2
        &kp UP_ARROW
        &kp DOWN_ARROW
        &kp LEFT_ARROW
        &kp RIGHT_ARROW
        &xiaord_home_datetime
        &none
        &xiaord_home_info
        &none
    >;
};
```

#### Updating an existing keyboard repo for directional slides and touch zones

If your keyboard repo does not override `&virtual_gesture_behavior`, no overlay
change is required. Update the `zmk-module-xiaord` revision in your keyboard
repo's `config/west.yml`, rebuild, and the default slide bindings will be used.
The default 12 o'clock date/time toggle and 6 o'clock home info toggle will
also be used.

If your keyboard repo already overrides `&virtual_gesture_behavior`, update that
whole node to include all gesture codes. DTS array overrides replace the full
list, so partial updates will drop any omitted gestures:

```dts
#include <dt-bindings/xiaord/input_codes.h>
#include <dt-bindings/zmk/keys.h>

&virtual_gesture_behavior {
    codes = <
        INPUT_VIRTUAL_GESTURE_TAP
        INPUT_VIRTUAL_GESTURE_DOUBLE_TAP
        INPUT_VIRTUAL_GESTURE_CW
        INPUT_VIRTUAL_GESTURE_CCW
        INPUT_VIRTUAL_GESTURE_SLIDE_UP
        INPUT_VIRTUAL_GESTURE_SLIDE_DOWN
        INPUT_VIRTUAL_GESTURE_SLIDE_LEFT
        INPUT_VIRTUAL_GESTURE_SLIDE_RIGHT
        INPUT_VIRTUAL_GESTURE_TOUCH_12
        INPUT_VIRTUAL_GESTURE_TOUCH_3
        INPUT_VIRTUAL_GESTURE_TOUCH_6
        INPUT_VIRTUAL_GESTURE_TOUCH_9
    >;
    bindings = <
        &xiaord_menu      /* single tap */
        &kp C_MUTE        /* double tap */
        &kp C_VOL_UP
        &kp C_VOL_DN
        &kp UP_ARROW
        &kp DOWN_ARROW
        &kp LEFT_ARROW
        &kp RIGHT_ARROW
        &xiaord_home_datetime
        &none
        &xiaord_home_info
        &none
    >;
};
```

### Bluetooth Management Screen

- Select a BT profile (up to 12 profiles)
- Clear a BT profile
- Switch to USB output mode

## Installation

Example config: [xiaord_example](xiaord_example/)

### Adding to west.yml

Add this module to your keyboard config's `config/west.yml`:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-module-xiaord
      url: https://github.com/TakeshiAkehi/zmk-module-xiaord.git
      revision: main
  self:
    path: config
```

### Adding to build.yaml

Add `xiaord` to the shield list of the dongle target. The dongle acts as the BLE central, so **no other split half should set `CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y`**:

```yaml
include:
  - board: xiao_ble//zmk
    shield: xiaord your_keyboard_dongle
    artifact-name: your_keyboard_dongle
  # All other halves (left, right, …) run as peripherals — do NOT set CENTRAL=y.
  - board: xiao_ble//zmk
    shield: your_keyboard_left
    artifact-name: your_keyboard_left
  - board: xiao_ble//zmk
    shield: your_keyboard_right
    artifact-name: your_keyboard_right
```

> The `left`/`right` targets above are examples. The rule applies to every non-dongle half: because the dongle holds the central role, `CONFIG_ZMK_SPLIT_ROLE_CENTRAL=n` (the default) must be used for all keyboard halves when a dongle is present. Adding `CENTRAL=y` to any half would conflict with the dongle.

## Configuration

> **Breaking change (v0.0 → v0.1):** The home button configuration API changed significantly.
>
> | | v0.0 | v0.1 |
> |-|------|------|
> | Button property | `code = <INPUT_VIRTUAL_SYM_*>` | `symbol = <ICON_*>` |
> | Header | `<dt-bindings/xiaord/input_codes.h>` | `<dt-bindings/xiaord/icons.h>` |
> | Fired code | icon code (`INPUT_VIRTUAL_SYM_*`) | position code (`INPUT_VIRTUAL_POS_*`) |
>
> v0.0 configs are not compatible with v0.1. Refer to the README at the `v0.0` tag for the old API.

### `.conf` Options

These are the Xiaord-specific options you can use in your keyboard `.conf` or
`prj.conf`. This table intentionally leaves out standard ZMK options.

| Option | Default | Description |
|--------|---------|-------------|
| `CONFIG_XIAORD_BG_1` | `y` if no other compiled background is selected | Use built-in background 1. |
| `CONFIG_XIAORD_BG_2` | `n` | Use built-in background 2. |
| `CONFIG_XIAORD_BG_3` | `n` | Use built-in background 3. |
| `CONFIG_XIAORD_BG_4` | `n` | Generate and use one custom background from your keyboard config repo. |
| `CONFIG_XIAORD_BG_4_SOURCE_DIR` | `""` | Folder containing JPG/PNG source images for `BG_4`. Relative paths are resolved from the keyboard config repo. Empty means use `config/xiaord-bg`, the `XIAORD_BG_4_SOURCE_DIR` environment variable, or fall back to `BG_1`. |
| `CONFIG_XIAORD_BG_SD` | `n` | Load runtime backgrounds from the microSD card instead of compiling every photo into firmware. A compiled background should still be enabled as a fallback. |
| `CONFIG_XIAORD_BG_SD_MOUNT_POINT` | `"/SD:"` | Filesystem mount point for the SD card. |
| `CONFIG_XIAORD_BG_SD_VOLUME_NAME` | `"SD"` | Disk name registered by the board's SD/MMC driver. The Seeed XIAO Round Display normally uses `SD`. |
| `CONFIG_XIAORD_BG_SD_DIR` | `"/xiaord-bg/converted"` | Directory under the mount point containing converted `bg001.rgb565`, `bg002.rgb565`, and so on. |
| `CONFIG_XIAORD_BG_SD_MAX_FILES` | `999` | Maximum number of SD background files to index. This is only a maximum; fewer files are fine. |
| `CONFIG_XIAORD_BG_SD_ROTATE_MS` | `0` | Auto-advance SD backgrounds after this many milliseconds. `0` disables auto-rotation. |
| `CONFIG_XIAORD_BG_SD_RETRY_MS` | `5000` | Retry SD-card mounting after boot if the card is not ready. `0` disables retrying. |
| `CONFIG_XIAORD_BG_SD_GESTURES` | `y` when SD backgrounds are enabled | Use slide gestures to control SD backgrounds instead of firing the normal slide bindings. |
| `CONFIG_XIAORD_BG_5` | `n` | Deprecated compatibility option. Accepted so old configs still parse; custom backgrounds now use `BG_4`. |
| `CONFIG_XIAORD_BG_6` | `n` | Deprecated compatibility option. Accepted so old configs still parse; custom backgrounds now use `BG_4`. |
| `CONFIG_XIAORD_REMOVE_DATE_TIME` | `n` | Start the home screen with date/time hidden. The widgets are still available and can be toggled with `&xiaord_home_datetime`. |
| `CONFIG_XIAORD_DOUBLE_TAP_MS` | `450` | Maximum time between center taps for a double tap. Increase it if double-tap feels too hard. |
| `CONFIG_XIAORD_GESTURE_ROTATION_JITTER_CROSS` | `120` | Ignore tiny circular-motion samples below this value. Increase it if touch noise triggers rotation. |
| `CONFIG_XIAORD_GESTURE_ROTATION_INTENT_CROSS` | `9000` | Amount of accumulated circular motion required before clockwise/counterclockwise can fire. Increase it to make rotation more intentional. |
| `CONFIG_XIAORD_GESTURE_ROTATION_STEP_CROSS` | `6500` | Amount of circular motion required for each repeated clockwise/counterclockwise event after intent is detected. Increase it to reduce repeat rate. |
| `CONFIG_XIAORD_GESTURE_ROTATION_TANGENTIAL_RATIO` | `200` | How strongly motion must follow the ring instead of moving inward/outward. Higher values make straight slides less likely to count as rotation. |

Common examples:

```conf
# Start with date/time hidden, but allow touch 12 to show it.
CONFIG_XIAORD_REMOVE_DATE_TIME=y

# Make double-tap easier.
CONFIG_XIAORD_DOUBLE_TAP_MS=600

# Make clockwise/counterclockwise gestures more deliberate.
CONFIG_XIAORD_GESTURE_ROTATION_INTENT_CROSS=12000
CONFIG_XIAORD_GESTURE_ROTATION_STEP_CROSS=8000
CONFIG_XIAORD_GESTURE_ROTATION_TANGENTIAL_RATIO=250
```

### Dongle Overlay

Your keyboard's dongle overlay (`your_keyboard_dongle.overlay`) must disable the physical key matrix and redirect ZMK to use the mock kscan provided by this module.

#### Minimal overlay (no home button customization)

```dts
#include "your_keyboard.dtsi"

// The dongle has no physical key matrix — disable it and use xiaord's mock kscan.
&kscan0 { status = "disabled"; };
/ { chosen { zmk,kscan = &xiaord_mock_kscan; }; };
```

| Element | Purpose |
|---------|---------|
| `#include "your_keyboard.dtsi"` | Shared hardware definitions (pinout, etc.) |
| `&kscan0 { status = "disabled"; }` | Disables the GPIO key matrix (the dongle has no switches) |
| `xiaord_mock_kscan` | Dummy kscan provided by xiaord; satisfies ZMK's requirement for a kscan device |

#### Overlay with home button customization

To reassign a home button icon and its behavior, add the following includes and override the relevant nodes:

```dts
#include "your_keyboard.dtsi"
#include <dt-bindings/xiaord/icons.h>
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/outputs.h>

&kscan0 { status = "disabled"; };
/ { chosen { zmk,kscan = &xiaord_mock_kscan; }; };

// Change the icon at position 1 to show a keyboard symbol.
&home_button_1 { symbol = <ICON_KEYBOARD>; };

// Update virtual_symbol_behavior bindings.
// codes are fixed position indices (INPUT_VIRTUAL_POS_0..11) — they do NOT
// change when icons change. Only rewrite bindings when changing actions.
// You must rewrite the entire codes/bindings table — DTS does not support
// partial array overrides, so all entries must be listed.
&virtual_symbol_behavior {
    codes = <
        INPUT_VIRTUAL_POS_0    INPUT_VIRTUAL_POS_1
        INPUT_VIRTUAL_POS_2    INPUT_VIRTUAL_POS_3
        INPUT_VIRTUAL_POS_4    INPUT_VIRTUAL_POS_5
        INPUT_VIRTUAL_POS_6    INPUT_VIRTUAL_POS_7
        INPUT_VIRTUAL_POS_8    INPUT_VIRTUAL_POS_9
        INPUT_VIRTUAL_POS_10   INPUT_VIRTUAL_POS_11
    >;
    bindings = <
        &bootloader            &kp CAPSLOCK        /* pos 1 → CAPSLOCK */
        &kp C_VOL_UP           &kp C_MUTE
        &kp C_VOL_DN           &kp C_NEXT
        &kp C_PLAY             &kp C_PREV
        &kp LC(LA(DEL))        &out OUT_TOG
        &none                  &none
    >;
};
```

> **Important:** `&virtual_symbol_behavior` stores `codes` and `bindings` as flat DTS arrays. Overriding the node replaces the arrays entirely — list all entries, not just the changed ones.
>
> **Key design point:** The fired code is always `INPUT_VIRTUAL_POS_<n>` (the button's slot index), independent of which icon is displayed. Changing a button's icon (`symbol`) does **not** require updating `codes` — only `bindings` needs to change if you also want a different action.

### Customizing Home Screen Buttons

All 12 home button nodes (`home_button_0` … `home_button_11`) are defined with labels, so you only need to reference the buttons you want to change:

```dts
#include <dt-bindings/xiaord/icons.h>

&home_button_1 { symbol = <ICON_KEYBOARD>; };  /* 1 o'clock */
```

See `include/dt-bindings/xiaord/icons.h` for available `ICON_*` codepoints (FontAwesome 5).
You can also specify a Unicode codepoint directly as an integer literal:

```dts
&home_button_1 { symbol = <0xF11C>; };  /* U+F11C = keyboard glyph */
```

Changing `symbol` only affects the displayed icon; the fired code remains `INPUT_VIRTUAL_POS_1`.

### Bluetooth Management Screen

Each profile slot is shown with a background color indicating its current state:

| Color | Status |
|-------|--------|
| Blue | Connected |
| Red | Cannot connect (bonded but disconnected) |
| Yellow | Pairing |
| Green | Paired, not in use (bonded, not selected) |
| White | Open (not bonded) |

### Background Image

Original backgrounds and one keyboard-repo custom photo background are available. Enable one image in your keyboard's `.conf` or `prj.conf`.

| Setting | Preview |
|---------|---------|
| `CONFIG_XIAORD_BG_1=y` | ![bg1](src/display/ui/bg/bg1.png) |
| `CONFIG_XIAORD_BG_2=y` | ![bg2](src/display/ui/bg/bg2.png) |
| `CONFIG_XIAORD_BG_3=y` | ![bg3](src/display/ui/bg/bg3.png) |
| `CONFIG_XIAORD_BG_4=y` | First image in your keyboard repo's `config/xiaord-bg` folder |
| `CONFIG_XIAORD_BG_SD=y` | Runtime backgrounds from the microSD card |

The nRF52840 build reliably fits one full-size photo background. `BG_4` is generated during the keyboard build and is not stored in this module. If `CONFIG_XIAORD_BG_4=y` but no image can be found or generated, the firmware falls back to `BG_1` and still compiles.

For the default GitHub Actions ZMK workflow, put one PNG in your keyboard config repo:

```text
config/xiaord-bg/01-background.png
```

Then enable `BG_4`:

```conf
CONFIG_XIAORD_BG_4=y
```

If the picture is private or sensitive, keep your keyboard config repo private.
If the picture is not private, it is fine for the keyboard config repo to be
public.

PNG works in GitHub Actions without extra dependencies. JPG/JPEG files require
Pillow, which the default runner may not have.

If you want to use a different folder inside the keyboard config repo, set a relative path:

```conf
CONFIG_XIAORD_BG_4=y
CONFIG_XIAORD_BG_4_SOURCE_DIR="my-background-folder"
```

To start with the home screen date and time hidden for cleaner custom
backgrounds:

```conf
CONFIG_XIAORD_REMOVE_DATE_TIME=y
```

Tap the 12 o'clock home touch zone to show or hide the clock/date at runtime.
Set it back to `n` or remove the line to show the clock/date by default. The
RTC continues keeping time while the labels are hidden, so you do not need to
reset the clock when changing backgrounds.

### SD Card Backgrounds

SD card backgrounds keep the existing `BG_1` through `BG_4` options intact, but
move photo storage out of the firmware. Enable SD mode in the dongle `.conf`
and leave one compiled background enabled as the fallback:

```conf
CONFIG_XIAORD_BG_1=y
CONFIG_XIAORD_BG_2=n
CONFIG_XIAORD_BG_3=n
CONFIG_XIAORD_BG_4=n
CONFIG_XIAORD_BG_SD=y

# Default for the Seeed XIAO Round Display SD disk.
CONFIG_XIAORD_BG_SD_VOLUME_NAME="SD"

# Optional: rotate every 60 seconds. Set to 0 to disable auto-rotation.
CONFIG_XIAORD_BG_SD_ROTATE_MS=60000

# Optional: retry mounting every 5 seconds if the SD card is not ready at boot.
CONFIG_XIAORD_BG_SD_RETRY_MS=5000

# Optional: remap gestures for SD backgrounds (see below).
CONFIG_XIAORD_BG_SD_GESTURES=y
```

The firmware expects converted 240x240 RGB565 files here:

```text
/SD:/xiaord-bg/converted/bg001.rgb565
/SD:/xiaord-bg/converted/bg002.rgb565
/SD:/xiaord-bg/converted/bg003.rgb565
```

The host-side prep tool creates this SD card layout:

```text
xiaord-bg/
  raw/        original JPG/PNG files
  converted/  bg001.rgb565, bg002.rgb565, ...
  tools/      converter scripts copied onto the SD card
```

From this repo, prepare a mounted SD card like this:

```powershell
python tools\xiaord_sd_backgrounds.py E:\ --source C:\Path\To\Pictures
```

You do not need to clone the whole module just to prepare an SD card. You can
download these three files into one local folder and run the same command from
there:

```text
xiaord_sd_backgrounds.py
convert_xiaord_bg.py
SD_BACKGROUND_STEPS.txt
```

If the picture folder has spaces in its path, wrap it in quotes:

```powershell
python xiaord_sd_backgrounds.py E:\ --source "C:\Path With Spaces\Xiaord Backgrounds"
```

Later, you can put new JPG/PNG files directly into `E:\xiaord-bg\raw` and run
the copied tool from the SD card:

```powershell
python E:\xiaord-bg\tools\xiaord_sd_backgrounds.py E:\
```

Files are scanned in numeric filename order. `bg001.rgb565` is shown first, then
`bg002.rgb565`, and the list wraps back to the first image. If the SD card is
missing, the mount fails, or no converted files are found, the firmware falls
back to whichever compiled background option is enabled.

For example, this uses SD backgrounds when available and `BG_4` as the fallback:

```conf
CONFIG_XIAORD_BG_1=n
CONFIG_XIAORD_BG_2=n
CONFIG_XIAORD_BG_3=n
CONFIG_XIAORD_BG_4=y
CONFIG_XIAORD_BG_SD=y
```

`CONFIG_XIAORD_BG_SD_MAX_FILES=999` is a maximum, not a required picture count.
If the card has 5 converted files, the firmware cycles those 5 files. If it has
no converted files, it uses the compiled fallback.

If the SD card is inserted but the firmware does not find it, make sure the card
is formatted as FAT32, the converted files are under `xiaord-bg/converted`, and
`CONFIG_XIAORD_BG_SD_VOLUME_NAME` matches the disk name registered by the board.
The Seeed XIAO Round Display normally uses `SD`, which mounts at `/SD:`.
The card needs to be inserted before the keyboard boots.
If it is slow to become ready, the firmware retries by default every 5 seconds.

Each converted full-screen background is 115,200 bytes on the SD card. The
firmware loads each background into RAM when you switch to it and does not
compile the SD photos into the UF2.

#### SD Gesture Mode (`CONFIG_XIAORD_BG_SD_GESTURES=y`)

When enabled, the four slide directions are remapped to control SD backgrounds
instead of firing the standard arrow-key bindings:

| Gesture | Action |
|---------|--------|
| Single tap | Show/hide the shortcut button ring |
| Double tap | Mute/unmute |
| Swipe down (top → bottom) | Next background |
| Swipe up (bottom → top) | Previous background |
| Swipe left (right → left) | Resume auto-scroll |
| Swipe right (left → right) | Pause auto-scroll |

A small white dot appears at the bottom of the screen when auto-scroll is
paused, so you always know the current state.

Auto-scroll uses the `CONFIG_XIAORD_BG_SD_ROTATE_MS` interval. When you
manually change the background while auto-scroll is running, the timer resets
so the full interval elapses before the next automatic advance. Pausing and
resuming auto-scroll with swipe-right / swipe-left preserves the current
background and the timer state.

When `CONFIG_XIAORD_BG_SD_GESTURES=n`, all four slide directions fire the
normal `virtual_gesture_behavior` bindings defined in your overlay. When
`CONFIG_XIAORD_BG_SD=y`, SD gesture mode defaults to enabled.

### Display Backlight Timeout

The module uses ZMK's idle timeout to turn off the XIAO Round Display backlight.
To keep the display on for 5 minutes after activity, add this to your keyboard
`.conf`:

```conf
CONFIG_ZMK_IDLE_TIMEOUT=300000
```

The value is in milliseconds. Backlight control also requires the back-side
`KE` switch 2 to be in the `ON` position.

### RTC

Install a **CR927 coin cell** in the XIAO Round Display to retain the time across power cycles. Without a battery the clock resets on every boot.

To open the clock settings screen from a home button, assign `XIAORD_PAGE_CLOCK` to its `nav-page` property:

```dts
&home_button_11 { symbol = <ICON_SETTINGS>; nav-page = <XIAORD_PAGE_CLOCK>; };
```

## License

MIT. Free to use for any purpose. No warranty of any kind.

## Icon & Code Reference

Home button icons are specified using `ICON_*` constants defined in `include/dt-bindings/xiaord/icons.h`. Each constant is a FontAwesome 5 Unicode codepoint rendered via the bundled Montserrat LVGL font.

The virtual event codes fired on button tap are defined in `include/dt-bindings/xiaord/input_codes.h`:

| Range | Category |
|-------|----------|
| `0x00–0x0B` | Home button positions (`INPUT_VIRTUAL_POS_0` … `INPUT_VIRTUAL_POS_11`) |
| `0x10–0x1B` | Home screen gestures (`INPUT_VIRTUAL_GESTURE_*`) |
| `0x40–0x6B` | ZMK BT/output behaviors (`INPUT_VIRTUAL_ZMK_*`) |

Gesture codes use `INPUT_VIRTUAL_GESTURE_TAP`, `INPUT_VIRTUAL_GESTURE_CW`,
`INPUT_VIRTUAL_GESTURE_CCW`, `INPUT_VIRTUAL_GESTURE_SLIDE_UP`,
`INPUT_VIRTUAL_GESTURE_SLIDE_DOWN`, `INPUT_VIRTUAL_GESTURE_SLIDE_LEFT`, and
`INPUT_VIRTUAL_GESTURE_SLIDE_RIGHT`, plus the cardinal home-screen touch zones
`INPUT_VIRTUAL_GESTURE_TOUCH_12`, `INPUT_VIRTUAL_GESTURE_TOUCH_3`,
`INPUT_VIRTUAL_GESTURE_TOUCH_6`, and `INPUT_VIRTUAL_GESTURE_TOUCH_9`. They are handled by
`virtual_gesture_behavior`, which runs before the home button position
processor.

## Behavior Conversion Flow

When a home button is tapped, the following chain executes:

```
home button tap
  → INPUT_VIRTUAL_POS_<n> code emitted (n = clock-position index 0–11)
  → touchpad_listener (zmk,input-listener) receives the event
  → input-processors consulted in order:
      1. &virtual_zmk_behavior    — matches ZMK BT/output codes (0x40–0x6B)
      2. &virtual_symbol_behavior — matches position codes (0x00–0x0B)
  → matching binding (e.g. &kp CAPSLOCK) is executed
```

The fired code encodes only **position**, not icon. The displayed icon (`ICON_*` codepoint) is a pure display concern stored separately in the button descriptor and rendered via UTF-8 conversion.

### Processor roles

| Processor | Codes handled | Typical use |
|-----------|--------------|-------------|
| `virtual_zmk_behavior` | `INPUT_VIRTUAL_ZMK_*` (BT_SEL, BT_CLR, OUT_USB…) | Internal BT management pages |
| `virtual_symbol_behavior` | `INPUT_VIRTUAL_POS_*` (positions 0–11) | Home screen buttons; customizable per keyboard |

Both processors are defined in `boards/shields/xiaord/zmk_behaviors.dtsi`. `virtual_zmk_behavior` covers all standard BT/output operations and rarely needs changes. `virtual_symbol_behavior` maps each position to a ZMK binding and is intended to be overridden per keyboard via the dongle overlay. Because codes are positional and fixed, overriding only requires updating `bindings` — `codes` stays the same regardless of which icons are displayed.

## Known Limitations / Not Yet Implemented

- Only tested on XIAO BLE (nRF52840)
- Occasional hang on the date-setting screen
- Font color options other than white
- Battery management UI for the XIAO Round Display itself
