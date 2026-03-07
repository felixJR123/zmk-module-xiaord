/*
 * SPDX-License-Identifier: MIT
 *
 * Bluetooth settings screen.
 *
 * Layout (screen 240×240, center 120,120):
 *
 *   Profile radio buttons on circle perimeter (r=110, 32×32, up to 5):
 *     0  12 o'clock  offset (  0, -110)
 *     1  11 o'clock  offset (-55,  -95)
 *     2   1 o'clock  offset ( 55,  -95)
 *     3  10 o'clock  offset (-95,  -55)
 *     4   2 o'clock  offset ( 95,  -55)
 *
 *   Inner area (r < ~80 from center):
 *     Center  : output status label
 *     Upper   : [SEL] [-45] and [CLR] [+45] rectangular buttons
 *     Lower   : [⌂] [-38] and [USB] [+38] circle buttons
 */

#include <lvgl.h>
#include <zmk/endpoints.h>
#include "page_ops.h"
#include "bt_status.h"

#if IS_ENABLED(CONFIG_ZMK_BLE)
#include <zmk/ble.h>
#endif

/* ── Profile count ──────────────────────────────────────────────────────── */

#if IS_ENABLED(CONFIG_ZMK_BLE)
#define BT_PROFILE_COUNT_RAW \
	(CONFIG_BT_MAX_CONN - CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS)
#define BT_PROFILE_COUNT MIN(BT_PROFILE_COUNT_RAW, 5)
#else
#define BT_PROFILE_COUNT 0
#endif

/* ── Profile button positions (x,y offset from screen center) ──────────── */

static const int16_t s_profile_offsets[5][2] = {
	{   0, -110 }, /* 0: 12 o'clock */
	{ -55,  -95 }, /* 1: 11 o'clock */
	{  55,  -95 }, /* 2:  1 o'clock */
	{ -95,  -55 }, /* 3: 10 o'clock */
	{  95,  -55 }, /* 4:  2 o'clock */
};

/* ── State ──────────────────────────────────────────────────────────────── */

static int s_selected_profile;
static lv_obj_t *s_profile_btns[5];

/* ── Button style ───────────────────────────────────────────────────────── */

static lv_style_t s_style_rect;
static bool s_styles_init;

static void init_styles(void)
{
	if (s_styles_init) {
		return;
	}
	s_styles_init = true;

	lv_style_init(&s_style_rect);
	lv_style_set_bg_color(&s_style_rect, lv_palette_main(LV_PALETTE_BLUE_GREY));
	lv_style_set_bg_opa(&s_style_rect, LV_OPA_COVER);
	lv_style_set_border_color(&s_style_rect, lv_color_white());
	lv_style_set_border_width(&s_style_rect, 2);
	lv_style_set_radius(&s_style_rect, 8);
}

/* ── Profile selection helper ───────────────────────────────────────────── */

static void set_selected_profile(int idx)
{
	s_selected_profile = idx;
	for (int i = 0; i < BT_PROFILE_COUNT; i++) {
		lv_obj_set_style_bg_opa(s_profile_btns[i],
					(i == idx) ? LV_OPA_COVER : LV_OPA_30, 0);
	}
}

/* ── Callbacks ──────────────────────────────────────────────────────────── */

static void profile_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}
	int idx = (int)(uintptr_t)lv_event_get_user_data(e);
	set_selected_profile(idx);
}

static void sel_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}
	ss_fire_behavior(SS_SEL_0 + s_selected_profile);
}

static void clr_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}
	ss_fire_behavior(SS_CLR_0 + s_selected_profile);
}

static void home_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}
	ss_navigate_to(PAGE_HOME);
}

static void usb_btn_cb(lv_event_t *e)
{
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}
	ss_fire_behavior(SS_USB);
}

/* ── Widget factories ───────────────────────────────────────────────────── */

