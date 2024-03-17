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

if(REMOTE_MAC_BUILD)
  execute_process(
    COMMAND zsh "-c"
            "${CMAKE_CURRENT_SOURCE_DIR}/sh/symlink-macos-frameworks.sh")
endif()
