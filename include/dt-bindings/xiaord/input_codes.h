/*
 * SPDX-License-Identifier: MIT
 *
 * Virtual input event codes for INPUT_EV_ZMK_BEHAVIORS (0xF1).
 * Zephyr reserves INPUT_EV_VENDOR_START (0xF0) .. INPUT_EV_VENDOR_STOP (0xFF)
 * for vendor use. We claim 0xF1 for behavior events.
 *
 * Three categories:
 *   INPUT_VIRTUAL_KEY_<n>         — macropad key slots (0x00-0x04)
 *   INPUT_VIRTUAL_ZMK_<behavior>  — ZMK behavior codes (0x10-0x5B)
 *   INPUT_VIRTUAL_SYM_<SYMBOL>    — LVGL symbol codes  (0x100+)
 *
 * Usable from both C source and DTS overlays via #include.
 */

#pragma once

/* Custom event type: "invoke ZMK behavior by index". */
#define INPUT_EV_ZMK_BEHAVIORS 0xF1

/* ── Category 1: Macropad key slots ────────────────────────────────────── */

#define INPUT_VIRTUAL_KEY_0     0x00
#define INPUT_VIRTUAL_KEY_1     0x01
#define INPUT_VIRTUAL_KEY_2     0x02
#define INPUT_VIRTUAL_KEY_3     0x03
#define INPUT_VIRTUAL_KEY_4     0x04
#define INPUT_VIRTUAL_KEY_COUNT 0x05

/* ── Category 2: ZMK behavior codes ────────────────────────────────────── */
/*
 * 0x10-0x1F  system      (requires: none / CONFIG_ZMK_PM_SOFT_OFF / CONFIG_ZMK_STUDIO)
 * 0x20-0x2F  BT management (requires: CONFIG_ZMK_BLE)
 * 0x30-0x3B  BT_SEL n    (requires: CONFIG_ZMK_BLE)
 * 0x40-0x4B  BT_CLR n    (per-profile; leave unmapped or define via keyboard overlay)
 * 0x50-0x5B  BT_DISC n   (requires: CONFIG_ZMK_BLE)
 */

#define INPUT_VIRTUAL_ZMK_SYS_RESET     0x10
#define INPUT_VIRTUAL_ZMK_BOOTLOADER    0x11
#define INPUT_VIRTUAL_ZMK_SOFT_OFF      0x12  /* CONFIG_ZMK_PM_SOFT_OFF=y */
#define INPUT_VIRTUAL_ZMK_STUDIO_UNLOCK 0x13  /* CONFIG_ZMK_STUDIO=y */

#define INPUT_VIRTUAL_ZMK_BT_CLR        0x20
#define INPUT_VIRTUAL_ZMK_BT_CLR_ALL    0x21
#define INPUT_VIRTUAL_ZMK_BT_NXT        0x22
#define INPUT_VIRTUAL_ZMK_BT_PRV        0x23

#define INPUT_VIRTUAL_ZMK_BT_SEL_0      0x30
#define INPUT_VIRTUAL_ZMK_BT_SEL_1      0x31
#define INPUT_VIRTUAL_ZMK_BT_SEL_2      0x32
#define INPUT_VIRTUAL_ZMK_BT_SEL_3      0x33
#define INPUT_VIRTUAL_ZMK_BT_SEL_4      0x34
#define INPUT_VIRTUAL_ZMK_BT_SEL_5      0x35
#define INPUT_VIRTUAL_ZMK_BT_SEL_6      0x36
#define INPUT_VIRTUAL_ZMK_BT_SEL_7      0x37
#define INPUT_VIRTUAL_ZMK_BT_SEL_8      0x38
#define INPUT_VIRTUAL_ZMK_BT_SEL_9      0x39
#define INPUT_VIRTUAL_ZMK_BT_SEL_10     0x3A
#define INPUT_VIRTUAL_ZMK_BT_SEL_11     0x3B

#define INPUT_VIRTUAL_ZMK_BT_CLR_0      0x40
#define INPUT_VIRTUAL_ZMK_BT_CLR_1      0x41
#define INPUT_VIRTUAL_ZMK_BT_CLR_2      0x42
#define INPUT_VIRTUAL_ZMK_BT_CLR_3      0x43
#define INPUT_VIRTUAL_ZMK_BT_CLR_4      0x44
#define INPUT_VIRTUAL_ZMK_BT_CLR_5      0x45
#define INPUT_VIRTUAL_ZMK_BT_CLR_6      0x46
#define INPUT_VIRTUAL_ZMK_BT_CLR_7      0x47
#define INPUT_VIRTUAL_ZMK_BT_CLR_8      0x48
#define INPUT_VIRTUAL_ZMK_BT_CLR_9      0x49
#define INPUT_VIRTUAL_ZMK_BT_CLR_10     0x4A
#define INPUT_VIRTUAL_ZMK_BT_CLR_11     0x4B

#define INPUT_VIRTUAL_ZMK_BT_DISC_0     0x50
#define INPUT_VIRTUAL_ZMK_BT_DISC_1     0x51
#define INPUT_VIRTUAL_ZMK_BT_DISC_2     0x52
#define INPUT_VIRTUAL_ZMK_BT_DISC_3     0x53
#define INPUT_VIRTUAL_ZMK_BT_DISC_4     0x54
#define INPUT_VIRTUAL_ZMK_BT_DISC_5     0x55
#define INPUT_VIRTUAL_ZMK_BT_DISC_6     0x56
#define INPUT_VIRTUAL_ZMK_BT_DISC_7     0x57
#define INPUT_VIRTUAL_ZMK_BT_DISC_8     0x58
#define INPUT_VIRTUAL_ZMK_BT_DISC_9     0x59
#define INPUT_VIRTUAL_ZMK_BT_DISC_10    0x5A
#define INPUT_VIRTUAL_ZMK_BT_DISC_11    0x5B

