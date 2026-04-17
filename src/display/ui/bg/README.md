# Xiaord Background Images

The display backgrounds are compiled into firmware as 240x240 LVGL RGB565 C files.

For the step-by-step beginner guide, including choosing a private picture folder and converting one picture, use:

```text
tools/README.md
```

## Config Behavior

Each image has its own Kconfig option:

```conf
CONFIG_XIAORD_BG_1=y
CONFIG_XIAORD_BG_2=n
CONFIG_XIAORD_BG_3=n
CONFIG_XIAORD_BG_4=n
```

The firmware uses one static background. `BG_4` is the only custom slot and is intentionally local-only. The generated `bg4.c` and `bg4.png` files are ignored by Git so personal pictures do not get published with the module.

If `CONFIG_XIAORD_BG_4=y` but no local custom image can be found or generated, the firmware falls back to `BG_1` and still compiles.

For example, to use `bg4`:

```conf
CONFIG_XIAORD_BG_1=n
CONFIG_XIAORD_BG_2=n
CONFIG_XIAORD_BG_3=n
CONFIG_XIAORD_BG_4=y
CONFIG_XIAORD_BG_4_SOURCE_DIR="C:/Users/YOUR_NAME/Pictures/Dongle Backgrounds"
```

For a cleaner photo background, hide the home screen date and time:

```conf
CONFIG_XIAORD_REMOVE_DATE_TIME=y
```

The RTC still keeps time while the clock/date is hidden.
