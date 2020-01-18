/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <string>

/* StringsEqualNInsens: returns true if the first len characters of a
and b are the same insensitive to case. */
bool StringsEqualNInsens(const char *a, const char *b, int len);

/* StringsEqualInsens: returns true if the strings a and b are the same
insensitive to case. */
bool StringsEqualInsens(const char *a, const char *b);

/* ToOSSlash: converts all forward and back slashes in s to the path
separator character for the operating system and returns the result. */
std::string ToOSSlashes(const std::string &s);

/* ToBackslashes: converts all forward slashes into backslashes in s
and returns the result. */
std::string ToBackslashes(const std::string &s);

#ifdef _WINDOWS
constexpr char PathSepChar = '\\';
constexpr const char *PathSepStr = "\\";
#else
constexpr char PathSepChar = '/';
constexpr const char *PathSepStr = "/";
#endif
