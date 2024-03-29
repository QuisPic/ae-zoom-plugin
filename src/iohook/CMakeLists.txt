if(WIN32 OR WIN64)
  set(IOHOOK_SOURCE_DIR "windows")
  set(IOHOOK_EXTENSION "cpp")
elseif(APPLE)
  set(IOHOOK_SOURCE_DIR "darwin")
  set(IOHOOK_EXTENSION "mm")
endif()

target_sources(
  ${PROJECT_NAME}
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/logger.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/iohook.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/${IOHOOK_SOURCE_DIR}/input-helper.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/${IOHOOK_SOURCE_DIR}/dispatch-event.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/logger.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/${IOHOOK_SOURCE_DIR}/iohook.${IOHOOK_EXTENSION}"
    "${CMAKE_CURRENT_SOURCE_DIR}/${IOHOOK_SOURCE_DIR}/input-helper.${IOHOOK_EXTENSION}"
    "${CMAKE_CURRENT_SOURCE_DIR}/${IOHOOK_SOURCE_DIR}/dispatch-event.${IOHOOK_EXTENSION}"
)

target_include_directories(
  ${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                          ${CMAKE_CURRENT_SOURCE_DIR}/${IOHOOK_SOURCE_DIR})

if(APPLE)
  set(CMAKE_MACOSX_RPATH 1)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.5")

  find_package(Threads REQUIRED)
  target_link_libraries(${PROJECT_NAME} PRIVATE "${CMAKE_THREAD_LIBS_INIT}")

  find_library(CARBON Carbon REQUIRED)
  target_include_directories(${PROJECT_NAME} PRIVATE "${CARBON}")
  target_link_libraries(${PROJECT_NAME} PRIVATE "${CARBON}")

  option(USE_APPLICATION_SERVICES "ApplicationServices framework (default: ON)"
         ON)
  if(USE_APPLICATION_SERVICES)
    find_library(APPLICATION_SERVICES ApplicationServices REQUIRED)
    add_compile_definitions(USE_APPLICATION_SERVICES)
    target_include_directories(${PROJECT_NAME}
                               PRIVATE "${APPLICATION_SERVICES}")
    target_link_libraries(${PROJECT_NAME} PRIVATE "${APPLICATION_SERVICES}")
  endif()

  option(USE_IOKIT "IOKit framework (default: ON)" ON)
  if(USE_IOKIT)
    find_library(IOKIT IOKit REQUIRED)
    add_compile_definitions(USE_IOKIT)
    target_include_directories(${PROJECT_NAME} PRIVATE "${IOKIT}")
    target_link_libraries(${PROJECT_NAME} PRIVATE "${IOKIT}")
  endif()

  option(USE_APPKIT "AppKit framework (default: ON)" ON)
  if(USE_APPKIT)
    find_library(APPKIT AppKit REQUIRED)
    add_compile_definitions(USE_APPKIT)
    target_include_directories(${PROJECT_NAME} PRIVATE "${APPKIT}")
    target_link_libraries(${PROJECT_NAME} PRIVATE "${APPKIT}")
  endif()
elseif(WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE advapi32)
endif()

list(REMOVE_DUPLICATES INTERFACE_LINK_LIBRARIES)
string(REPLACE ";" " " COMPILE_LIBRARIES "${INTERFACE_LINK_LIBRARIES}")
