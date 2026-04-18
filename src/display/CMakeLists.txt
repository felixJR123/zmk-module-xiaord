# SPDX-License-Identifier: MIT

zephyr_library()

# Expose ZMK's app/include (PRIVATE in ZMK's own CMakeLists) so we can use
# zmk/display.h (ZMK_DISPLAY_WIDGET_LISTENER) and other ZMK headers.
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)

# Top-level headers (page_iface.h, display_api.h, xiaord_input_codes.h)
# visible to all subdirectory compilation units.
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Subdirectory headers visible globally (no relative-path includes needed).
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/listeners)
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ui)

if(CONFIG_XIAORD_BG_SD)
  set(XIAORD_FATFS_INCLUDE_CANDIDATES)
  if(DEFINED ZEPHYR_FATFS_MODULE_DIR)
    list(APPEND XIAORD_FATFS_INCLUDE_CANDIDATES "${ZEPHYR_FATFS_MODULE_DIR}/include")
  endif()
  if(DEFINED WEST_TOPDIR)
    list(APPEND XIAORD_FATFS_INCLUDE_CANDIDATES
      "${WEST_TOPDIR}/modules/fatfs/include"
      "${WEST_TOPDIR}/modules/fs/fatfs/include"
    )
  endif()
  list(APPEND XIAORD_FATFS_INCLUDE_CANDIDATES
    "${ZEPHYR_BASE}/../modules/fatfs/include"
    "${ZEPHYR_BASE}/../modules/fs/fatfs/include"
  )
  foreach(XIAORD_FATFS_INCLUDE ${XIAORD_FATFS_INCLUDE_CANDIDATES})
    if(EXISTS "${XIAORD_FATFS_INCLUDE}/ff.h")
      zephyr_include_directories(${XIAORD_FATFS_INCLUDE})
      break()
    endif()
  endforeach()
endif()

add_subdirectory(listeners)
add_subdirectory(ui)
add_subdirectory(pages)

zephyr_library_sources(status_screen.c)
