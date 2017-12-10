/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#ifdef _WINDOWS
	#include <Windows.h>
#endif

#include <string>
#include <fstream>

namespace PlatformUtil {
#ifdef _WINDOWS
	// ACP wide to multibyte
	std::string WideToMultiByteACP(const std::wstring& wstr);

	// UTF-8 multibyte to wide
	std::wstring MultiByteToWideUTF8(const std::string& str);
#endif

	void OpenFileStream(std::fstream& file, const std::string& fileName, unsigned int mode);

	// Provide std::wstring function for Windows
#ifdef _WINDOWS
	void OpenFileStream(std::fstream& file, const std::wstring& fileName, unsigned int mode);
#endif
}
