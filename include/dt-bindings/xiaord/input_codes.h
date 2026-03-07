/*
 * SPDX-License-Identifier: MIT
 *
 * Virtual input event codes for INPUT_EV_ZMK_BEHAVIORS (0xF1).
 * Zephyr reserves INPUT_EV_VENDOR_START (0xF0) .. INPUT_EV_VENDOR_STOP (0xFF)
 * for vendor use. We claim 0xF1 for behavior events.
 *
 * Two categories:
 *   INPUT_VIRTUAL_SYM_<SYMBOL>    — LVGL symbol codes  (0x00-0x3D)
 *   INPUT_VIRTUAL_ZMK_<behavior>  — ZMK BT behavior codes (0x40-0x7B)
 *
 * Usable from both C source and DTS overlays via #include.
 */

#pragma once

/* Custom event type: "invoke ZMK behavior by index". */
#define INPUT_EV_ZMK_BEHAVIORS 0xF1

/* ── Category 1: LVGL symbol codes ─────────────────────────────────────── */
/* Ordered to match lv_symbol_def.h. User-extensible from 0x3E onward.     */

#define INPUT_VIRTUAL_SYM_BASE          0x00
#define INPUT_VIRTUAL_SYM_MAX           0x3D

#define INPUT_VIRTUAL_SYM_BULLET        0x00
#define INPUT_VIRTUAL_SYM_AUDIO         0x01
#define INPUT_VIRTUAL_SYM_VIDEO         0x02
#define INPUT_VIRTUAL_SYM_LIST          0x03
#define INPUT_VIRTUAL_SYM_OK            0x04
#define INPUT_VIRTUAL_SYM_CLOSE         0x05
#define INPUT_VIRTUAL_SYM_POWER         0x06
#define INPUT_VIRTUAL_SYM_SETTINGS      0x07
#define INPUT_VIRTUAL_SYM_HOME          0x08
#define INPUT_VIRTUAL_SYM_DOWNLOAD      0x09
#define INPUT_VIRTUAL_SYM_DRIVE         0x0A
#define INPUT_VIRTUAL_SYM_REFRESH       0x0B
#define INPUT_VIRTUAL_SYM_MUTE          0x0C
#define INPUT_VIRTUAL_SYM_VOLUME_MID    0x0D
#define INPUT_VIRTUAL_SYM_VOLUME_MAX    0x0E
#define INPUT_VIRTUAL_SYM_IMAGE         0x0F
#define INPUT_VIRTUAL_SYM_TINT          0x10
#define INPUT_VIRTUAL_SYM_PREV          0x11
#define INPUT_VIRTUAL_SYM_PLAY          0x12
#define INPUT_VIRTUAL_SYM_PAUSE         0x13
#define INPUT_VIRTUAL_SYM_STOP          0x14
#define INPUT_VIRTUAL_SYM_NEXT          0x15
#define INPUT_VIRTUAL_SYM_EJECT         0x16
#define INPUT_VIRTUAL_SYM_LEFT          0x17
#define INPUT_VIRTUAL_SYM_RIGHT         0x18
#define INPUT_VIRTUAL_SYM_PLUS          0x19
#define INPUT_VIRTUAL_SYM_MINUS         0x1A
#define INPUT_VIRTUAL_SYM_EYE_OPEN     0x1B
#define INPUT_VIRTUAL_SYM_EYE_CLOSE    0x1C
#define INPUT_VIRTUAL_SYM_WARNING       0x1D
#define INPUT_VIRTUAL_SYM_SHUFFLE       0x1E
#define INPUT_VIRTUAL_SYM_UP            0x1F
#define INPUT_VIRTUAL_SYM_DOWN          0x20
#define INPUT_VIRTUAL_SYM_LOOP          0x21
#define INPUT_VIRTUAL_SYM_DIRECTORY     0x22
#define INPUT_VIRTUAL_SYM_UPLOAD        0x23
#define INPUT_VIRTUAL_SYM_CALL          0x24
#define INPUT_VIRTUAL_SYM_CUT           0x25
#define INPUT_VIRTUAL_SYM_COPY          0x26
#define INPUT_VIRTUAL_SYM_SAVE          0x27
#define INPUT_VIRTUAL_SYM_BARS          0x28
#define INPUT_VIRTUAL_SYM_ENVELOPE      0x29
#define INPUT_VIRTUAL_SYM_CHARGE        0x2A
#define INPUT_VIRTUAL_SYM_PASTE         0x2B
#define INPUT_VIRTUAL_SYM_BELL          0x2C
#define INPUT_VIRTUAL_SYM_KEYBOARD      0x2D
#define INPUT_VIRTUAL_SYM_GPS           0x2E
#define INPUT_VIRTUAL_SYM_FILE          0x2F
#define INPUT_VIRTUAL_SYM_WIFI          0x30
#define INPUT_VIRTUAL_SYM_BATTERY_FULL  0x31
#define INPUT_VIRTUAL_SYM_BATTERY_3     0x32
#define INPUT_VIRTUAL_SYM_BATTERY_2     0x33
#define INPUT_VIRTUAL_SYM_BATTERY_1     0x34
#define INPUT_VIRTUAL_SYM_BATTERY_EMPTY 0x35
#define INPUT_VIRTUAL_SYM_USB           0x36
#define INPUT_VIRTUAL_SYM_BLUETOOTH     0x37
#define INPUT_VIRTUAL_SYM_TRASH         0x38
#define INPUT_VIRTUAL_SYM_BACKSPACE     0x39
#define INPUT_VIRTUAL_SYM_SD_CARD       0x3A
#define INPUT_VIRTUAL_SYM_NEW_LINE      0x3B
#define INPUT_VIRTUAL_SYM_DUMMY         0x3C
#define INPUT_VIRTUAL_SYM_EDIT          0x3D

