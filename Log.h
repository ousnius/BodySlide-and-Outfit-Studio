#pragma once
#include <wx/datetime.h>
#include <wx/log.h>
#include <fstream>

class LogFormatter : public wxLogFormatter {
	virtual wxString Format(wxLogLevel level, const wxString& msg, const wxLogRecordInfo& info) const {
		wxDateTime logTime(info.timestamp);
		return wxString::Format("[%02d:%02d:%02d][%d] %s(%d): %s",
			logTime.GetHour(), logTime.GetMinute(), logTime.GetSecond(), level, info.filename, info.line, msg);
	}
};

class LogFormatterNoFile : public wxLogFormatter {
	virtual wxString Format(wxLogLevel level, const wxString& msg, const wxLogRecordInfo& info) const {
		wxDateTime logTime(info.timestamp);
		return wxString::Format("[%02d:%02d:%02d][%d] %s",
			logTime.GetHour(), logTime.GetMinute(), logTime.GetSecond(), level, msg);
	}
};

class Log {
	std::ofstream stream;

public:
	void Initialize(wxString fileName = "Log.txt");
	void SetFormatter(bool withFile = true);
};
