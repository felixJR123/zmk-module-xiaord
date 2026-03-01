# SPDX-License-Identifier: MIT

zephyr_library()
zephyr_library_sources(status_screen.c)
zephyr_library_sources(page_home.c)
zephyr_library_sources(page_clock.c)
zephyr_library_sources(page_macropad.c)
zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
