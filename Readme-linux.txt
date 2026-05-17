Building BodySlide and Outfit Studio on Linux

There's just one file that specifies the build process: CMakeLists.txt.
CMake 3.16 or newer is required.

Install wxWidgets 3.1.3 or newer.  If you get errors about an ABI
mismatch, that means you compiled wxWidgets with a different compiler
version than BS&OS.  Either gtk2 or gtk3 works.  With gtk2, you have
more background color problems; with gtk3, many widgets are distorted
because they don't have enough space.  Note that many of wxWidget's
configure options (such as --enable-universal) result in a broken
wxWidgets library, so prefer to use as few options as possible.
Also note that some distribution-provided wxWidgets packages are
broken, so it's likely that you'll have to build wxWidgets yourself
(which is really easy).

The CMake build first looks for an installed wxWidgets CMake package and
falls back to CMake's FindwxWidgets module, which uses wx-config on Linux.
If you build wxWidgets with CMake, install it to a local prefix and pass
that prefix to this project:

mkdir wx-build
cd wx-build
cmake -S /path/to/wxWidgets -B . -DCMAKE_BUILD_TYPE=Release -DwxBUILD_TOOLKIT=gtk3 -DCMAKE_INSTALL_PREFIX=$HOME/opt/wxwidgets-gtk3
cmake --build . --parallel
cmake --install .

Then configure BS&OS with:

cmake -S . -B Release -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$HOME/opt/wxwidgets-gtk3

If CMake still does not find the package, point it directly at the package
directory, usually one of these:

cmake -S . -B Release -DCMAKE_BUILD_TYPE=Release -DwxWidgets_DIR=$HOME/opt/wxwidgets-gtk3/lib/cmake/wxWidgets
cmake -S . -B Release -DCMAKE_BUILD_TYPE=Release -DwxWidgets_ROOT=$HOME/opt/wxwidgets-gtk3

For a wxWidgets build made with configure/make instead of CMake, use the
fallback module by passing the wx-config path:

cmake -S . -B Release -DCMAKE_BUILD_TYPE=Release -DwxWidgets_CONFIG_EXECUTABLE=$HOME/opt/wxwidgets-gtk3/bin/wx-config

FBX SDK support is optional.  By default, CMake enables FBX support when it
can find the SDK and builds Outfit Studio without FBX import/export when it
cannot.  To disable FBX support explicitly:

cmake -S . -B Release -DCMAKE_BUILD_TYPE=Release -DBSOS_ENABLE_FBXSDK=OFF

To enable FBX support from a non-standard location, pass the SDK root with
BSOS_FBXSDK_ROOT, the legacy fbxsdk_dir cache variable, or the FBXSDK_ROOT
environment variable:

cmake -S . -B Release -DCMAKE_BUILD_TYPE=Release -DBSOS_FBXSDK_ROOT=/opt/fbxsdk

Make sure GLEW is installed.

Then go to your BodySlide-and-Outfit-Studio directory and do:
mkdir Release
cd Release
cmake -DCMAKE_BUILD_TYPE=Release ..
make

The possible values for CMAKE_BUILD_TYPE:
	Release
	RelWithDebInfo
	Debug
	MinSizeRel
	(nothing)

To specify the compiler, set CC and CXX before running cmake.
The build directory must be completely empty, or cmake will ignore CC
and CXX and use the same compilers as it did last time.  Don't forget
to build wxWidgets with the same compiler; the wxWidgets configure script
also uses the CC and CXX environment variables.

Some useful make options:
make VERBOSE=1
make -j 4

The BodySlide and OutfitStudio executables need to be able to find
the res directory, which contains important program data such as xrc
files and icons.  By default, the executables search for the res
directory in the same directory as the executables.  So, when you
copy the executables into the BodySlide directory within your game,
you need to copy the entire res directory too.

If you don't want to copy the executables to the BodySlide
directory within your game, set WX_BODYSLIDE_DATA_DIR and
WX_OUTFITSTUDIO_DATA_DIR to the BodySlide directory.  The res directory
must still be copied into the BodySlide directory.
