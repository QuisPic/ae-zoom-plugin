# CMakeList.txt : CMake project for cmake-test, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
set( CMAKE_CXX_STANDARD 20 )
include(FetchContent)

if( APPLE )
	# Create Universal Binary
	set( CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)

	# Disable warnings
	add_compile_options(
		-Wno-pragma-pack
	)
endif()

project ("Zoom")

# Fetch JSON library
FetchContent_Declare(
	json 
	URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/extern/json)

# Fetch uiohook library
if ( WIN32 )
FetchContent_Declare(
	uiohook 
	GIT_REPOSITORY https://github.com/kwhat/libuiohook.git
	GIT_TAG 1.3
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/extern/uiohook-win
)
elseif ( APPLE )
FetchContent_Declare(
	uiohook 
	GIT_REPOSITORY https://github.com/kwhat/libuiohook.git
	GIT_TAG 1.3
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/extern/uiohook-mac
)
endif()

FetchContent_MakeAvailable(json uiohook)

add_subdirectory(extern)

add_library(
	${PROJECT_NAME}
	MODULE
	"src/ae-zoom.cpp" "src/ae-egg.cpp" "src/mangled-names/mangled-names.h" "src/mangled-names/mangled-names-win.cpp" "src/mangled-names/mangled-names-mac.cpp")

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
	include
)

if( WIN32 )
	set(
		PLUGIN_FOLDER
		"C:/Program Files/Adobe/Adobe After Effects 2023/Support Files/Plug-ins"
		CACHE
		STRING
		"After Effects Plugin folder"
	)

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rr
		COMMAND
		cl /I \"${AESDK_ROOT}/Headers\" /EP \"${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.r\" > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rr
	)

	add_custom_command(
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rr
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rrc
		COMMAND ${AFX_REZ} ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rr ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rrc
	)

	add_custom_command(
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rrc
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc
		COMMAND cl /D "MSWindows" /EP ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rrc > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc
	)
	target_sources(
		${PROJECT_NAME} PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc
	)
elseif( APPLE )
	set(
		PLUGIN_FOLDER
		"/Applications/Adobe After Effects 2023/Plug-ins"
		CACHE
		STRING
		"After Effects Plugin folder"
	)
	set( ${PIPL_INCLUDES} "" )

	foreach( INC ${AESDK_INCLUDE} )
		list( APPEND PIPL_INCLUDES -i ${INC} )
	endforeach()

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rsrc
		COMMAND ${AFX_REZ}
		ARGS ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.r
			-o ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rsrc
			-useDF
			${PIPL_INCLUDES}
			-D __MACH__
	)
	set_source_files_properties(
		${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rsrc
		PROPERTIES
		MACOSX_PACKAGE_LOCATION Resources/
	)
	target_sources( ${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rsrc )
endif()

target_link_libraries(
	${PROJECT_NAME} PRIVATE
	AESDK
	uiohook
	nlohmann_json::nlohmann_json
)

if( WIN32 )
	set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".aex")

	install(
		FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.aex"
		DESTINATION "${PLUGIN_FOLDER}/"
	)
elseif( APPLE )
	set_target_properties( ${PROJECT_NAME} PROPERTIES PREFIX "" )
	set_target_properties( ${PROJECT_NAME} PROPERTIES SUFFIX "" )

	set_target_properties( ${PROJECT_NAME} PROPERTIES BUNDLE ON )
	set_target_properties( ${PROJECT_NAME} PROPERTIES BUNDLE_EXTENSION "plugin" )

	#PkgInfo
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/PkgInfo
		COMMAND echo "AEgxFXTC" > ${CMAKE_CURRENT_BINARY_DIR}/PkgInfo
	)
	set_source_files_properties(
		${CMAKE_CURRENT_BINARY_DIR}/PkgInfo
		PROPERTIES
		MACOSX_PACKAGE_LOCATION ""
	)
	target_sources( ${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/PkgInfo )

	install(
		DIRECTORY "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.plugin"
		DESTINATION "${PLUGIN_FOLDER}/"
	)
endif()

if ( REMOTE_MAC_BUILD )
	execute_process(COMMAND zsh "-c" "${CMAKE_CURRENT_SOURCE_DIR}/sh/symlink-macos-frameworks.sh")
endif()
