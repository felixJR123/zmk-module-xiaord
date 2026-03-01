# Copyright (c) 2024 The ZMK Contributors
# SPDX-License-Identifier: MIT

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_INPUT_CHSC6X_CUSTOM input_chsc6x_custom.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_VIRTUAL_KEY_SOURCE virtual_key_source.c)

# Expose virtual_key_source.h to the rest of the build (e.g. status_screen.c)
zephyr_include_directories_ifdef(CONFIG_ZMK_VIRTUAL_KEY_SOURCE .)
