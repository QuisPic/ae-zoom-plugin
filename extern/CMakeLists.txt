include(FetchContent)

add_subdirectory("Adobe After Effects SDK")

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

# Fix uppercase A in "Advapi32" included lib from the uiohook cmake file, should be "advapi32" for compilation on linux
get_target_property(UIOHOOK_LINKED_LIBS uiohook INTERFACE_LINK_LIBRARIES)
list(REMOVE_ITEM UIOHOOK_LINKED_LIBS Advapi32)
set_property(TARGET uiohook PROPERTY INTERFACE_LINK_LIBRARIES ${UIOHOOK_LINKED_LIBS})
target_link_libraries(uiohook advapi32)
