/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PlatformUtil.h"

namespace PlatformUtil {
#ifdef _WINDOWS
	// ACP wide to multibyte
	std::string WideToMultiByteACP(const std::wstring &wstr) {
		if (wstr.empty())
			return std::string();

		int size_needed = WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
		std::string strTo(size_needed, 0);
		WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
		return strTo;
	}

	// UTF-8 multibyte to wide
	std::wstring MultiByteToWideUTF8(const std::string &str) {
		if (str.empty())
			return std::wstring();

		int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), nullptr, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
		return wstrTo;
	}
#endif

	void OpenFileStream(std::fstream& file, const std::string& fileName, unsigned int mode) {
#ifdef _WINDOWS
		// Convert to std::wstring on Windows only
		file.open(MultiByteToWideUTF8(fileName).c_str(), mode);
#else
		file.open(fileName.c_str(), mode);
#endif
	}

	// Provide std::wstring function for Windows
#ifdef _WINDOWS
	void OpenFileStream(std::fstream& file, const std::wstring& fileName, unsigned int mode) {
		file.open(fileName.c_str(), mode);
	}
#endif
}
