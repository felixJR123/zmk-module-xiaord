# SPDX-License-Identifier: Apache-2.0
add_subdirectory(drivers)

# # Create library target for this module
# zephyr_library()

# # Add custom driver sources only when xiaord shield is enabled
# if(CONFIG_SHIELD_XIAORD)
#     zephyr_library_sources(drivers/input/input_chsc6x_custom.c)
# endif()
