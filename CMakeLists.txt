# CMakeList.txt : CMake project for cmake-test, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("ae-zoom")

add_subdirectory(extern)

add_library(
	${PROJECT_NAME}
	MODULE
	src/ae-zoom.cpp
)

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
		"/Applications/Adobe After Effects 2020/Plug-ins/"
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
	${PROJECT_NAME}
	AESDK
	uiohook
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

install(
	TARGETS ${PROJECT_NAME}
	DESTINATION ${PLUGIN_FOLDER}
)

if( WIN32 )
	set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".aex")
elseif( APPLE )
	set_target_properties( ${PROJECT_NAME} PROPERTIES PREFIX "" )
	set_target_properties( ${PROJECT_NAME} PROPERTIES SUFFIX "" )

	set_target_properties( ${PROJECT_NAME} PROPERTIES BUNDLE ON )
	set_target_properties( ${PROJECT_NAME} PROPERTIES BUNDLE_EXTENSION "plugin" )

	#PkgInfo
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/PkgInfo
		COMMAND echo "eFKTFXTC" >> ${CMAKE_CURRENT_BINARY_DIR}/PkgInfo
	)
	set_source_files_properties(
		${CMAKE_CURRENT_BINARY_DIR}/PkgInfo
		PROPERTIES
		MACOSX_PACKAGE_LOCATION ""
	)
	target_sources( ${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/PkgInfo )
endif()

# TODO: Add tests and install targets if needed.
