# Xiaord Background Images

The display backgrounds are compiled into firmware as 240x240 LVGL RGB565 C files.

For the step-by-step beginner guide, including forking, cloning, choosing a picture folder, converting pictures, committing, and pushing, use:

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
CONFIG_XIAORD_BG_5=n
CONFIG_XIAORD_BG_6=n
```

The firmware uses one static background. If more than one background is enabled, the module chooses the first available image in this priority: `BG_4`, `BG_5`, `BG_6`, then `BG_1`, `BG_2`, `BG_3`.

The firmware reliably fits one full-size 240x240 RGB565 photo background. Keep only one `CONFIG_XIAORD_BG_*` option enabled in the keyboard config for the safest build.

For example, to use `bg4`:

```conf
CONFIG_XIAORD_BG_1=n
CONFIG_XIAORD_BG_2=n
CONFIG_XIAORD_BG_3=n
CONFIG_XIAORD_BG_4=y
CONFIG_XIAORD_BG_5=n
CONFIG_XIAORD_BG_6=n
```

For a cleaner photo background, hide the home screen date and time:

```conf
CONFIG_XIAORD_REMOVE_DATE_TIME=y
```

The RTC still keeps time while the clock/date is hidden.
