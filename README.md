# Zoom Plug-in for After Effects
After Effects plug-in that comes with [Zoom](https://github.com/QuisPic/ae-zoom) script for Ae. The plug-in adds support for custom key bindings that can change zoom value of the active viewer.

## Please note
This plugin is intended to be used together with [Zoom](https://github.com/QuisPic/ae-zoom) script. You can add key bindings only using the script. The plugin will only read existing key bindings added by the script.

## Dependencies
* [Cmake 3.8.0+](https://www.cmake.org/download/)
## Building
Clone the repository:
`git clone https://github.com/QuisPic/ae-zoom-plugin.git`

### Windows
#### CMake
Open the repository folder in terminal and run:
```
cmake -S . -B build
cmake --build build --config Debug
```
To build release version run:
```
cmake -S . -B build
cmake --build build --config Release
```
After that the plug-in can be found in `./build/Debug/Zoom.aex` or `./build/Release/Zoom.aex`
#### Visual Studio
Open `CMakeLists.txt` using Visual Studio's [built-in support for opening CMake projects](https://blogs.msdn.microsoft.com/vcblog/2016/10/05/cmake-support-in-visual-studio/) and build.

### macOS
#### CMake
Open the repository folder in terminal and run:
```
cmake -S . -B build -D CMAKE_BUILD_TYPE=Debug
cmake --build build
```
To build release version run:
```
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build
```
After that the plug-in can be found in `./build/Zoom.plugin`
#### XCode
Generate XCode project via Terminal using CMake
```
mkdir build
cd build
cmake -GXcode ..
```

## Installation
Copy the plug-in to:

Windows: `C:\Program Files\Adobe\Adobe After Effects [version]\Support Files\Plug-ins\`

macOS: `/Applications/Adobe After Effects (version)/Plug-ins/`