/* ── Category 2: ZMK BT behavior codes ─────────────────────────────────── */
/*
 * 0x40-0x43  BT management (requires: CONFIG_ZMK_BLE)
 * 0x50-0x5B  BT_SEL n    (requires: CONFIG_ZMK_BLE)
 * 0x60-0x6B  BT_CLR n    (per-profile; leave unmapped or define via keyboard overlay)
 */

#define INPUT_VIRTUAL_ZMK_BT_CLR        0x40
#define INPUT_VIRTUAL_ZMK_BT_CLR_ALL    0x41
#define INPUT_VIRTUAL_ZMK_BT_NXT        0x42
#define INPUT_VIRTUAL_ZMK_BT_PRV        0x43

#define INPUT_VIRTUAL_ZMK_OUT_USB       0x44
#define INPUT_VIRTUAL_ZMK_OUT_BLE       0x45
#define INPUT_VIRTUAL_ZMK_OUT_TOG       0x46

#define INPUT_VIRTUAL_ZMK_BT_SEL_0      0x50
#define INPUT_VIRTUAL_ZMK_BT_SEL_1      0x51
#define INPUT_VIRTUAL_ZMK_BT_SEL_2      0x52
#define INPUT_VIRTUAL_ZMK_BT_SEL_3      0x53
#define INPUT_VIRTUAL_ZMK_BT_SEL_4      0x54
#define INPUT_VIRTUAL_ZMK_BT_SEL_5      0x55
#define INPUT_VIRTUAL_ZMK_BT_SEL_6      0x56
#define INPUT_VIRTUAL_ZMK_BT_SEL_7      0x57
#define INPUT_VIRTUAL_ZMK_BT_SEL_8      0x58
#define INPUT_VIRTUAL_ZMK_BT_SEL_9      0x59
#define INPUT_VIRTUAL_ZMK_BT_SEL_10     0x5A
#define INPUT_VIRTUAL_ZMK_BT_SEL_11     0x5B

#define INPUT_VIRTUAL_ZMK_BT_CLR_0      0x60
#define INPUT_VIRTUAL_ZMK_BT_CLR_1      0x61
#define INPUT_VIRTUAL_ZMK_BT_CLR_2      0x62
#define INPUT_VIRTUAL_ZMK_BT_CLR_3      0x63
#define INPUT_VIRTUAL_ZMK_BT_CLR_4      0x64
#define INPUT_VIRTUAL_ZMK_BT_CLR_5      0x65
#define INPUT_VIRTUAL_ZMK_BT_CLR_6      0x66
#define INPUT_VIRTUAL_ZMK_BT_CLR_7      0x67
#define INPUT_VIRTUAL_ZMK_BT_CLR_8      0x68
#define INPUT_VIRTUAL_ZMK_BT_CLR_9      0x69
#define INPUT_VIRTUAL_ZMK_BT_CLR_10     0x6A
#define INPUT_VIRTUAL_ZMK_BT_CLR_11     0x6B

/* ── Page indices for use in DTS overlays ───────────────────────────────── */

#define XIAORD_PAGE_HOME   0
#define XIAORD_PAGE_CLOCK  1
#define XIAORD_PAGE_BT     2
