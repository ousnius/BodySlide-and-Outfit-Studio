#include "Log.h"
#include "BodySlideApp.h"

void Log::Initialize(wxString fileName) {
	wxLog::EnableLogging(false);
	int logLevel = Config.GetIntValue("LogLevel", -1);

	if (logLevel >= 0) {
		string signature = "[#Log";
		string textCopy;
		string word;
		int sigCount = 0;

		ifstream truncStream(fileName.ToStdString(), ios_base::ate);
		if (truncStream) {
			int streamSize = truncStream.tellg();
			truncStream.seekg(0, ios::beg);

			sigCount = count(istream_iterator<string>(truncStream), istream_iterator<string>(), signature);
			if (sigCount >= 4) {
				sigCount = 0;
				truncStream.clear();
				truncStream.seekg(0, ios::beg);

				auto pos = istream_iterator<string>(truncStream);
				auto end = istream_iterator<string>();
				for (; pos != end; ++pos) {
					if (*pos == signature)
						sigCount++;

					if (sigCount >= 2) {
						int posCopy = truncStream.tellg();
						textCopy.resize(streamSize - posCopy);
						truncStream.read((char*)&textCopy.front(), textCopy.size());
						textCopy.insert(0, *pos);
						int trim = textCopy.find_last_of('\n');
						if (trim != string::npos)
							textCopy.erase(trim);
						break;
					}
				}
			}
			truncStream.close();
		}

		stream.open(fileName.ToStdString(), ios_base::app);
		if (stream) {
			wxLog* log = new wxLogStream(&stream);
			log->SetLogLevel(logLevel);

			if (!textCopy.empty()) {
				stream.close();
				stream.clear();
				stream.open(fileName.ToStdString());
				log->LogText(textCopy);
			}

			log->LogText(wxString::Format("%s %s]", signature, wxNow()));
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