/* ── Category 3: LVGL symbol codes ─────────────────────────────────────── */
/* Ordered to match lv_symbol_def.h. User-extensible from 0x13E onward.    */

#define INPUT_VIRTUAL_SYM_BULLET        0x100
#define INPUT_VIRTUAL_SYM_AUDIO         0x101
#define INPUT_VIRTUAL_SYM_VIDEO         0x102
#define INPUT_VIRTUAL_SYM_LIST          0x103
#define INPUT_VIRTUAL_SYM_OK            0x104
#define INPUT_VIRTUAL_SYM_CLOSE         0x105
#define INPUT_VIRTUAL_SYM_POWER         0x106
#define INPUT_VIRTUAL_SYM_SETTINGS      0x107
#define INPUT_VIRTUAL_SYM_HOME          0x108
#define INPUT_VIRTUAL_SYM_DOWNLOAD      0x109
#define INPUT_VIRTUAL_SYM_DRIVE         0x10A
#define INPUT_VIRTUAL_SYM_REFRESH       0x10B
#define INPUT_VIRTUAL_SYM_MUTE          0x10C
#define INPUT_VIRTUAL_SYM_VOLUME_MID    0x10D
#define INPUT_VIRTUAL_SYM_VOLUME_MAX    0x10E
#define INPUT_VIRTUAL_SYM_IMAGE         0x10F
#define INPUT_VIRTUAL_SYM_TINT          0x110
#define INPUT_VIRTUAL_SYM_PREV          0x111
#define INPUT_VIRTUAL_SYM_PLAY          0x112
#define INPUT_VIRTUAL_SYM_PAUSE         0x113
#define INPUT_VIRTUAL_SYM_STOP          0x114
#define INPUT_VIRTUAL_SYM_NEXT          0x115
#define INPUT_VIRTUAL_SYM_EJECT         0x116
#define INPUT_VIRTUAL_SYM_LEFT          0x117
#define INPUT_VIRTUAL_SYM_RIGHT         0x118
#define INPUT_VIRTUAL_SYM_PLUS          0x119
#define INPUT_VIRTUAL_SYM_MINUS         0x11A
#define INPUT_VIRTUAL_SYM_EYE_OPEN     0x11B
#define INPUT_VIRTUAL_SYM_EYE_CLOSE    0x11C
#define INPUT_VIRTUAL_SYM_WARNING       0x11D
#define INPUT_VIRTUAL_SYM_SHUFFLE       0x11E
#define INPUT_VIRTUAL_SYM_UP            0x11F
#define INPUT_VIRTUAL_SYM_DOWN          0x120
#define INPUT_VIRTUAL_SYM_LOOP          0x121
#define INPUT_VIRTUAL_SYM_DIRECTORY     0x122
#define INPUT_VIRTUAL_SYM_UPLOAD        0x123
#define INPUT_VIRTUAL_SYM_CALL          0x124
#define INPUT_VIRTUAL_SYM_CUT           0x125
#define INPUT_VIRTUAL_SYM_COPY          0x126
#define INPUT_VIRTUAL_SYM_SAVE          0x127
#define INPUT_VIRTUAL_SYM_BARS          0x128
#define INPUT_VIRTUAL_SYM_ENVELOPE      0x129
#define INPUT_VIRTUAL_SYM_CHARGE        0x12A
#define INPUT_VIRTUAL_SYM_PASTE         0x12B
#define INPUT_VIRTUAL_SYM_BELL          0x12C
#define INPUT_VIRTUAL_SYM_KEYBOARD      0x12D
#define INPUT_VIRTUAL_SYM_GPS           0x12E
#define INPUT_VIRTUAL_SYM_FILE          0x12F
#define INPUT_VIRTUAL_SYM_WIFI          0x130
#define INPUT_VIRTUAL_SYM_BATTERY_FULL  0x131
#define INPUT_VIRTUAL_SYM_BATTERY_3     0x132
#define INPUT_VIRTUAL_SYM_BATTERY_2     0x133
#define INPUT_VIRTUAL_SYM_BATTERY_1     0x134
#define INPUT_VIRTUAL_SYM_BATTERY_EMPTY 0x135
#define INPUT_VIRTUAL_SYM_USB           0x136
#define INPUT_VIRTUAL_SYM_BLUETOOTH     0x137
#define INPUT_VIRTUAL_SYM_TRASH         0x138
#define INPUT_VIRTUAL_SYM_BACKSPACE     0x139
#define INPUT_VIRTUAL_SYM_SD_CARD       0x13A
#define INPUT_VIRTUAL_SYM_NEW_LINE      0x13B
#define INPUT_VIRTUAL_SYM_DUMMY         0x13C
#define INPUT_VIRTUAL_SYM_EDIT          0x13D

/* ── Page indices for use in DTS overlays ───────────────────────────────── */

#define XIAORD_PAGE_HOME   0
#define XIAORD_PAGE_CLOCK  1
#define XIAORD_PAGE_BT     2
