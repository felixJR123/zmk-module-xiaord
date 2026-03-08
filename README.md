# zmk-module-xiaord

A ZMK module for the Seeed XIAO Round Display. Adds a touch-enabled circular display as a companion device for your keyboard.

## Hardware

- [Seeed Studio XIAO Round Display](https://wiki.seeedstudio.com/get_start_round_display/) (display + touchpad + RTC + microSD)
- Tested with XIAO BLE (nRF52840)

## Features

- Peripheral battery level shown on the display
- BLE connection status display and management
- ZMK behaviors triggered by touch input
- Current time display via RTC

### Home Screen

- **Clock** — current time from the RTC
- **Peripheral battery** — battery level of the split keyboard peripheral
- **Output status** — current output (USB / Bluetooth) and active BT profile

### Home Screen Shortcut Buttons

Up to 12 buttons can be placed around the edge of the screen, indexed clockwise from 12 o'clock (position 0). Default assignments:

| Position | Icon | Action |
|----------|------|--------|
| 0 (12 o'clock) | UPLOAD | Enter bootloader |
| 1 (1 o'clock) | IMAGE | PrintScreen |
| 2 (2 o'clock) | VOLUME MAX | Volume up |
| 3 (3 o'clock) | MUTE | Mute |
| 4 (4 o'clock) | VOLUME MID | Volume down |
| 5 (5 o'clock) | NEXT | Next track |
| 6 (6 o'clock) | PLAY | Play/Pause |
| 7 (7 o'clock) | PREV | Previous track |
| 8 (8 o'clock) | WARNING | Ctrl+Alt+Del |
| 9 (9 o'clock) | USB | Switch to USB output |
| 10 (10 o'clock) | BLUETOOTH | Go to BT management screen |
| 11 (11 o'clock) | SETTINGS | Go to clock settings screen |

### Bluetooth Management Screen

- Select a BT profile (up to 12 profiles)
- Clear a BT profile
- Switch to USB output mode

## Installation

Example config: [zmk-config-fish](https://github.com/TakeshiAkehi/zmk-config-fish.git)

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
#include <dt-bindings/xiaord/input_codes.h>
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/outputs.h>

&kscan0 { status = "disabled"; };
/ { chosen { zmk,kscan = &xiaord_mock_kscan; }; };

// Change the icon at position 1 to show a keyboard symbol.
&home_button_1 { code = <INPUT_VIRTUAL_SYM_KEYBOARD>; };

// Update virtual_symbol_behavior to map the new icon to a behavior.
// You must rewrite the entire codes/bindings table — DTS does not support
// partial array overrides, so all entries must be listed.
&virtual_symbol_behavior {
    codes = <
        INPUT_VIRTUAL_SYM_UPLOAD
        INPUT_VIRTUAL_SYM_KEYBOARD      /* replaces IMAGE at position 1 */
        INPUT_VIRTUAL_SYM_VOLUME_MAX
        /* ... all other codes you want to handle ... */
    >;
    bindings = <
        &bootloader
        &kp CAPSLOCK                    /* new binding for KEYBOARD icon */
        &kp C_VOL_UP
        /* ... matching bindings for each code above ... */
    >;
};
```

> **Important:** `&virtual_symbol_behavior` stores `codes` and `bindings` as flat DTS arrays. Overriding the node replaces the arrays entirely — list all entries, not just the changed ones.

### Customizing Home Screen Buttons

Override the home buttons by adding a `home_buttons` node to your keyboard's `.overlay` file:

```dts
#include <dt-bindings/xiaord/input_codes.h>

/ {
    home_buttons {
        compatible = "xiaord,home-buttons";

        btn_copy {
            position = <3>;  /* 3 o'clock */
            code = <INPUT_VIRTUAL_SYM_COPY>;
        };
        btn_bt {
            position = <9>;  /* 9 o'clock */
            code = <INPUT_VIRTUAL_SYM_BLUETOOTH>;
            nav-page = <XIAORD_PAGE_BT>;  /* navigate to BT screen on tap */
        };
    };
};
```

`position` is a clockwise index from 0 (12 o'clock) to 11 (11 o'clock). See `include/dt-bindings/xiaord/input_codes.h` for available icon codes.

### Background Image

Three background images are available. Set one in your keyboard's `.conf` or `prj.conf`:

| Setting | Preview |
|---------|---------|
| `CONFIG_XIAORD_BG_1=y` (default) | ![bg1](src/display/ui/bg/bg1.png) |
| `CONFIG_XIAORD_BG_2=y` | ![bg2](src/display/ui/bg/bg2.png) |
| `CONFIG_XIAORD_BG_3=y` | ![bg3](src/display/ui/bg/bg3.png) |

### RTC

Install a **CR927 coin cell** in the XIAO Round Display to retain the time across power cycles. Without a battery the clock resets on every boot.

## License

MIT. Free to use for any purpose. No warranty of any kind.

## Symbol Reference

Home button icons are selected using `INPUT_VIRTUAL_SYM_*` constants defined in `include/dt-bindings/xiaord/input_codes.h`. Each constant maps to an LVGL built-in symbol glyph. See the [LVGL font overview](https://docs.lvgl.io/9.0/overview/font.html) for visual previews of each symbol.

| Range | Category |
|-------|----------|
| `0x00–0x3D` | LVGL symbol glyphs (`INPUT_VIRTUAL_SYM_*`) |
| `0x40–0x7B` | ZMK BT/output behaviors (`INPUT_VIRTUAL_ZMK_*`) |

## Behavior Conversion Flow

When a home button is tapped, the following chain executes:

```
home button tap
  → INPUT_VIRTUAL_SYM_* code emitted by virtual_key_source
  → touchpad_listener (zmk,input-listener) receives the event
  → input-processors consulted in order:
      1. &virtual_zmk_behavior    — matches ZMK BT/output codes (0x40–0x7B)
      2. &virtual_symbol_behavior — matches LVGL symbol codes (0x00–0x3D)
  → matching binding (e.g. &kp CAPSLOCK) is executed
```

### Processor roles

| Processor | Codes handled | Typical use |
|-----------|--------------|-------------|
| `virtual_zmk_behavior` | `INPUT_VIRTUAL_ZMK_*` (BT_SEL, BT_CLR, OUT_USB…) | Internal BT management pages |
| `virtual_symbol_behavior` | `INPUT_VIRTUAL_SYM_*` (LVGL symbols) | Home screen buttons; customizable per keyboard |

Both processors are defined in `boards/shields/xiaord/zmk_behaviors.dtsi`. `virtual_zmk_behavior` covers all standard BT/output operations and rarely needs changes. `virtual_symbol_behavior` provides defaults matching the built-in home button layout, but is intended to be overridden per keyboard via the dongle overlay when home buttons are reassigned.

## Architecture

```mermaid
flowchart LR
    chsc6x["chsc6x\n(touch driver)"]
    lvgl_ptr["lvgl_pointer"]
    display["display\n(LVGL UI)"]
    vkey["virtual_key_source"]
    listener["ZMK input_listener"]
    processor["zmk,input-processor-behaviors"]
    behavior["ZMK behavior"]

    chsc6x -->|input API| lvgl_ptr
    lvgl_ptr --> display
    display -->|input API| vkey
    vkey -->|input API| listener
    listener --> processor
    processor --> behavior
```

## Known Limitations / Not Yet Implemented

- Only tested on XIAO BLE (nRF52840)
- Occasional hang on the date-setting screen
- Loading custom background images from microSD
- Font color options other than white
- Battery management UI for the XIAO Round Display itself
