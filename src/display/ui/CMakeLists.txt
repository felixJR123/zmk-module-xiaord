# SPDX-License-Identifier: MIT

zephyr_library_sources(ui_btn.c)
zephyr_library_sources(home_buttons.c)

if(CONFIG_XIAORD_BG_4 OR CONFIG_XIAORD_BG_5 OR CONFIG_XIAORD_BG_6)
  zephyr_library_sources_ifdef(CONFIG_XIAORD_BG_4 bg/bg4.c)
  zephyr_library_sources_ifdef(CONFIG_XIAORD_BG_5 bg/bg5.c)
  zephyr_library_sources_ifdef(CONFIG_XIAORD_BG_6 bg/bg6.c)
else()
  zephyr_library_sources_ifdef(CONFIG_XIAORD_BG_1 bg/bg1.c)
  zephyr_library_sources_ifdef(CONFIG_XIAORD_BG_2 bg/bg2.c)
  zephyr_library_sources_ifdef(CONFIG_XIAORD_BG_3 bg/bg3.c)
endif()
