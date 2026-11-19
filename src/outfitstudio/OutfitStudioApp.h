/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "utils/Log.h"
#include "utils/TargetGame.h"

#include <wx/app.h>
#include <wx/cmdline.h>
#include <wx/ipc.h>
#include <wx/snglinst.h>

class OutfitStudioFrame;

class OutfitStudio : public wxApp {
public:
	virtual ~OutfitStudio();

	virtual bool OnInit();
	virtual int OnRun() override;
	virtual int OnExit() override;
	virtual void OnInitCmdLine(wxCmdLineParser& parser);
	virtual bool OnCmdLineParsed(wxCmdLineParser& parser);

	virtual bool OnExceptionInMainLoop();
	virtual void OnUnhandledException();
	virtual void OnFatalException();

	void CharHook(wxKeyEvent& event);

	void InitLanguage();

	bool SetDefaultConfig();
	bool ShowSetup();

	TargetGame targetGame = TargetGame::FO3;

private:
	OutfitStudioFrame* frame = nullptr;

	Log logger;
	wxLocale* locale = nullptr;
	int language = 0;

	wxArrayString cmdFiles;
	wxString cmdProject;
	wxString cmdAutomation;
	int cmdForceSingleInstanceBehavior = -1;  // -1 = not set, 0 = no (force new), 1 = yes (force existing)
	bool cmdMcp = false;
	long cmdMcpPort = -1;
	int automationExitCode = 0;

	int RunAutomationFromCmdLine();

	// DDE uses a service name, TCP uses a port number
#if defined(__WINDOWS__) && wxUSE_DDE_FOR_IPC
	const wxString OS_IPC_SERVICE = "OutfitStudioIPC";
#else
	const wxString OS_IPC_SERVICE = "54318";
#endif

	// Single instance checker and IPC server
	wxSingleInstanceChecker* singleChecker = nullptr;
	wxServer* ipcServer = nullptr;
};

wxDECLARE_APP(OutfitStudio);
