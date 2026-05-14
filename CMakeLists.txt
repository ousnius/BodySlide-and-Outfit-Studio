cmake_minimum_required(VERSION 3.16)

project(BSOS LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(BSOS_ENABLE_FBXSDK "Build Outfit Studio with Autodesk FBX SDK support when available" ON)
set(BSOS_FBXSDK_ROOT "" CACHE PATH "Root directory of the Autodesk FBX SDK")
set(fbxsdk_dir "" CACHE PATH "Compatibility alias for BSOS_FBXSDK_ROOT")

if(WIN32 AND DEFINED wxWidgets_ROOT_DIR AND wxWidgets_ROOT_DIR
		AND (NOT DEFINED wxWidgets_LIB_DIR OR wxWidgets_LIB_DIR STREQUAL "" OR wxWidgets_LIB_DIR MATCHES "-NOTFOUND$"))
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(wxWidgets_LIB_DIR "${wxWidgets_ROOT_DIR}/lib/vc_x64_lib" CACHE PATH "wxWidgets library directory" FORCE)
	else()
		set(wxWidgets_LIB_DIR "${wxWidgets_ROOT_DIR}/lib/vc_lib" CACHE PATH "wxWidgets library directory" FORCE)
	endif()
endif()

set(BSOS_WX_COMPONENTS gl core base net xml xrc adv qa html propgrid)
find_package(wxWidgets CONFIG QUIET COMPONENTS ${BSOS_WX_COMPONENTS})
if(wxWidgets_FOUND AND TARGET wx::base)
	set(BSOS_WX_LIBRARIES
		wx::gl
		wx::core
		wx::base
		wx::net
		wx::xml
		wx::xrc
		wx::adv
		wx::qa
		wx::html
		wx::propgrid
	)
else()
	find_package(wxWidgets REQUIRED COMPONENTS ${BSOS_WX_COMPONENTS})
	include(${wxWidgets_USE_FILE})
	set(BSOS_WX_LIBRARIES ${wxWidgets_LIBRARIES})
endif()

find_package(OpenGL REQUIRED)
if(UNIX AND NOT APPLE)
	find_package(GLEW REQUIRED)
endif()

set(BSOS_OPENGL_LIBRARIES)
if(TARGET OpenGL::GL)
	list(APPEND BSOS_OPENGL_LIBRARIES OpenGL::GL)
else()
	list(APPEND BSOS_OPENGL_LIBRARIES ${OPENGL_LIBRARIES})
endif()
if(TARGET OpenGL::GLU)
	list(APPEND BSOS_OPENGL_LIBRARIES OpenGL::GLU)
elseif(OPENGL_glu_LIBRARY)
	list(APPEND BSOS_OPENGL_LIBRARIES ${OPENGL_glu_LIBRARY})
endif()

set(BSOS_GLEW_LIBRARY)
if(UNIX AND NOT APPLE)
	if(TARGET GLEW::GLEW)
		set(BSOS_GLEW_LIBRARY GLEW::GLEW)
	else()
		set(BSOS_GLEW_LIBRARY ${GLEW_LIBRARIES})
	endif()
endif()

set(BSOS_FBXSDK_HINTS)
foreach(candidate IN ITEMS
		"${BSOS_FBXSDK_ROOT}"
		"${fbxsdk_dir}"
		"$ENV{FBXSDK_ROOT}"
		"$ENV{FBX_SDK_ROOT}"
		"${CMAKE_CURRENT_LIST_DIR}/../fbxsdk")
	if(candidate)
		list(APPEND BSOS_FBXSDK_HINTS "${candidate}")
	endif()
endforeach()

foreach(parent_dir IN ITEMS
		"${CMAKE_CURRENT_LIST_DIR}/../FBX SDK"
		"$ENV{ProgramFiles}/Autodesk/FBX/FBX SDK"
		"$ENV{ProgramW6432}/Autodesk/FBX/FBX SDK")
	if(parent_dir)
		file(GLOB BSOS_FBXSDK_VERSION_DIRS LIST_DIRECTORIES true "${parent_dir}/*")
		foreach(candidate IN LISTS BSOS_FBXSDK_VERSION_DIRS)
			if(IS_DIRECTORY "${candidate}")
				list(APPEND BSOS_FBXSDK_HINTS "${candidate}")
			endif()
		endforeach()
	endif()
endforeach()
list(REMOVE_DUPLICATES BSOS_FBXSDK_HINTS)

set(BSOS_FBXSDK_LIBRARY_SUFFIXES_RELEASE
	lib/gcc/x64/release
	lib/vs2022/x64/release
	lib/vs2019/x64/release
	lib/vs2017/x64/release
	lib/x64/release
	lib/x64
	lib
)

set(BSOS_FBXSDK_LIBRARY_SUFFIXES_DEBUG
	lib/gcc/x64/debug
	lib/vs2022/x64/debug
	lib/vs2019/x64/debug
	lib/vs2017/x64/debug
	lib/x64/debug
	lib/x64
	lib
)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	list(APPEND BSOS_FBXSDK_LIBRARY_SUFFIXES_RELEASE
		lib/vs2022/x86/release
		lib/vs2019/x86/release
		lib/vs2017/x86/release
		lib/x86/release
		lib/x86
		lib/vc_lib
	)
	list(APPEND BSOS_FBXSDK_LIBRARY_SUFFIXES_DEBUG
		lib/vs2022/x86/debug
		lib/vs2019/x86/debug
		lib/vs2017/x86/debug
		lib/x86/debug
		lib/x86
		lib/vc_lib
	)
endif()

set(BSOS_HAVE_FBXSDK OFF)
if(BSOS_ENABLE_FBXSDK AND CMAKE_SIZEOF_VOID_P EQUAL 8)
	find_path(FBXSDK_INCLUDE_DIR
		NAMES fbxsdk.h
		HINTS ${BSOS_FBXSDK_HINTS}
		PATH_SUFFIXES include
	)
	find_library(FBXSDK_LIBRARY_RELEASE
		NAMES libfbxsdk-mt fbxsdk libfbxsdk
		HINTS ${BSOS_FBXSDK_HINTS}
		PATH_SUFFIXES ${BSOS_FBXSDK_LIBRARY_SUFFIXES_RELEASE}
	)
	find_library(FBXSDK_LIBRARY_DEBUG
		NAMES libfbxsdk-mtd libfbxsdk-mt fbxsdk libfbxsdk
		HINTS ${BSOS_FBXSDK_HINTS}
		PATH_SUFFIXES ${BSOS_FBXSDK_LIBRARY_SUFFIXES_DEBUG}
	)

	if(FBXSDK_INCLUDE_DIR AND (FBXSDK_LIBRARY_RELEASE OR FBXSDK_LIBRARY_DEBUG))
		set(BSOS_HAVE_FBXSDK ON)
		add_library(FBXSDK::FBXSDK UNKNOWN IMPORTED)
		set_target_properties(FBXSDK::FBXSDK PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${FBXSDK_INCLUDE_DIR}"
		)

		if(FBXSDK_LIBRARY_DEBUG)
			set_property(TARGET FBXSDK::FBXSDK APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
			set_target_properties(FBXSDK::FBXSDK PROPERTIES IMPORTED_LOCATION_DEBUG "${FBXSDK_LIBRARY_DEBUG}")
		endif()
		if(FBXSDK_LIBRARY_RELEASE)
			foreach(config IN ITEMS RELEASE RELWITHDEBINFO MINSIZEREL)
				set_property(TARGET FBXSDK::FBXSDK APPEND PROPERTY IMPORTED_CONFIGURATIONS ${config})
				set_target_properties(FBXSDK::FBXSDK PROPERTIES IMPORTED_LOCATION_${config} "${FBXSDK_LIBRARY_RELEASE}")
			endforeach()
		endif()
		if(NOT FBXSDK_LIBRARY_RELEASE)
			set_target_properties(FBXSDK::FBXSDK PROPERTIES IMPORTED_LOCATION "${FBXSDK_LIBRARY_DEBUG}")
		elseif(NOT FBXSDK_LIBRARY_DEBUG)
			set_target_properties(FBXSDK::FBXSDK PROPERTIES IMPORTED_LOCATION "${FBXSDK_LIBRARY_RELEASE}")
		endif()
	endif()
endif()

if(BSOS_HAVE_FBXSDK)
	find_library(FBXSDK_XML2_LIBRARY_RELEASE
		NAMES libxml2-mt xml2 libxml2
		HINTS ${BSOS_FBXSDK_HINTS}
		PATH_SUFFIXES ${BSOS_FBXSDK_LIBRARY_SUFFIXES_RELEASE}
	)
	find_library(FBXSDK_XML2_LIBRARY_DEBUG
		NAMES libxml2-mtd libxml2-mt xml2 libxml2
		HINTS ${BSOS_FBXSDK_HINTS}
		PATH_SUFFIXES ${BSOS_FBXSDK_LIBRARY_SUFFIXES_DEBUG}
	)
	find_library(FBXSDK_ZLIB_LIBRARY_RELEASE
		NAMES zlib-mt zlib z
		HINTS ${BSOS_FBXSDK_HINTS}
		PATH_SUFFIXES ${BSOS_FBXSDK_LIBRARY_SUFFIXES_RELEASE}
	)
	find_library(FBXSDK_ZLIB_LIBRARY_DEBUG
		NAMES zlib-mtd zlib-mt zlib z
		HINTS ${BSOS_FBXSDK_HINTS}
		PATH_SUFFIXES ${BSOS_FBXSDK_LIBRARY_SUFFIXES_DEBUG}
	)
	find_package(LibXml2 QUIET)
	find_package(ZLIB QUIET)
	message(STATUS "FBX SDK found: ${FBXSDK_INCLUDE_DIR}")
elseif(BSOS_ENABLE_FBXSDK AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(STATUS "FBX SDK support is disabled for 32-bit builds")
elseif(BSOS_ENABLE_FBXSDK)
	message(STATUS "FBX SDK not found; Outfit Studio will build without FBX support")
else()
	message(STATUS "FBX SDK support disabled")
endif()

set(BSOS_COMMON_SOURCES
	lib/FSEngine/FSBSA.cpp
	lib/FSEngine/FSEngine.cpp
	lib/FSEngine/FSManager.cpp
	lib/gli/core/dummy.cpp
	lib/gli/glm/detail/glm.cpp
	lib/LZ4F/lz4.c
	lib/LZ4F/lz4file.c
	lib/LZ4F/lz4frame.c
	lib/LZ4F/lz4hc.c
	lib/LZ4F/xxhash.c
	lib/nifly/src/Animation.cpp
	lib/nifly/src/BasicTypes.cpp
	lib/nifly/src/bhk.cpp
	lib/nifly/src/ExtraData.cpp
	lib/nifly/src/Factory.cpp
	lib/nifly/src/Geometry.cpp
	lib/nifly/src/NifFile.cpp
	lib/nifly/src/NifUtil.cpp
	lib/nifly/src/Nodes.cpp
	lib/nifly/src/Object3d.cpp
	lib/nifly/src/Objects.cpp
	lib/nifly/src/Particles.cpp
	lib/nifly/src/Shaders.cpp
	lib/nifly/src/Skin.cpp
	lib/SOIL2/wfETC.c
	lib/SOIL2/image_DXT.c
	lib/SOIL2/image_helper.c
	lib/SOIL2/SOIL2.c
	lib/TinyXML-2/tinyxml2.cpp
	src/components/Anim.cpp
	src/components/Automorph.cpp
	src/components/ClippingFixer.cpp
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

set(BSOS_OUTFIT_STUDIO_SOURCES
	${BSOS_COMMON_SOURCES}
	src/components/PoseData.cpp
	src/components/PoseDataYaml.cpp
	src/components/PoseDataJson.cpp
	src/components/PoseDataHkx.cpp
	src/components/RefTemplates.cpp
	src/components/Automation.cpp
	src/components/TweakBrush.cpp
	src/components/UndoHistory.cpp
	src/components/WeightNorm.cpp
	src/files/FBXWrangler.cpp
	src/files/HkxFile.cpp
	src/files/MaskFile.cpp
	src/files/ObjFile.cpp
	src/files/SFMorphFile.cpp
	src/ui/wxBrushSettingsPopup.cpp
	src/ui/WeightCopyDialog.cpp
	src/ui/wxSliderPanel.cpp
	src/program/EditUV.cpp
	src/program/FBXImportDialog.cpp
	src/program/ObjImportDialog.cpp
	src/program/OutfitProject.cpp
	src/program/OutfitStudio.cpp
	src/program/ShapeProperties.cpp
	src/program/ConvertBodyReferenceDialog.cpp
	src/program/AutomationDialog.cpp
	src/program/SliderDataDialog.cpp
	src/program/SliderDataImportDialog.cpp
	src/render/GLCanvas.cpp
	src/render/GLDialog.cpp
)

set(BSOS_BODYSLIDE_SOURCES
	${BSOS_COMMON_SOURCES}
	src/components/BuildSelection.cpp
	src/files/ObjFile.cpp
	src/files/SFMorphFile.cpp
	src/files/wxDDSImage.cpp
	src/program/BodySlideApp.cpp
	src/program/NormalsGenDialog.cpp
	src/program/PreviewWindow.cpp
	src/ui/PreviewPanel.cpp
	src/ui/wxNormalsGenDlg.cpp
)

if(WIN32)
	list(APPEND BSOS_OUTFIT_STUDIO_SOURCES OutfitStudio.rc)
	list(APPEND BSOS_BODYSLIDE_SOURCES BodySlide.rc)
endif()

add_executable(OutfitStudio ${BSOS_OUTFIT_STUDIO_SOURCES})
add_executable(BodySlide ${BSOS_BODYSLIDE_SOURCES})

set(BSOS_COMMON_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/lib
	${CMAKE_CURRENT_SOURCE_DIR}/lib/gli
	${CMAKE_CURRENT_SOURCE_DIR}/lib/nifly/include
	${CMAKE_CURRENT_SOURCE_DIR}/lib/nifly/external
	${CMAKE_CURRENT_SOURCE_DIR}/lib/TinyXML-2
	${CMAKE_CURRENT_SOURCE_DIR}/src
)
set(BSOS_OUTFIT_STUDIO_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/lib/fkYAML/include
	${CMAKE_CURRENT_SOURCE_DIR}/lib/nlohmannjson/include
)

set(BSOS_PLATFORM_DEFINITIONS)
if(WIN32)
	list(APPEND BSOS_PLATFORM_DEFINITIONS _WINDOWS _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN NOMINMAX)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		list(APPEND BSOS_PLATFORM_DEFINITIONS WIN64)
	else()
		list(APPEND BSOS_PLATFORM_DEFINITIONS WIN32)
	endif()
endif()

function(bsos_configure_target target)
	target_include_directories(${target} PRIVATE ${BSOS_COMMON_INCLUDE_DIRS})
	if(NOT TARGET OpenGL::GL AND OPENGL_INCLUDE_DIR)
		target_include_directories(${target} SYSTEM PRIVATE ${OPENGL_INCLUDE_DIR})
	endif()
	if(UNIX AND NOT APPLE AND NOT TARGET GLEW::GLEW AND GLEW_INCLUDE_DIRS)
		target_include_directories(${target} SYSTEM PRIVATE ${GLEW_INCLUDE_DIRS})
	endif()
	target_compile_definitions(${target} PRIVATE ${BSOS_PLATFORM_DEFINITIONS})
	target_link_libraries(${target} PRIVATE
		${BSOS_WX_LIBRARIES}
		${BSOS_OPENGL_LIBRARIES}
		${BSOS_GLEW_LIBRARY}
	)

	if(MSVC)
		target_compile_options(${target} PRIVATE /W4 /bigobj /wd26812)
	elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
		target_compile_options(${target} PRIVATE -Wall)
	endif()
endfunction()

bsos_configure_target(OutfitStudio)
bsos_configure_target(BodySlide)

target_include_directories(OutfitStudio PRIVATE ${BSOS_OUTFIT_STUDIO_INCLUDE_DIRS})

function(bsos_link_config_libraries target debug_library release_library)
	if(debug_library AND release_library)
		target_link_libraries(${target} PRIVATE
			$<$<CONFIG:Debug>:${debug_library}>
			$<$<NOT:$<CONFIG:Debug>>:${release_library}>
		)
	elseif(debug_library)
		target_link_libraries(${target} PRIVATE ${debug_library})
	elseif(release_library)
		target_link_libraries(${target} PRIVATE ${release_library})
	endif()
endfunction()

if(BSOS_HAVE_FBXSDK)
	target_compile_definitions(OutfitStudio PRIVATE USE_FBXSDK)
	target_link_libraries(OutfitStudio PRIVATE FBXSDK::FBXSDK)
	bsos_link_config_libraries(OutfitStudio "${FBXSDK_XML2_LIBRARY_DEBUG}" "${FBXSDK_XML2_LIBRARY_RELEASE}")
	if(NOT FBXSDK_XML2_LIBRARY_DEBUG AND NOT FBXSDK_XML2_LIBRARY_RELEASE AND TARGET LibXml2::LibXml2)
		target_link_libraries(OutfitStudio PRIVATE LibXml2::LibXml2)
	endif()
	bsos_link_config_libraries(OutfitStudio "${FBXSDK_ZLIB_LIBRARY_DEBUG}" "${FBXSDK_ZLIB_LIBRARY_RELEASE}")
	if(NOT FBXSDK_ZLIB_LIBRARY_DEBUG AND NOT FBXSDK_ZLIB_LIBRARY_RELEASE AND TARGET ZLIB::ZLIB)
		target_link_libraries(OutfitStudio PRIVATE ZLIB::ZLIB)
	endif()
endif()

if(MSVC)
	set_target_properties(OutfitStudio PROPERTIES WIN32_EXECUTABLE ON)
	set_target_properties(BodySlide PROPERTIES WIN32_EXECUTABLE ON)
	target_link_options(OutfitStudio PRIVATE /ignore:4099 /MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/OutfitStudio.manifest)
	target_link_options(BodySlide PRIVATE /MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/BodySlide.manifest)
elseif(UNIX AND NOT APPLE)
	target_link_options(OutfitStudio PRIVATE -rdynamic)
	target_link_options(BodySlide PRIVATE -rdynamic)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set_target_properties(OutfitStudio PROPERTIES OUTPUT_NAME "OutfitStudio x64")
	set_target_properties(BodySlide PROPERTIES OUTPUT_NAME "BodySlide x64")
else()
	set_target_properties(OutfitStudio PROPERTIES OUTPUT_NAME "OutfitStudio")
	set_target_properties(BodySlide PROPERTIES OUTPUT_NAME "BodySlide")
endif()
