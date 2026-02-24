# SPDX-License-Identifier: MIT

zephyr_library()
zephyr_library_sources(input_processor_code_canceller.c)
zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)