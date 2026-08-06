#pragma once

#include <algorithm>
#include <cctype>
#include <string>

// Minimal subset of hdtSMP64's string helpers needed by the ported physics
// code.
namespace hdt
{
	// Trim ASCII whitespace from both ends of a string.
	inline std::string TrimAsciiWhitespace(const std::string& s)
	{
		auto start = s.find_first_not_of(" \t\r\n");
		if (start == std::string::npos)
			return "";
		auto end = s.find_last_not_of(" \t\r\n");
		return s.substr(start, end - start + 1);
	}

	// ASCII lower-case copy. Name comparisons folded with this mirror the
	// engine's case-insensitive BSFixedString matching.
	inline std::string ToLowerAscii(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return s;
	}
}
