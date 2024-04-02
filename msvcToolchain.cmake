# Download FindVcvars.cmake
set(dest_file "${CMAKE_CURRENT_SOURCE_DIR}/FindVcvars.cmake")
set(expected_hash "0fd1104a5e0f91f89b64dd700ce57ab9f7d7356fcabe9c895e62aca32386008e")
set(url "https://raw.githubusercontent.com/scikit-build/cmake-FindVcvars/v1.6/FindVcvars.cmake")
if(NOT EXISTS ${dest_file})
  file(DOWNLOAD ${url} ${dest_file} EXPECTED_HASH SHA256=${expected_hash})
else()
  file(SHA256 ${dest_file} current_hash)
  if(NOT ${current_hash} STREQUAL ${expected_hash})
    file(DOWNLOAD ${url} ${dest_file} EXPECTED_HASH SHA256=${expected_hash})
  endif()
endif()

# allow findXXX resolution in CMake/
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Vcvars)

if (NOT ${Vcvars_FOUND})
	message(FATAL_ERROR "[msvcToolchain.cmake] Looks like no Visual Studio installed")
endif()

# got vcvars launcher !
set(cmd_wrapper ${Vcvars_LAUNCHER})

# DEBUG : dump env after vcvars*.bat call
#execute_process(COMMAND ${cmd_wrapper} set OUTPUT_FILE ${CMAKE_CURRENT_LIST_DIR}/debug/out.env.before.txt OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND ${cmd_wrapper} set OUTPUT_VARIABLE env_dump OUTPUT_STRIP_TRAILING_WHITESPACE)

#1. escaping troublesome chars 
string(REPLACE ";" "__semicolon__" env_dump "${env_dump}")
string(REPLACE "\\" "__backslash__" env_dump "${env_dump}")
string(REPLACE "\"" "__doublequote__" env_dump "${env_dump}")

#2. multi-line => one line
string(REGEX REPLACE "[\r\n]+" ";" env_dump "${env_dump}")

#3. keep only lines looking like xx=yy
list(FILTER env_dump INCLUDE REGEX ".+\=.+")

#4. setting captured env var right here
foreach(key_value ${env_dump})
	string(REPLACE "=" ";" key_value_as_list ${key_value})
	list(GET key_value_as_list 0 key)
	list(GET key_value_as_list 1 value)
	
	string(REPLACE "__semicolon__" "\;" key "${key}")
	string(REPLACE "__backslash__" "\\" key "${key}")
	string(REPLACE "__doublequote__" "\"" key "${key}")

	string(REPLACE "__semicolon__" ";" value "${value}")
	string(REPLACE "__backslash__" "\\" value "${value}")
	string(REPLACE "__doublequote__" "\"" value "${value}")
	
	set(ENV{${key}} "${value}")
endforeach()

#5. adjust cmake vars to find msvc headers & resolve libs path
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES $ENV{INCLUDE})
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES $ENV{INCLUDE})
link_directories($ENV{LIB})

message(STATUS "[msvcToolchain.cmake] env dumping done")

# DEBUG : dump en var after all this
# execute_process(COMMAND cmd /C set OUTPUT_FILE ${CMAKE_CURRENT_LIST_DIR}/debug/out.env.after.txt OUTPUT_STRIP_TRAILING_WHITESPACE)
