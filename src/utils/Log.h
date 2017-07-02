/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include <wx/datetime.h>
#include <wx/log.h>
#include <fstream>

//Example
//[19:30:25][3] Log.h(10): Message here
class LogFormatter : public wxLogFormatter {
	virtual wxString Format(wxLogLevel level, const wxString& msg, const wxLogRecordInfo& info) const {
		wxDateTime logTime(info.timestamp);
		wxString fileName = info.filename;
		fileName = fileName.AfterLast('\\');
		return wxString::Format("[%02d:%02d:%02d][%d]\t%s(%d): %s",
			logTime.GetHour(), logTime.GetMinute(), logTime.GetSecond(), level, fileName, info.line, msg);
	}
};

//Example
//[19:30:25][3] Message here
class LogFormatterNoFile : public wxLogFormatter {
	virtual wxString Format(wxLogLevel level, const wxString& msg, const wxLogRecordInfo& info) const {
		wxDateTime logTime(info.timestamp);
		return wxString::Format("[%02d:%02d:%02d][%d]\t%s",
			logTime.GetHour(), logTime.GetMinute(), logTime.GetSecond(), level, msg);
	}
};

class Log {
	//Stream doing the actual writing
	std::ofstream stream;

public:
	//Opens and truncates log file to a maximum of runs
	void Initialize(int level = -1, const wxString& fileName = "Log.txt");

	//Swaps out log formatter
	void SetFormatter(bool withFile = true);
};
