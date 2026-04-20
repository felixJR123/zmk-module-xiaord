# SPDX-License-Identifier: MIT

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_ZMK_DISPLAY behavior_xiaord_menu.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_DISPLAY behavior_xiaord_home_info.c)
