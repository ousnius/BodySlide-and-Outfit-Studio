#pragma once

#include "stdafx.h"

using namespace std;

class XmlFinder {
public:
	explicit XmlFinder(const wxString& path);
	virtual ~XmlFinder();

	bool atEnd() const {
		return hfind == INVALID_HANDLE_VALUE;
	}

	wxString next();

	bool hadError() const {
		return error;
	}

private:
	void advanceUntilXml();
	void close();

	WIN32_FIND_DATAW wfd;
	HANDLE hfind;
	wxString basePath;
	bool error = false;
};
