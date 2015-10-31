#include "Log.h"
#include "BodySlideApp.h"

Log::Log(wxString fileName) {
	wxLog::EnableLogging(false);
	int logLevel = Config.GetIntValue("LogLevel", -1);

	if (logLevel >= 0) {
		stream = new ofstream(fileName.ToStdString());
		if (*stream) {
			wxLog* log = new wxLogStream(stream);
			log->SetLogLevel(logLevel);

			wxLogFormatter* oldFormatter = log->SetFormatter(new LogFormatter());
			delete oldFormatter;

			wxLog::SetActiveTarget(log);
			wxLog::EnableLogging();
		}
	}
}

Log::~Log() {
	if (stream)
		delete stream;
}
