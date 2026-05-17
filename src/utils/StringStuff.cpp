#include "StringStuff.h"

#include <algorithm>
#include <cctype>
#include <sstream>

bool StringsEqualNInsens(const char* a, const char* b, int len) {
	while (len > 0) {
		if (ToLower(*a) != ToLower(*b))
			return false;
		if (*a == '\0')
			return true;
		++a, ++b, --len;
	}
	return true;
}

bool StringsEqualInsens(const char* a, const char* b) {
	while (true) {
		if (ToLower(*a) != ToLower(*b))
			return false;
		if (*a == '\0')
			return true;
		++a, ++b;
	}
}

bool StringStartsWith(std::string_view s, std::string_view prefix) {
	return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

bool StringEndsWith(std::string_view s, std::string_view suffix) {
	return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

char ToLower(unsigned char c) {
	return static_cast<char>(std::tolower(c));
}

std::string ToLower(const std::string& s) {
	std::string d(s);
	std::transform(d.begin(), d.end(), d.begin(), [](unsigned char c) { return ToLower(c); });
	return d;
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
