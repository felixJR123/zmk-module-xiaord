/*
 * SPDX-License-Identifier: MIT
 *
 * Vendor-specific input event type for ZMK behavior invocation.
 * Zephyr reserves INPUT_EV_VENDOR_START (0xF0) .. INPUT_EV_VENDOR_STOP (0xFF)
 * for vendor use. We claim 0xF1 for behavior events.
 *
 * Usable from both C source and DTS overlays via #include.
 */

#pragma once

/* Custom event type: "invoke ZMK behavior by index". */
#define INPUT_EV_ZMK_BEHAVIORS 0xF1

/* Behavior slot indices — used as event codes with INPUT_EV_ZMK_BEHAVIORS. */
#define SS_KEY_0    0
#define SS_KEY_1    1
#define SS_KEY_2    2
#define SS_KEY_3    3
#define SS_KEY_4    4
#define SS_KEY_COUNT 5

/* ZMK system/BT behavior codes — used as event codes with INPUT_EV_ZMK_BEHAVIORS.
 * Ranges:
 *   0x10-0x1F  system      (requires: none / CONFIG_ZMK_PM_SOFT_OFF / CONFIG_ZMK_STUDIO)
 *   0x20-0x2F  BT management (requires: CONFIG_ZMK_BLE)
 *   0x30-0x3F  BT_SEL n      (requires: CONFIG_ZMK_BLE)
 *   0x40-0x4F  BT_DISC n     (requires: CONFIG_ZMK_BLE)
 */
#define ZMK_BEHAVIOR_SYS_RESET      0x10
#define ZMK_BEHAVIOR_BOOTLOADER     0x11
#define ZMK_BEHAVIOR_SOFT_OFF       0x12  /* CONFIG_ZMK_PM_SOFT_OFF=y */
#define ZMK_BEHAVIOR_STUDIO_UNLOCK  0x13  /* CONFIG_ZMK_STUDIO=y */

#define ZMK_BEHAVIOR_BT_CLR         0x20
#define ZMK_BEHAVIOR_BT_CLR_ALL     0x21
#define ZMK_BEHAVIOR_BT_NXT         0x22
#define ZMK_BEHAVIOR_BT_PRV         0x23

#define ZMK_BEHAVIOR_BT_SEL_0       0x30
#define ZMK_BEHAVIOR_BT_SEL_1       0x31
#define ZMK_BEHAVIOR_BT_SEL_2       0x32
#define ZMK_BEHAVIOR_BT_SEL_3       0x33
#define ZMK_BEHAVIOR_BT_SEL_4       0x34

#define ZMK_BEHAVIOR_BT_DISC_0      0x40
#define ZMK_BEHAVIOR_BT_DISC_1      0x41
#define ZMK_BEHAVIOR_BT_DISC_2      0x42
#define ZMK_BEHAVIOR_BT_DISC_3      0x43
#define ZMK_BEHAVIOR_BT_DISC_4      0x44

/* Circle-button codes — used as event codes with INPUT_EV_ZMK_BEHAVIORS.
 *   0x50-0x5F  home screen circle buttons */
#define SS_UPLOAD      0x50
#define SS_POWER       0x51
#define SS_VOLUME_MAX  0x52
#define SS_MUTE        0x53
#define SS_VOLUME_MID  0x54
#define SS_PLUS        0x55
#define SS_MINUS       0x56
#define SS_EYE_CLOSE   0x57
#define SS_USB         0x58
#define SS_BLUETOOTH   0x59
#define SS_HOME        0x5A
#define SS_SETTINGS    0x5B

/* BT settings screen codes — 16-slot ranges for future expansion.
 *   0x60-0x6F  BT CLR by profile index
 *   0x70-0x7F  BT SEL by profile index
 * Use SS_CLR_0 + index / SS_SEL_0 + index for dynamic offset arithmetic. */
#define SS_CLR_0    0x60
#define SS_CLR_1    0x61
#define SS_CLR_2    0x62
#define SS_CLR_3    0x63
#define SS_CLR_4    0x64

#define SS_SEL_0    0x70
#define SS_SEL_1    0x71
#define SS_SEL_2    0x72
#define SS_SEL_3    0x73
#define SS_SEL_4    0x74
