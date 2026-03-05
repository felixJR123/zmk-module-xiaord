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
