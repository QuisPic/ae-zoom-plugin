# libUIOHook: Cross-platform keyboard and mouse hooking from userland.
# Copyright (C) 2006-2023 Alexander Barker.  All Rights Reserved.
# https://github.com/kwhat/libuiohook/
#
# libUIOHook is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# libUIOHook is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.10)

project(uiohook VERSION 1.2.0 LANGUAGES C)


if (WIN32 OR WIN64)
    set(UIOHOOK_SOURCE_DIR "windows")
elseif (APPLE)
    set(UIOHOOK_SOURCE_DIR "darwin")
endif()

add_library(uiohook
    "src/logger.c"
    "src/${UIOHOOK_SOURCE_DIR}/dispatch_event.c"
    "src/${UIOHOOK_SOURCE_DIR}/input_helper.c"
    "src/${UIOHOOK_SOURCE_DIR}/input_hook.c"
    "src/${UIOHOOK_SOURCE_DIR}/post_event.c"
    "src/${UIOHOOK_SOURCE_DIR}/system_properties.c"
)

set_target_properties(uiohook PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE 1
    OUTPUT_NAME "${PROJECT_NAME}"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/uiohook.h
)

include(GNUInstallDirs)
target_include_directories(uiohook
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>

    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/${UIOHOOK_SOURCE_DIR}
)


install(TARGETS uiohook
    EXPORT ${PROJECT_NAME}-config
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

export(TARGETS uiohook FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake")
install(EXPORT ${PROJECT_NAME}-config DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})


option(USE_EPOCH_TIME "Use Unix epoch time for event timestamps (default: ON)" ON)
if(USE_EPOCH_TIME)
    add_compile_definitions(uiohook PRIVATE USE_EPOCH_TIME)
endif()

if(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.5")

    find_package(Threads REQUIRED)
    target_link_libraries(uiohook "${CMAKE_THREAD_LIBS_INIT}")

    find_library(CARBON Carbon REQUIRED)
    target_include_directories(uiohook PRIVATE "${CARBON}")
    target_link_libraries(uiohook "${CARBON}")

    option(USE_APPLICATION_SERVICES "ApplicationServices framework (default: ON)" ON)
    if(USE_APPLICATION_SERVICES)
        find_library(APPLICATION_SERVICES ApplicationServices REQUIRED)
        add_compile_definitions(USE_APPLICATION_SERVICES)
        target_include_directories(uiohook PRIVATE "${APPLICATION_SERVICES}")
        target_link_libraries(uiohook "${APPLICATION_SERVICES}")
    endif()

    option(USE_IOKIT "IOKit framework (default: ON)" ON)
    if(USE_IOKIT)
        find_library(IOKIT IOKit REQUIRED)
        add_compile_definitions(USE_IOKIT)
        target_include_directories(uiohook PRIVATE "${IOKIT}")
        target_link_libraries(uiohook "${IOKIT}")
    endif()

    option(USE_APPKIT "AppKit framework (default: ON)" ON)
    if(USE_APPKIT)
        find_library(APPKIT AppKit REQUIRED)
        add_compile_definitions(USE_APPKIT)
        target_include_directories(uiohook PRIVATE "${APPKIT}")
        target_link_libraries(uiohook "${APPKIT}")
    endif()
elseif(WIN32)
    target_link_libraries(uiohook advapi32)
endif()


list(REMOVE_DUPLICATES INTERFACE_LINK_LIBRARIES)
string(REPLACE ";" " " COMPILE_LIBRARIES "${INTERFACE_LINK_LIBRARIES}")
configure_file("pc/uiohook.pc.in" "${PROJECT_BINARY_DIR}/uiohook.pc" @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/uiohook.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
