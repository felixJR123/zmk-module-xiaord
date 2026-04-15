# Xiaord Dongle Notes

Date: 2026-04-14

## Current Goal

The dongle display now supports custom photo backgrounds. The intent is:

- Keep the original 3 backgrounds available as single/static options.
- Add custom backgrounds `BG_4`, `BG_5`, and `BG_6`.
- Use one static compile-time background; auto-rotation was removed after one photo was confirmed working.
- Avoid compiling too many 240x240 RGB565 images because flash space is tight.

## Important Hardware Note

The LCD backlight issue was not only firmware. The tiny back-side screen switch `KE 2` needed to be ON. After turning `KE 2` on, the screen/backlight shutoff worked.

## Latest Relevant Module Commits Pushed

- `2543de6 Make custom background converter path portable`
- `b8b9da6 Exclude original backgrounds when custom images are enabled`

The latest pushed commit at the time this note was first written was:

```text
b8b9da6 Exclude original backgrounds when custom images are enabled
```

## Build Error Seen

GitHub Actions failed at link time with:

```text
section `rodata' will not fit in region `FLASH'
region `FLASH' overflowed by 113912 bytes
```

That overflow size is almost exactly one 240x240 RGB565 background image. After excluding original backgrounds when custom images are enabled, the same overflow means the firmware likely fits two full-size photo backgrounds, not three.

The latest fix changes both:

- `src/display/ui/CMakeLists.txt`
- `src/display/status_screen.c`

so when any custom background is enabled, the module compiles/references only custom backgrounds and leaves `BG_1`, `BG_2`, and `BG_3` out.

## Keyboard Config To Use

In the keyboard repo config, use this build-safe setup for one custom picture:

```conf
CONFIG_XIAORD_BG_1=n
CONFIG_XIAORD_BG_2=n
CONFIG_XIAORD_BG_3=n
CONFIG_XIAORD_BG_4=y
CONFIG_XIAORD_BG_5=n
CONFIG_XIAORD_BG_6=n
```

After commit `b8b9da6`, the build should ignore original backgrounds if a stale config still sets `CONFIG_XIAORD_BG_1=y`, but it is cleaner to set it to `n` explicitly.

## Confirmed Working

After setting only one custom image to `y`, the GitHub build and UF2 worked. Keep `BG_5` and `BG_6` in the module as available options, but only enable one full-size photo background at once unless image storage is optimized later.

## UF2 Flashing / Bootloader Notes

The dongle bootloader info shown by `INFO_UF2.TXT`:

```text
UF2 Bootloader 0.6.1 lib/nrfx (v2.0.0) lib/tinyusb (0.10.1-293-gaf8e5a90) lib/uf2 (remotes/origin/configupdate-9-gadbb8c7)
Model: Seeed XIAO nRF52840
Board-ID: Seeed_XIAO_nRF52840_Sense
SoftDevice: S140 version 7.3.0
Date: Nov 12 2021
```

Symptom after successful two-photo build: user could copy the UF2 file, but the board did not reset on its own and the screen stayed blank. Need verify whether the UF2 drive disappears after copy. If it disappears, the firmware probably flashed but crashes/does not start display. If it stays mounted, the bootloader likely rejected/ignored the UF2 or the wrong artifact was copied.

Confirmed: one custom image works. Two photos built but caused UF2/boot/runtime trouble, so stay with one full-size photo for now.

Suggested one-photo config:

```conf
CONFIG_XIAORD_BG_1=n
CONFIG_XIAORD_BG_2=n
CONFIG_XIAORD_BG_3=n
CONFIG_XIAORD_BG_4=y
CONFIG_XIAORD_BG_5=n
CONFIG_XIAORD_BG_6=n
```

## Rotation Removed

Auto-rotating backgrounds were removed after confirming one picture works. The module now uses a single static compile-time background. If more than one `CONFIG_XIAORD_BG_*` option is accidentally enabled, it picks the first available image in this priority: `BG_4`, `BG_5`, `BG_6`, then `BG_1`, `BG_2`, `BG_3`.

## Date/Time Overlay Option

Added `CONFIG_XIAORD_REMOVE_DATE_TIME=y` to remove the date and time labels from the home screen for cleaner custom backgrounds. Set it to `n` or remove the line to bring the clock/date back for default backgrounds. The separate clock settings page remains available.

## Touch Volume Gestures

Added home-screen touch gestures:

- Single tap fires `INPUT_VIRTUAL_POS_3` (default mute).
- Double tap runs the old tap behavior to show/hide the shortcut ring.
- Clockwise slide fires `INPUT_VIRTUAL_POS_2` repeatedly (default volume up).
- Counterclockwise slide fires `INPUT_VIRTUAL_POS_4` repeatedly (default volume down).

The gesture uses the existing positional bindings, so keyboard overlays can still customize what positions 2, 3, and 4 do.
