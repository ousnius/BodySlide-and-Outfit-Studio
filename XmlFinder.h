#pragma once

#include "stdafx.h"
#include <string>

class XmlFinder {
public:
	explicit XmlFinder(const std::string& path);
	virtual ~XmlFinder();

	bool atEnd() const {
		return hfind == INVALID_HANDLE_VALUE;
	}

	std::string next();

	bool hadError() const {
		return error;
	}

private:
	void advanceUntilXml();
	void close();

	WIN32_FIND_DATAA wfd;
	HANDLE hfind;
	std::string basePath;
	bool error{false};
};
