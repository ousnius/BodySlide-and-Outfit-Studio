/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <string>

namespace ProjectUtil {
	/// Get the project path with directory existence checks.
	/// If ProjectPath config is set and exists, returns it.
	/// Otherwise, checks fallback paths in order:
	///   1. AppDir/SliderSets
	///   2. GameDataPath/CalienteTools/BodySlide
	///   3. GameDataPath/Tools/BodySlide
	/// Falls back to AppDir if no configured path and no fallback exists.
	std::string GetProjectPath();
} // namespace ProjectUtil
