# SPDX-License-Identifier: MIT

zephyr_library_sources(ui_btn.c)
zephyr_library_sources(home_buttons.c)

if(CONFIG_XIAORD_BG_4)
  set(XIAORD_BG4_C "${CMAKE_CURRENT_SOURCE_DIR}/bg/bg4.c")

  if(NOT EXISTS "${XIAORD_BG4_C}" AND NOT "${CONFIG_XIAORD_BG_4_SOURCE_DIR}" STREQUAL "")
    file(TO_CMAKE_PATH "${CONFIG_XIAORD_BG_4_SOURCE_DIR}" XIAORD_BG_4_SOURCE_DIR_NORMALIZED)

    if(EXISTS "${XIAORD_BG_4_SOURCE_DIR_NORMALIZED}")
      file(GLOB XIAORD_BG_4_SOURCES LIST_DIRECTORIES false
        "${XIAORD_BG_4_SOURCE_DIR_NORMALIZED}/*.jpg"
        "${XIAORD_BG_4_SOURCE_DIR_NORMALIZED}/*.jpeg"
        "${XIAORD_BG_4_SOURCE_DIR_NORMALIZED}/*.png"
      )
      list(SORT XIAORD_BG_4_SOURCES)
      list(LENGTH XIAORD_BG_4_SOURCES XIAORD_BG_4_SOURCE_COUNT)

      if(XIAORD_BG_4_SOURCE_COUNT GREATER 0)
        list(GET XIAORD_BG_4_SOURCES 0 XIAORD_BG_4_SOURCE)
        find_package(Python3 COMPONENTS Interpreter QUIET)

        if(Python3_Interpreter_FOUND)
          execute_process(
            COMMAND
              "${Python3_EXECUTABLE}"
              "${CMAKE_CURRENT_SOURCE_DIR}/../../../tools/convert_xiaord_bg.py"
              "${XIAORD_BG_4_SOURCE}"
              4
              --out-dir
              "${CMAKE_CURRENT_SOURCE_DIR}/bg"
            RESULT_VARIABLE XIAORD_BG_4_CONVERT_RESULT
            OUTPUT_VARIABLE XIAORD_BG_4_CONVERT_OUTPUT
            ERROR_VARIABLE XIAORD_BG_4_CONVERT_ERROR
          )

          if(NOT XIAORD_BG_4_CONVERT_RESULT EQUAL 0)
            message(WARNING "XIAORD_BG_4 conversion failed; falling back to BG_1. ${XIAORD_BG_4_CONVERT_ERROR}")
          endif()
        else()
          message(WARNING "Python3 was not found; falling back to BG_1 for XIAORD_BG_4.")
        endif()
      else()
        message(WARNING "No JPG/PNG files found in CONFIG_XIAORD_BG_4_SOURCE_DIR; falling back to BG_1.")
      endif()
    else()
      message(WARNING "CONFIG_XIAORD_BG_4_SOURCE_DIR does not exist; falling back to BG_1.")
    endif()
  endif()

  if(EXISTS "${XIAORD_BG4_C}")
    zephyr_library_sources(bg/bg4.c)
    zephyr_compile_definitions(XIAORD_LOCAL_BG4_AVAILABLE=1)
  else()
    zephyr_library_sources(bg/bg1.c)
  endif()
elseif(CONFIG_XIAORD_BG_1)
  zephyr_library_sources(bg/bg1.c)
elseif(CONFIG_XIAORD_BG_2)
  zephyr_library_sources(bg/bg2.c)
elseif(CONFIG_XIAORD_BG_3)
  zephyr_library_sources(bg/bg3.c)
elseif(CONFIG_XIAORD_BG_5 OR CONFIG_XIAORD_BG_6)
  message(WARNING "CONFIG_XIAORD_BG_5/BG_6 are deprecated; falling back to BG_1.")
  zephyr_library_sources(bg/bg1.c)
endif()
