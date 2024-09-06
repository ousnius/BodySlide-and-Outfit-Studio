#include "StringStuff.h"
#include <cctype>
#include <sstream>

bool StringsEqualNInsens(const char* a, const char* b, int len) {
	while (len > 0) {
		if (std::tolower(*a) != std::tolower(*b))
			return false;
		if (*a == '\0')
			return true;
		++a, ++b, --len;
	}
	return true;
}

bool StringsEqualInsens(const char* a, const char* b) {
	while (true) {
		if (std::tolower(*a) != std::tolower(*b))
			return false;
		if (*a == '\0')
			return true;
		++a, ++b;
	}
}

std::string ToOSSlashes(const std::string& s) {
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

std::string ToBackslashes(const std::string& s) {
	std::string d(s);
	size_t len = d.length();
	for (size_t i = 0; i < len; ++i)
		if (d[i] == '/')
			d[i] = '\\';
	return d;
}

std::vector<std::string> SplitString(const std::string& s, const char delim) {
	std::stringstream sstrm(s);
	std::string segment;
	std::vector<std::string> seglist;

	while (std::getline(sstrm, segment, delim)) {
		seglist.push_back(segment);
	}

	return seglist;
}

std::string JoinStrings(const std::vector<std::string>& elements, const char* const separator) {
	switch (elements.size()) {
		case 0: return "";
		case 1: return elements[0];
		default:
			std::ostringstream os;
			std::copy(elements.begin(), elements.end() - 1, std::ostream_iterator<std::string>(os, separator));
			os << *elements.rbegin();
			return os.str();
	}
}
