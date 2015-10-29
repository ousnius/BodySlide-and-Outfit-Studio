/*
BodySlide and Outfit Studio
Copyright (C) 2015  Caliente & ousnius
See the included LICENSE file
*/

#include "XmlFinder.h"

XmlFinder::XmlFinder(const wxString& path) {
	basePath = path + "\\";
	wxString fileFilter = path + "\\*.xml";
	hfind = FindFirstFile(fileFilter.wc_str(), &wfd);
	if (hfind == INVALID_HANDLE_VALUE) {
		return;
	}
}

XmlFinder::~XmlFinder() {
	if (hfind != INVALID_HANDLE_VALUE) {
		close();
	}
}

wxString XmlFinder::next() {
	wxString fileName = basePath + wfd.cFileName;

	if (!FindNextFile(hfind, &wfd)) {
		DWORD searchStatus = GetLastError();
		if (searchStatus != ERROR_NO_MORE_FILES) {
			error = true;
		}
		close();
	}

	return fileName;
}

void XmlFinder::close() {
	FindClose(hfind);
	hfind = INVALID_HANDLE_VALUE;
}
