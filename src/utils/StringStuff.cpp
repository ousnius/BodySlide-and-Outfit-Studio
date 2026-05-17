#include "StringStuff.h"

#include <algorithm>
#include <cctype>
#include <limits>
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

std::string TrimString(const std::string& value) {
	size_t first = value.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return "";
	size_t last = value.find_last_not_of(" \t\r\n");
	return value.substr(first, last - first + 1);
}

bool ParseUInt32Value(const std::string& text, uint32_t& value) {
	std::string trimmed = TrimString(text);
	if (trimmed.empty())
		return false;
	if (trimmed[0] == '-')
		return false;

	try {
		size_t pos = 0;
		unsigned long parsed = std::stoul(trimmed, &pos, 0);
		if (pos != trimmed.size() || parsed > std::numeric_limits<uint32_t>::max())
			return false;
		value = static_cast<uint32_t>(parsed);
		return true;
	}
	catch (...) {
		return false;
	}
}

bool ParseFloatValue(const std::string& text, float& value) {
	std::string trimmed = TrimString(text);
	if (trimmed.empty())
		return false;

	try {
		size_t pos = 0;
		float parsed = std::stof(trimmed, &pos);
		if (pos != trimmed.size())
			return false;
		value = parsed;
		return true;
	}
	catch (...) {
		return false;
	}
}

bool ParseBoolValue(const std::string& text, bool& value) {
	std::string lower = ToLower(TrimString(text));
	if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") {
		value = true;
		return true;
	}
	if (lower == "false" || lower == "0" || lower == "no" || lower == "off") {
		value = false;
		return true;
	}
	return false;
}
