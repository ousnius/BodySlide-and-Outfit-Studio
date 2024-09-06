/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <string>
#include <vector>

/* StringsEqualNInsens: returns true if the first len characters of a
and b are the same insensitive to case. */
bool StringsEqualNInsens(const char* a, const char* b, int len);

/* StringsEqualInsens: returns true if the strings a and b are the same
insensitive to case. */
bool StringsEqualInsens(const char* a, const char* b);

/* ToOSSlash: converts all forward and back slashes in s to the path
separator character for the operating system and returns the result. */
std::string ToOSSlashes(const std::string& s);

/* ToBackslashes: converts all forward slashes into backslashes in s
and returns the result. */
std::string ToBackslashes(const std::string& s);

/* SplitString: splits a string into a vector of strings using a delimiter */
std::vector<std::string> SplitString(const std::string& s, const char delim);

/* JoinStrings: joins a vector of strings into one string with separators */
std::string JoinStrings(const std::vector<std::string>& elements, const char* const separator);

/* case_insensitive_compare: can be used for maps and more */
struct case_insensitive_compare {
	struct nocase_compare {
		bool operator()(const unsigned char& c1, const unsigned char& c2) const { return tolower(c1) < tolower(c2); }
	};

	bool operator()(const std::string& s1, const std::string& s2) const { return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(), nocase_compare()); }
};

#ifdef _WINDOWS
constexpr char PathSepChar = '\\';
constexpr const char* PathSepStr = "\\";
#else
constexpr char PathSepChar = '/';
constexpr const char* PathSepStr = "/";
#endif
