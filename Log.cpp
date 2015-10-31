#include "Log.h"
#include "BodySlideApp.h"

void Log::Initialize(wxString fileName) {
	wxLog::EnableLogging(false);
	int logLevel = Config.GetIntValue("LogLevel", -1);

	if (logLevel >= 0) {
		stream.open(fileName.ToStdString());
		if (stream) {
			wxLog* log = new wxLogStream(&stream);
			log->SetLogLevel(logLevel);

			wxLog::SetActiveTarget(log);
			wxLog::EnableLogging();
			SetFormatter();
		}
	}
}

void Log::SetFormatter(bool withFile) {
	wxLogFormatter* oldFormatter = nullptr;

	if (withFile)
		oldFormatter = wxLog::GetActiveTarget()->SetFormatter(new LogFormatter());
	else
		oldFormatter = wxLog::GetActiveTarget()->SetFormatter(new LogFormatterNoFile());

	if (oldFormatter)
		delete oldFormatter;
}
