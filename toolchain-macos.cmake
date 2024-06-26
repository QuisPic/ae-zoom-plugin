set(CMAKE_SYSTEM_NAME Darwin)
# set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_OSX_ARCHITECTURES
    "x86_64;arm64"
    CACHE STRING "" FORCE)
set(TOOLCHAIN_PREFIX o64)
# set(TOOLCHAIN_PREFIX x86_64-apple-darwin23)

# cross compilers to use for C, C++
set(CMAKE_AR /usr/local/osxcross/bin/arm64-apple-darwin23-ar)
set(CMAKE_RANLIB /usr/local/osxcross/bin/arm64-apple-darwin23-ranlib)
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-clang)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-clang++)
add_link_options("-fuse-ld=lld")

# Clang target triple
set(TARGET x86_64-apple-darwin)

# specify the cross compiler target
set(CMAKE_C_COMPILER_TARGET ${TARGET})
set(CMAKE_CXX_COMPILER_TARGET ${TARGET})

# target environment on the build host system
set(CMAKE_FIND_ROOT_PATH /usr/local/osxcross/SDK/MacOSX14.sdk)
set(CMAKE_SYSROOT /usr/local/osxcross/SDK/MacOSX14.sdk)
set(CMAKE_OSX_SYSROOT /usr/local/osxcross/SDK/MacOSX14.sdk)

# modify default behavior of FIND_XXX() commands
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
