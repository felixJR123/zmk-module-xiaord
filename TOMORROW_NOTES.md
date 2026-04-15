# Xiaord Dongle Notes

Date: 2026-04-14

## Current Goal

The dongle display now supports family photo backgrounds. The intent is:

- Keep the original 3 backgrounds available as single/static options.
- Add family backgrounds `BG_4`, `BG_5`, and `BG_6`.
- If more than one enabled background is present, rotate through enabled images.
- Avoid compiling too many 240x240 RGB565 images because flash space is tight.

## Important Hardware Note

The LCD backlight issue was not only firmware. The tiny back-side screen switch `KE 2` needed to be ON. After turning `KE 2` on, the screen/backlight shutoff worked.

## Latest Relevant Module Commits Pushed

- `2543de6 Make family background converter path portable`
- `b8b9da6 Exclude original backgrounds when family images are enabled`

The latest pushed commit at the time this note was first written was:

```text
b8b9da6 Exclude original backgrounds when family images are enabled
```

## Build Error Seen

GitHub Actions failed at link time with:

```text
section `rodata' will not fit in region `FLASH'
region `FLASH' overflowed by 113912 bytes
```

That overflow size is almost exactly one 240x240 RGB565 background image. After excluding original backgrounds when family images are enabled, the same overflow means the firmware likely fits two full-size photo backgrounds, not three.

The latest fix changes both:

- `src/display/ui/CMakeLists.txt`
- `src/display/status_screen.c`

so when any family background is enabled, the module compiles/references only family backgrounds and leaves `BG_1`, `BG_2`, and `BG_3` out.

## Keyboard Config To Use

In the keyboard repo config, use this build-safe setup for two family pictures:

```conf
CONFIG_XIAORD_BG_1=n
CONFIG_XIAORD_BG_2=n
CONFIG_XIAORD_BG_3=n
CONFIG_XIAORD_BG_4=y
CONFIG_XIAORD_BG_5=y
CONFIG_XIAORD_BG_6=n
CONFIG_XIAORD_BG_ROTATE_INTERVAL_MIN=5
```

After commit `b8b9da6`, the build should ignore original backgrounds if a stale config still sets `CONFIG_XIAORD_BG_1=y`, but it is cleaner to set it to `n` explicitly.

## Confirmed Working

After setting `CONFIG_XIAORD_BG_6=n` in the keyboard config, the GitHub build worked. Keep `BG_6` in the module as an available option, but only enable two full-size photo backgrounds at once unless image storage is optimized later.
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

Next test user planned: build with only one family image enabled to see if even two photos are too much at runtime or for final flash layout.

Suggested one-photo config:

```conf
CONFIG_XIAORD_BG_1=n
CONFIG_XIAORD_BG_2=n
CONFIG_XIAORD_BG_3=n
CONFIG_XIAORD_BG_4=y
CONFIG_XIAORD_BG_5=n
CONFIG_XIAORD_BG_6=n
CONFIG_XIAORD_BG_ROTATE_INTERVAL_MIN=5
```

## Tomorrow Follow-Up Idea

If the one-photo or two-photo firmware works today, user wants to go back to non-rotating backgrounds tomorrow. Removing or disabling rotation may clear a little memory and simplify the display code. Best approach tomorrow:

- Keep `BG_4`, `BG_5`, and `BG_6` assets available.
- Use only one selected background at compile time.
- Remove or gate the `status_background_timer` rotation logic behind a config option defaulting off.
- Consider making `CONFIG_XIAORD_BG_ROTATE_INTERVAL_MIN` depend on a new explicit rotate option, or remove rotation support if user decides it is not needed.
- This will not save as much flash as removing an image file from the build, but it can reduce code/data a little and reduce risk.
