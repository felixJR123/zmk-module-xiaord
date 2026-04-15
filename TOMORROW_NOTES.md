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