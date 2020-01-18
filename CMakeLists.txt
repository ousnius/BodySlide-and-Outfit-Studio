cmake_minimum_required(VERSION 3.10)

project(BSOS)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(wxWidgets REQUIRED gl core base net xrc adv qa html propgrid)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
set(fbxsdk_dir ../fbxsdk)
find_library(fbxsdk fbxsdk PATHS ${fbxsdk_dir}/lib/gcc/x64/release)

set(commonsources
	lib/FSEngine/FSBSA.cpp
	lib/FSEngine/FSEngine.cpp
	lib/FSEngine/FSManager.cpp
	lib/gli/glm/detail/glm.cpp
	lib/LZ4F/lz4.c
	lib/LZ4F/lz4frame.c
	lib/LZ4F/lz4hc.c
	lib/LZ4F/xxhash.c
	lib/NIF/Animation.cpp
	lib/NIF/BasicTypes.cpp
	lib/NIF/bhk.cpp
	lib/NIF/ExtraData.cpp
	lib/NIF/Factory.cpp
	lib/NIF/Geometry.cpp
	lib/NIF/NifFile.cpp
	lib/NIF/Nodes.cpp
	lib/NIF/Objects.cpp
	lib/NIF/Particles.cpp
	lib/NIF/Shaders.cpp
	lib/NIF/Skin.cpp
	lib/NIF/utils/Object3d.cpp
	lib/SOIL2/etc1_utils.c
	lib/SOIL2/image_DXT.c
	lib/SOIL2/image_helper.c
	lib/SOIL2/SOIL2.c
	lib/TinyXML-2/tinyxml2.cpp
	src/components/Anim.cpp
	src/components/Automorph.cpp
	src/components/DiffData.cpp
	src/components/Mesh.cpp
	src/components/NormalGenLayers.cpp
	src/components/SliderCategories.cpp
	src/components/SliderData.cpp
	src/components/SliderGroup.cpp
	src/components/SliderManager.cpp
	src/components/SliderPresets.cpp
	src/components/SliderSet.cpp
	src/files/MaterialFile.cpp
	src/files/ObjFile.cpp
	src/files/ResourceLoader.cpp
	src/files/TriFile.cpp
	src/program/GroupManager.cpp
	src/program/PresetSaveDialog.cpp
	src/render/GLExtensions.cpp
	src/render/GLMaterial.cpp
	src/render/GLOffscreenBuffer.cpp
	src/render/GLShader.cpp
	src/render/GLSurface.cpp
	src/ui/wxStateButton.cpp
	src/utils/AABBTree.cpp
	src/utils/ConfigurationManager.cpp
	src/utils/Log.cpp
	src/utils/PlatformUtil.cpp
	src/utils/StringStuff.cpp
	)
set(OSsources
	${commonsources}
	src/components/RefTemplates.cpp
	src/components/TweakBrush.cpp
	src/files/FBXWrangler.cpp
	src/program/EditUV.cpp
	src/program/FBXImportDialog.cpp
	src/program/OutfitProject.cpp
	src/program/OutfitStudio.cpp
	src/program/ShapeProperties.cpp
	)
set(BSsources
	${commonsources}
	src/files/wxDDSImage.cpp
	src/program/BodySlideApp.cpp
	src/program/NormalsGenDialog.cpp
	src/program/PreviewWindow.cpp
	src/ui/wxNormalsGenDlg.cpp
	)

add_executable(OutfitStudio ${OSsources})
add_executable(BodySlide ${BSsources})

include(${wxWidgets_USE_FILE})
target_include_directories(OutfitStudio SYSTEM PRIVATE
	${fbxsdk_dir}/include
	/usr/include/wine/windows
	)
target_include_directories(BodySlide SYSTEM PRIVATE
	/usr/include/wine/windows
	)
target_include_directories(OutfitStudio PUBLIC lib/gli)
target_include_directories(BodySlide PUBLIC lib/gli)

target_link_libraries(OutfitStudio
	${wxWidgets_LIBRARIES}
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
	${fbxsdk}
	xml2)

target_link_libraries(BodySlide
	${wxWidgets_LIBRARIES}
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
	xml2)
