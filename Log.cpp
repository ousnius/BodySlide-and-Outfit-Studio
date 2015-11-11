/*
BodySlide and Outfit Studio
Copyright (C) 2015  Caliente & ousnius
See the included LICENSE file
*/

#include "Log.h"
#include "BodySlideApp.h"

/*
<LogLevel>
-1: Off
0: wxLogFatalError
1: wxLogError
2: wxLogWarning
3: wxLogMessage
*/

void Log::Initialize(wxString fileName) {
	wxLog::EnableLogging(false);
	int logLevel = Config.GetIntValue("LogLevel", -1);

	if (logLevel >= 0) {
		//Signature to find separate runs
		string signature = "[#Log";
		string textCopy;
		string word;
		int sigCount = 0;

		//Open stream at end to find out file size
		ifstream truncStream(fileName.ToStdString(), ios_base::ate);
		if (truncStream) {
			//Store size and seek back to beginning
			int streamSize = truncStream.tellg();
			truncStream.seekg(0, ios::beg);

			//Count all occurences of the signature
			sigCount = count(istream_iterator<string>(truncStream), istream_iterator<string>(), signature);
			if (sigCount >= 4) {
				//Maximum amount of runs per log file exceeded, seek back to beginning
				sigCount = 0;
				truncStream.clear();
				truncStream.seekg(0, ios::beg);

				//Count signatures until the 2nd, starting the beginning, is reached
				auto pos = istream_iterator<string>(truncStream);
				auto end = istream_iterator<string>();
				for (; pos != end; ++pos) {
					if (*pos == signature)
						sigCount++;

					if (sigCount >= 2) {
						//Read text from 2nd signature to EOF
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

		//Open ofstream, creating empty log file
		stream.open(fileName.ToStdString(), ios_base::app);
		if (stream) {
			wxLog* log = new wxLogStream(&stream);
			log->SetLogLevel(logLevel);

			//Copy partial contents of old to new log
			if (!textCopy.empty()) {
				stream.close();
				stream.clear();
				stream.open(fileName.ToStdString());
				log->LogText(textCopy);
			}

			//Write new signature line with date
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
