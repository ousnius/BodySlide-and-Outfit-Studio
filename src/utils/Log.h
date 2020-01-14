/*
BodySlide and Outfit Studio
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
		fileName = fileName.AfterLast('/').AfterLast('\\');
		int ihour = logTime.GetHour();
		int iminute = logTime.GetMinute();
		int isecond = logTime.GetSecond();
		int ilevel = level;
		return wxString::Format("[%02d:%02d:%02d][%d]\t%s(%d): %s",
			ihour, iminute, isecond, ilevel, fileName, info.line, msg);
	}
};

//Example
//[19:30:25][3] Message here
class LogFormatterNoFile : public wxLogFormatter {
	virtual wxString Format(wxLogLevel level, const wxString& msg, const wxLogRecordInfo& info) const {
		wxDateTime logTime(info.timestamp);
		int ihour = logTime.GetHour();
		int iminute = logTime.GetMinute();
		int isecond = logTime.GetSecond();
		int ilevel = level;
		return wxString::Format("[%02d:%02d:%02d][%d]\t%s",
			ihour, iminute, isecond, ilevel, msg);
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
