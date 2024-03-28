if(WIN32 OR WIN64)
  set(IOHOOK_SOURCE_DIR "windows")
elseif(APPLE)
  set(IOHOOK_SOURCE_DIR "darwin")
endif()

target_sources(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/${IOHOOK_SOURCE_DIR}/iohook.cpp")
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
