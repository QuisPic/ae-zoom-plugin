set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

# Clang target triple
set(TARGET x86_64-pc-windows-gnu)

# cross compilers to use for C, C++ and Fortran
set(CMAKE_C_COMPILER /usr/llvm-mingw/bin/${TOOLCHAIN_PREFIX}-clang)
set(CMAKE_CXX_COMPILER /usr/llvm-mingw/bin/${TOOLCHAIN_PREFIX}-clang++)
set(CMAKE_RC_COMPILER /usr/llvm-mingw/bin/${TOOLCHAIN_PREFIX}-windres)
add_link_options("-fuse-ld=/usr/llvm-mingw/bin/${TOOLCHAIN_PREFIX}-ld")
# set(CMAKE_RC_COMPILER /usr/llvm-mingw/bin/llvm-rc) set(CMAKE_RC_COMPILER
# ${TOOLCHAIN_PREFIX}-windres)

# specify the cross compiler target
set(CMAKE_C_COMPILER_TARGET ${TARGET})
set(CMAKE_CXX_COMPILER_TARGET ${TARGET})

# target environment on the build host system
set(CMAKE_FIND_ROOT_PATH /usr/llvm-mingw/${TOOLCHAIN_PREFIX})

# modify default behavior of FIND_XXX() commands
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
