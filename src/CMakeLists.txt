add_library(
  ${PROJECT_NAME} MODULE
  # headers
  "ae-zoom.h"
  "ae-egg.h"
  "mangled-names.h"
  "options.h"
  "zoom-actions.h"
  "view-pano.h"
  "external-symbols.h"
  "JS/JS.h"
  "util-functions.h"
  # sources
  "ae-zoom.cpp"
  "ae-egg.cpp"
  "mangled-names.cpp"
  "options.cpp"
  "zoom-actions.cpp"
  "view-pano.cpp"
  "external-symbols.cpp"
  "JS/JS.cpp")

target_include_directories(${PROJECT_NAME} PRIVATE ../include)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE AESDK uiohook
                                              nlohmann_json::nlohmann_json)

if(WIN32)
  set(PLUGIN_FOLDER
      "C:/Program Files/Adobe/Adobe After Effects 2023/Support Files/Plug-ins"
      CACHE STRING "After Effects Plugin folder")

  if(CMAKE_HOST_WIN32)
    set(CMD_GENERATE_RRC
        ${AFX_REZ} ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rr
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rrc)
  elseif(CMAKE_HOST_UNIX AND NOT CMAKE_HOST_APPLE) # This currently works only
                                                   # on Windows Subsystem for
                                                   # Linux (WSL)
    execute_process(
      COMMAND bash ${PROJECT_SOURCE_DIR}/sh/check-if-wsl.sh
      OUTPUT_VARIABLE IS_WSL
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(IS_WSL)
      execute_process(
        COMMAND wslpath -m "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rr"
        OUTPUT_VARIABLE RR_WIN_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE)

      set(RRC_WIN_PATH "${RR_WIN_PATH}c")

      set(CMD_GENERATE_RRC ${AFX_REZ} ${RR_WIN_PATH} ${RRC_WIN_PATH})
    else()
      message(
        FATAL_ERROR
          "Not inside WSL. Running PiPLtool.exe is required for the build. Only WSL is currently supported."
      )
    endif()
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    separate_arguments(
      CMD_GENERATE_RR WINDOWS_COMMAND
      "${CMAKE_CXX_COMPILER} /I \"${AESDK_ROOT}/Headers\" /EP "
      "\"${PROJECT_SOURCE_DIR}/${PROJECT_NAME}.r\" > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rr"
    )

    separate_arguments(
      CMD_GENERATE_RC WINDOWS_COMMAND
      "${CMAKE_CXX_COMPILER} /D \"MSWindows\" /EP "
      "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rrc > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc"
    )
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL
                                                 "Clang")
    separate_arguments(
      CMD_GENERATE_RR
      UNIX_COMMAND
      "${CMAKE_CXX_COMPILER} -I \"${AESDK_ROOT}/Headers\" -E -P -x c \
      ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}.r -o ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rr"
    )

    separate_arguments(
      CMD_GENERATE_RC
      UNIX_COMMAND
      "${CMAKE_CXX_COMPILER} -D \"MSWindows\" -E -P -x c \
      ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rrc -o ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc"
    )
  endif()

  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rr
                     COMMAND ${CMD_GENERATE_RR})

  add_custom_command(
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rr
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rrc
    COMMAND ${CMD_GENERATE_RRC})

  add_custom_command(
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rrc
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc
    COMMAND ${CMD_GENERATE_RC})

  target_sources(${PROJECT_NAME}
                 PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc)
elseif(APPLE)
  set(PLUGIN_FOLDER
      "/Applications/Adobe After Effects 2023/Plug-ins"
      CACHE STRING "After Effects Plugin folder")

  if(CMAKE_HOST_APPLE) # if host is macOS we can compile the resource file
    set(${PIPL_INCLUDES} "")

    foreach(INC ${AESDK_INCLUDE})
      list(APPEND PIPL_INCLUDES -i ${INC})
    endforeach()

    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rsrc
      COMMAND
        ${AFX_REZ} ARGS ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}.r -o
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rsrc -useDF
        ${PIPL_INCLUDES} -D __MACH__)

    set(RSRC_MAC_FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rsrc)
  else() # on other host platforms we have to take the resource file that was
         # previously compiled using macOS
    set(RSRC_MAC_FILE ${PROJECT_SOURCE_DIR}/Zoom.rsrc)
  endif()

  set_source_files_properties(${RSRC_MAC_FILE}
                              PROPERTIES MACOSX_PACKAGE_LOCATION Resources/)

  target_sources(${PROJECT_NAME} PRIVATE ${RSRC_MAC_FILE})
endif()

if(WIN32)
  set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
  set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".aex")

  install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.aex"
          DESTINATION "${PLUGIN_FOLDER}/")
elseif(APPLE)
  set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
  set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX "")

  set_target_properties(${PROJECT_NAME} PROPERTIES BUNDLE ON)
  set_target_properties(${PROJECT_NAME} PROPERTIES BUNDLE_EXTENSION "plugin")

  # PkgInfo
  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/PkgInfo
    COMMAND echo "AEgxFXTC" > ${CMAKE_BINARY_DIR}/PkgInfo)
  set_source_files_properties(${CMAKE_BINARY_DIR}/PkgInfo
                              PROPERTIES MACOSX_PACKAGE_LOCATION "")
  target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/PkgInfo)

  install(DIRECTORY "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.plugin"
          DESTINATION "${PLUGIN_FOLDER}/")

  if(NOT CMAKE_HOST_APPLE)
    # create debug symbols
    add_custom_command(
      TARGET ${PROJECT_NAME}
      POST_BUILD USES_TERMINAL
      COMMAND
        dsymutil
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.plugin/Contents/MacOS/${PROJECT_NAME}
        -o ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.dSYM)

    # create lldb start-up script
    set(LLDB_REMOTE_NAME Deniss-MacBook-Air.local)
    set(LLDB_REMOTE_PORT 16000)
    file(
      WRITE ${CMAKE_BINARY_DIR}/lldb-remote-start-up.txt
      "platform select remote-macosx\n"
      "platform connect connect://${LLDB_REMOTE_NAME}:${LLDB_REMOTE_PORT}\n"
      "setting set target.memory-module-load-level partial\n"
      "attach -n \"After Effects\"\n"
      "target symbols add ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.dSYM/Contents/Resources/DWARF/${PROJECT_NAME}"
    )
  endif()
endif()
