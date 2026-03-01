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
#define INPUT_EV_ZMK_BEHAVIOR 0xF1

/* Behavior slot indices — used as event codes with INPUT_EV_ZMK_BEHAVIOR. */
#define SS_KEY_0    0
#define SS_KEY_1    1
#define SS_KEY_2    2
#define SS_KEY_3    3
#define SS_KEY_4    4
#define SS_KEY_COUNT 5
