# SPDX-License-Identifier: MIT

zephyr_library_sources(ui_btn.c)
zephyr_library_sources(home_buttons.c)
if(CONFIG_XIAORD_BG_ROTATE)
  zephyr_library_sources(bg/bg1.c bg/bg2.c bg/bg3.c)
else()
  zephyr_library_sources_ifdef(CONFIG_XIAORD_BG_1 bg/bg1.c)
  zephyr_library_sources_ifdef(CONFIG_XIAORD_BG_2 bg/bg2.c)
  zephyr_library_sources_ifdef(CONFIG_XIAORD_BG_3 bg/bg3.c)
endif()
