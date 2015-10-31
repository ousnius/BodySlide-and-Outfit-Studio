#pragma once
#include "stdafx.h"

class LogFormatter : public wxLogFormatter {
	virtual wxString Format(wxLogLevel level, const wxString& msg, const wxLogRecordInfo& info) const {
		wxDateTime logTime(info.timestamp);
		return wxString::Format("[%d:%d:%d][%d] %s(%d): %s",
			logTime.GetHour(), logTime.GetMinute(), logTime.GetSecond(), level, info.filename, info.line, msg);
	}
};

class Log {
	std::ofstream* stream = nullptr;

public:
	Log(wxString fileName = "Log.txt");
	~Log();
};
