/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#ifdef _WINDOWS
#include <Windows.h>
#endif

#include <fstream>
#include <string>

namespace PlatformUtil {
#ifdef _WINDOWS
// ACP wide to multibyte
std::string WideToMultiByteACP(const std::wstring& wstr);

// UTF-8 multibyte to wide
std::wstring MultiByteToWideUTF8(const std::string& str);
#endif

void OpenFileStream(std::fstream& file, const std::string& fileName, std::ios_base::openmode mode);

// Provide std::wstring function for Windows
#ifdef _WINDOWS
void OpenFileStream(std::fstream& file, const std::wstring& fileName, unsigned int mode);
#endif
} // namespace PlatformUtil

// This user-defined literal allows you to specify an int with a
// multi-character string: "OSD\0"_mci
inline constexpr uint32_t operator"" _mci(const char* p, size_t n) {
	uint32_t v = 0;
	for (size_t i = 0; i < n; ++i) {
		v <<= 8;
		v += static_cast<unsigned char>(p[i]);
	}
	return v;
}
