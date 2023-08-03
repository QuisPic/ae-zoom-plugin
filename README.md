# Zoom Plug-in for After Effects
Source code for the After Effects plug-in that comes with [Zoom](https://github.com/QuisPic/ae-zoom) script for Ae. The plug-in adds support for custom key bindings that can change zoom value of the active viewer inside After Effects.
## Please note
This plug-in is supposed to be used together with [Zoom](https://github.com/QuisPic/ae-zoom) script. You can add key bindings only using the script. The plug-in only reads existing key bindings that were added through the script.
## Dependencies
* [Cmake 3.8.0+](https://www.cmake.org/download/)
## Building
Clone the repository:
`git clone git@github.com:QuisPic/ae-zoom-plugin.git`
#### CMake
Open the repository folder in terminal and run:
```
cmake -S . -B build
cmake --build build
```
To build a release version replace the last command with:
```
cmake --build build --config Release
```
After that the plugin can be found in `./build/Debug/Zoom.aex` or `./build/Release/Zoom.aex`
#### Visual Studio on Windows
Open `CMakeLists.txt` using Visual Studio's [built-in support for opening CMake projects](https://blogs.msdn.microsoft.com/vcblog/2016/10/05/cmake-support-in-visual-studio/) and build.
#### Xcode
Generate XCode project via Terminal using CMake
```
mkdir build
cd build
cmake -GXcode ..
```
