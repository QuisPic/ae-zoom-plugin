cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 20)

if(APPLE)
  # Create Universal Binary
  set(CMAKE_OSX_ARCHITECTURES
      "arm64;x86_64"
      CACHE STRING "" FORCE)

  # Disable warnings
  add_compile_options(-Wno-pragma-pack)
endif()

project("Zoom")

add_subdirectory(extern)
add_subdirectory(src)

set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                 "${CMAKE_BINARY_DIR}")

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
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/PkgInfo
    COMMAND echo "AEgxFXTC" > ${CMAKE_CURRENT_BINARY_DIR}/PkgInfo)
  set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/PkgInfo
                              PROPERTIES MACOSX_PACKAGE_LOCATION "")
  target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/PkgInfo)

  install(DIRECTORY "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.plugin"
          DESTINATION "${PLUGIN_FOLDER}/")

  if(NOT CMAKE_HOST_APPLE)
    # create debug symbols
    add_custom_command(
      TARGET ${PROJECT_NAME}
      POST_BUILD USES_TERMINAL
      COMMAND
        dsymutil
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.plugin/Contents/MacOS/${PROJECT_NAME}
        -o ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.dSYM)

    # create lldb start-up script
    set(LLDB_REMOTE_NAME Deniss-MacBook-Air.local)
    set(LLDB_REMOTE_PORT 16000)
    file(
      WRITE ${CMAKE_CURRENT_BINARY_DIR}/lldb-remote-start-up.txt
      "platform select remote-macosx\n"
      "platform connect connect://${LLDB_REMOTE_NAME}:${LLDB_REMOTE_PORT}\n"
      "setting set target.memory-module-load-level partial\n"
      "attach -n \"After Effects\"\n"
      "target symbols add ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.dSYM/Contents/Resources/DWARF/${PROJECT_NAME}"
    )
  endif()
endif()

if(REMOTE_MAC_BUILD)
  execute_process(
    COMMAND zsh "-c"
            "${CMAKE_CURRENT_SOURCE_DIR}/sh/symlink-macos-frameworks.sh")
endif()
