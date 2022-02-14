Building BodySlide and Outfit Studio on Linux

There's just one file that specifies the build process: CMakeLists.txt.

Install wxWidgets 3.1.3 or newer.  Use the "--enable-stl" option to
configure.  If you get errors about an ABI mismatch, that means you
compiled wxWidgets with a different compiler version than BS&OS (and
wxWidgets is strangely picky about that).  Either gtk2 or gtk3 works.
With gtk2, you have more background color problems; with gtk3, many
widgets are distorted because they don't have enough space.  Note that
many of wxWidget's configure options (such as --enable-universal)
result in a broken wxWidgets library, so prefer to use as few options
as possible.

Install FBX SDK.  Put the path to your FBX SDK installation in the
fbxsdk_dir variable in CMakeLists.txt.

Make sure GLEW is installed.

Then go to your BodySlide-and-Outfit-Studio directory and do:
mkdir Release
cd Release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall" ..
make

The possible values for CMAKE_BUILD_TYPE:
	Release
	RelWithDebInfo
	Debug
	MinSizeRel
	(nothing)

To specify the compiler, set CC and CXX before running cmake.  The build
directory must be completely empty, or cmake will ignore CC and CXX and
use the same compilers as it did last time.

Some useful make options:
make VERBOSE=1
make -j 4

If you don't want to copy the executables to the BodySlide
directory within your game, set WX_BODYSLIDE_DATA_DIR and
WX_OUTFITSTUDIO_DATA_DIR to the BodySlide directory.  Unfortunately,
program data such as xrc files and icons also come from the data dir;
if you make changes to the data files, you'll have to copy them over.
