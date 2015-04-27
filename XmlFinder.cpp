#include "XmlFinder.h"

using std::string;

XmlFinder::XmlFinder(const std::string& path) {
	basePath = path + "\\";
	string filefilter = path + "\\*.xml";
	hfind = FindFirstFileA(filefilter.c_str(), &wfd);
	if (hfind == INVALID_HANDLE_VALUE) {
		return;
	}
}

XmlFinder::~XmlFinder() {
	if (hfind != INVALID_HANDLE_VALUE) {
		close();
	}
}

std::string XmlFinder::next() {
	std::string filename = basePath + wfd.cFileName;

	if (!FindNextFileA(hfind, &wfd)) {
		DWORD searchStatus = GetLastError();
		if (searchStatus != ERROR_NO_MORE_FILES) {
			error = true;
		}
		close();
	}

	return filename;
}

void XmlFinder::close() {
	FindClose(hfind);
	hfind = INVALID_HANDLE_VALUE;
}
