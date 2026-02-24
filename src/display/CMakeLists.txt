# SPDX-License-Identifier: MIT

zephyr_library()
zephyr_library_sources(status_screen.c)
zephyr_library_sources_ifdef(CONFIG_XIAORD_VIRTUAL_BUTTONS virtual_buttons.c)
zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)