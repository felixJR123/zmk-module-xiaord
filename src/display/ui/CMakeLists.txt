# SPDX-License-Identifier: MIT

zephyr_library_sources(ui_btn.c)
zephyr_library_sources(home_buttons.c)

if(CONFIG_XIAORD_BG_4)
  zephyr_library_sources(bg/bg4.c)
elseif(CONFIG_XIAORD_BG_5)
  zephyr_library_sources(bg/bg5.c)
elseif(CONFIG_XIAORD_BG_6)
  zephyr_library_sources(bg/bg6.c)
elseif(CONFIG_XIAORD_BG_1)
  zephyr_library_sources(bg/bg1.c)
elseif(CONFIG_XIAORD_BG_2)
  zephyr_library_sources(bg/bg2.c)
elseif(CONFIG_XIAORD_BG_3)
  zephyr_library_sources(bg/bg3.c)
endif()
