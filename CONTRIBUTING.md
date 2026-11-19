Request new features or report bugs by opening an issue on GitHub or commenting on the NexusMods mod pages.

To submit a patch, please open a pull request on GitHub. If you are thinking of making a large contribution, open an issue for it before starting work.

Please note the license of the project before committing your work.

To make it easier to review and accept your pull request, please follow these
guidelines:

1. Make a new branch on your fork and use that to open a pull request of your changes.
2. No trailing whitespaces.
3. Save your indentation as spaces. Tab size is 4 spaces.
4. Use the following format for curly brackets to be consistent with the rest:
```cpp
if (true) {
  doThing(1);
  doThing(2);
}
```
5. Include headers from `src`, like `#include "components/Mesh.h"`. Headers in the same folder are included by their name only.

## Source layout

Each folder may only use the folders above it in this list. CMake builds each one as a static library (`bsos_*`), so a wrong dependency fails to link on Linux.

| Folder | What it holds |
|---|---|
| `lib` | Third-party code (nifly, FSEngine, TinyXML-2, ...) |
| `src/utils`, `src/files`, `src/components` | File formats, meshes, sliders, skeletons and brushes. No GL and no windows. |
| `src/render` | OpenGL drawing |
| `src/physics` | The physics preview, drawn through `src/render` |
| `src/ui` | Dialogs and widgets that both applications use |
| `src/project` | The Outfit Studio project and its operations, without the window. The tests use it. |
| `src/mcp` | The MCP server of Outfit Studio and its tools |
| `src/ui/outfitstudio`, `src/ui/bodyslide` | Dialogs and widgets of one application |
| `src/outfitstudio`, `src/bodyslide` | The applications and their windows |

Large classes are split into one file per feature, named after the class. For example `OutfitStudioFrame.cpp` and `FrameSliders.cpp` both implement `OutfitStudioFrame`, and `OutfitProject.cpp` and `OutfitProjectSave.cpp` both implement `OutfitProject`.

## Adding or moving a source file

The file lists of CMake and of the Visual Studio projects are kept by hand. A new or moved file goes into each list that builds it:

- `CMakeLists.txt`: the library or application source list of its folder, and the `BSOSTests` list for a test
- `OutfitStudio.vcxproj` and `BodySlide.vcxproj` for the applications that use it, and `BSOSTests.vcxproj` if the tests use it
- the matching `.vcxproj.filters` files, so the file shows up in its folder in Visual Studio

Source files use Windows (CRLF) line endings.
