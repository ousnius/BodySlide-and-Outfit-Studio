#include "StringStuff.h"
#include <cctype>

bool StringsEqualNInsens(const char *a, const char *b, int len)
{
	while (len > 0) {
		if (std::tolower(*a) != std::tolower(*b)) return false;
		if (*a == '\0') return true;
		++a, ++b, --len;
	}
	return true;
}

bool StringsEqualInsens(const char *a, const char *b)
{
	while (true) {
		if (std::tolower(*a) != std::tolower(*b)) return false;
		if (*a == '\0') return true;
		++a, ++b;
	}
}

std::string ToOSSlashes(const std::string &s)
{
	std::string d(s);
	size_t len = d.length();
	for (size_t i = 0; i < len; ++i)
#ifdef _WINDOWS
		if (d[i] == '/')
			d[i] = '\\';
#else
		if (d[i] == '\\')
			d[i] = '/';
#endif
	return d;
}

std::string ToBackslashes(const std::string &s)
{
	std::string d(s);
	size_t len = d.length();
	for (size_t i = 0; i < len; ++i)
		if (d[i] == '/')
			d[i] = '\\';
	return d;
}
