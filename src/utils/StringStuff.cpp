#include "StringStuff.h"
#include <ctype.h>

bool StringsEqualNInsens(const char *a, const char *b, int len)
{
	while (len > 0) {
		if (tolower(*a) != tolower(*b)) return false;
		if (*a == '\0') return true;
		++a, ++b, --len;
	}
	return true;
}

bool StringsEqualInsens(const char *a, const char *b)
{
	while (true) {
		if (tolower(*a) != tolower(*b)) return false;
		if (*a == '\0') return true;
		++a, ++b;
	}
}