static lv_obj_t *make_rect_btn(lv_obj_t *parent, const char *text,
			       int x_off, int y_off, int w, int h,
			       lv_event_cb_t cb, void *user_data)
{
	lv_obj_t *btn = lv_obj_create(parent);
	lv_obj_set_size(btn, w, h);
	lv_obj_align(btn, LV_ALIGN_CENTER, x_off, y_off);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_style(btn, &s_style_rect, LV_PART_MAIN);
	lv_obj_add_event_cb(btn, cb, LV_EVENT_ALL, user_data);

	lv_obj_t *lbl = lv_label_create(btn);
	lv_label_set_text(lbl, text);
	lv_obj_center(lbl);

	return btn;
}

static lv_obj_t *make_circle_btn(lv_obj_t *parent, const char *symbol,
				 int x_off, int y_off,
				 lv_event_cb_t cb, void *user_data)
{
	lv_obj_t *btn = lv_obj_create(parent);
	lv_obj_set_size(btn, 44, 44);
	lv_obj_align(btn, LV_ALIGN_CENTER, x_off, y_off);
	lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
	lv_obj_set_style_bg_color(btn, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(btn, LV_OPA_30, 0);
	lv_obj_set_style_border_width(btn, 0, 0);
	lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_event_cb(btn, cb, LV_EVENT_ALL, user_data);

	lv_obj_t *lbl = lv_label_create(btn);
	lv_label_set_text(lbl, symbol);
	lv_obj_center(lbl);

	return btn;
}

/* ── Page create ────────────────────────────────────────────────────────── */

static int page_bt_create(lv_obj_t *screen)
{
	init_styles();

	/* Profile radio buttons on perimeter */
	for (int i = 0; i < BT_PROFILE_COUNT; i++) {
		char label[4];
		snprintf(label, sizeof(label), "%d", i);

		lv_obj_t *btn = lv_obj_create(screen);
		lv_obj_set_size(btn, 32, 32);
		lv_obj_align(btn, LV_ALIGN_CENTER,
			     s_profile_offsets[i][0], s_profile_offsets[i][1]);
		lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
		lv_obj_set_style_bg_color(btn, lv_color_white(), 0);
		lv_obj_set_style_bg_opa(btn, (i == 0) ? LV_OPA_COVER : LV_OPA_30, 0);
		lv_obj_set_style_border_width(btn, 0, 0);
		lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_add_event_cb(btn, profile_btn_cb, LV_EVENT_ALL, (void *)(uintptr_t)i);

		lv_obj_t *lbl = lv_label_create(btn);
		lv_label_set_text(lbl, label);
		lv_obj_center(lbl);

		s_profile_btns[i] = btn;
	}

	/* Output status label — screen center */
	lv_obj_t *output_lbl = create_output_status_label(screen, &lv_font_montserrat_24);
	lv_obj_align(output_lbl, LV_ALIGN_CENTER, 0, 0);
	bt_status_init(output_lbl);

	/* SEL / CLR rectangular buttons — upper inner area */
	make_rect_btn(screen, LV_SYMBOL_OK,    -38, -45, 60, 30, sel_btn_cb, NULL);
	make_rect_btn(screen, LV_SYMBOL_TRASH,  38, -45, 60, 30, clr_btn_cb, NULL);

	/* HOME / USB circle buttons — lower inner area */
	make_circle_btn(screen, LV_SYMBOL_HOME, -38, 55, home_btn_cb, NULL);
	make_circle_btn(screen, LV_SYMBOL_USB,   38, 55, usb_btn_cb, NULL);

	return 0;
}

/* ── Page enter ─────────────────────────────────────────────────────────── */

static void page_bt_enter(void)
{
	/* Sync radio button selection to current active BLE profile */
#if IS_ENABLED(CONFIG_ZMK_BLE)
	struct zmk_endpoint_instance ep = zmk_endpoint_get_selected();
	if (ep.transport == ZMK_TRANSPORT_BLE) {
		int idx = ep.ble.profile_index;
		if (idx >= 0 && idx < BT_PROFILE_COUNT) {
			set_selected_profile(idx);
		}
	}
#endif
}

/* ── Page ops ───────────────────────────────────────────────────────────── */

const struct page_ops page_bt_ops = {
	.name     = "bt",
	.create   = page_bt_create,
	.on_enter = page_bt_enter,
	.on_leave = NULL,
};
