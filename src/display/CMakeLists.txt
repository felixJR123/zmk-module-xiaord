# SPDX-License-Identifier: MIT

zephyr_library()

# Expose ZMK's app/include (PRIVATE in ZMK's own CMakeLists) so we can use
# zmk/display.h (ZMK_DISPLAY_WIDGET_LISTENER) and other ZMK headers.
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)

zephyr_library_sources(ui_btn.c)
zephyr_library_sources(image-mesh-gradient.c)
zephyr_library_sources(status_screen.c)
zephyr_library_sources(home_status.c)
zephyr_library_sources(bt_status.c)
zephyr_library_sources(home_buttons.c)
zephyr_library_sources(page_home.c)
zephyr_library_sources(page_clock.c)
zephyr_library_sources(page_bt.c)
