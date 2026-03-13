/*
 * SPDX-License-Identifier: MIT
 *
 * Unicode codepoint → UTF-8 conversion utility for LVGL icon rendering.
 */

#pragma once
#include <stdint.h>

/* Unicode codepoint → UTF-8 string (max 4 bytes + null) */
static inline int unicode_to_utf8(uint32_t cp, char *buf)
{
	if (cp < 0x80) {
		buf[0] = (char)cp; buf[1] = 0; return 1;
	} else if (cp < 0x800) {
		buf[0] = 0xC0 | (cp >> 6);
		buf[1] = 0x80 | (cp & 0x3F);
		buf[2] = 0; return 2;
	} else if (cp < 0x10000) {
		buf[0] = 0xE0 | (cp >> 12);
		buf[1] = 0x80 | ((cp >> 6) & 0x3F);
		buf[2] = 0x80 | (cp & 0x3F);
		buf[3] = 0; return 3;
	} else {
		buf[0] = 0xF0 | (cp >> 18);
		buf[1] = 0x80 | ((cp >> 12) & 0x3F);
		buf[2] = 0x80 | ((cp >> 6) & 0x3F);
		buf[3] = 0x80 | (cp & 0x3F);
		buf[4] = 0; return 4;
	}
}
