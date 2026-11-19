/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// The Outfit Studio application: start-up, command line, setup, language and single instance

#include "OutfitStudioApp.h"
#include "OutfitStudioFrame.h"
#include "ui/AppSetup.h"
#include "utils/ConfigurationManager.h"
#include "FSEngine/FSManager.h"
#include "utils/TargetGame.h"
#include "files/HkxFile.h"
#include "files/MaskFile.h"
#include "ui/outfitstudio/AutomationDialog.h"
#include "utils/GameUtil.h"
#include "utils/StackTrace.h"
#include "ui/SettingsDialogShared.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>
#include <wx/debugrpt.h>
#include <wx/textctrl.h>

using namespace nifly;

// IPC connection handler for Outfit Studio single-instance checking
class OutfitStudioIPCConnection : public wxConnection {
public:
	OutfitStudioIPCConnection() {}

	virtual bool OnExec(const wxString& WXUNUSED(topic), const wxString& data) override {
		// data is expected to be newline-separated file paths
		wxArrayString files;
		wxStringTokenizer tokenizer(data, "\n");
		while (tokenizer.HasMoreTokens()) {
			wxString token = tokenizer.GetNextToken().Trim();
			if (!token.IsEmpty())
				files.Add(token);
		}

		OutfitStudio* app = dynamic_cast<OutfitStudio*>(wxTheApp);
		if (!files.IsEmpty() && app && app->GetTopWindow()) {
			OutfitStudioFrame* frame = dynamic_cast<OutfitStudioFrame*>(app->GetTopWindow());
			if (frame) {
				// Use wxCallAfter to call OpenFiles on the main GUI thread
				wxTheApp->CallAfter([frame, files]() {
					frame->LoadFiles(files);
				});
				return true;
			}
		}

		return false;
	}
};

class OutfitStudioIPCServer : public wxServer {
public:
	virtual wxConnectionBase* OnAcceptConnection(const wxString& WXUNUSED(topic)) override {
		return new OutfitStudioIPCConnection();
	}
};

wxIMPLEMENT_APP(OutfitStudio);

ConfigurationManager Config;
ConfigurationManager OutfitStudioConfig;

OutfitStudio::~OutfitStudio() {
	if (ipcServer) {
		delete ipcServer;
		ipcServer = nullptr;
	}

	if (singleChecker) {
		delete singleChecker;
		singleChecker = nullptr;
	}

	if (locale) {
		delete locale;
		locale = nullptr;
	}

	FSManager::del();
}

bool OutfitStudio::OnInit() {
	if (!wxApp::OnInit())
		return false;

#ifdef _DEBUG
	std::string dataDir{wxGetCwd().ToUTF8()};
#else
	std::string dataDir{wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath().ToUTF8()};
#endif

	Config.LoadConfig(dataDir + "/Config.xml");
	OutfitStudioConfig.LoadConfig(dataDir + "/OutfitStudio.xml", "OutfitStudioConfig");

	Config.SetDefaultValue("AppDir", dataDir);

	logger.Initialize(Config.GetIntValue("LogLevel", 2), dataDir + "/Log_OS.txt");
	wxLogMessage("Initializing Outfit Studio...");

#ifdef NDEBUG
	wxHandleFatalExceptions();
#endif

#ifdef __WXMSW__
	SetAppearance(SettingsDialogShared::GetConfiguredAppearance(Config));
#endif

	wxString appDirUri = wxString::FromUTF8(dataDir);
	appDirUri.Replace("#", "%23");
	wxSetEnv("AppDir", appDirUri);

	wxXmlResource* xrc = wxXmlResource::Get();
	xrc->SetFlags(wxXRC_USE_LOCALE | wxXRC_USE_ENVVARS);
	xrc->InitAllHandlers();
	wxInitAllImageHandlers();

	wxLogMessage("Working directory: %s", wxGetCwd());
	wxLogMessage("Executable directory: %s", wxString::FromUTF8(dataDir));
	if (!SetDefaultConfig())
		return false;

	InitLanguage();

	wxString gameName = "Target game: ";
	switch (targetGame) {
		case FO3: gameName.Append("Fallout 3"); break;
		case FONV: gameName.Append("Fallout New Vegas"); break;
		case SKYRIM: gameName.Append("Skyrim"); break;
		case FO4: gameName.Append("Fallout 4"); break;
		case SKYRIMSE: gameName.Append("Skyrim Special Edition"); break;
		case FO4VR: gameName.Append("Fallout 4 VR"); break;
		case SKYRIMVR: gameName.Append("Skyrim VR"); break;
		case FO76: gameName.Append("Fallout 76"); break;
		case OB: gameName.Append("Oblivion"); break;
		case SF: gameName.Append("Starfield"); break;
		default: gameName.Append("Invalid");
	}
	wxLogMessage(gameName);

	int x = OutfitStudioConfig.GetIntValue("OutfitStudioFrame.x");
	int y = OutfitStudioConfig.GetIntValue("OutfitStudioFrame.y");
	int w = OutfitStudioConfig.GetIntValue("OutfitStudioFrame.width");
	int h = OutfitStudioConfig.GetIntValue("OutfitStudioFrame.height");
	std::string maximized = OutfitStudioConfig["OutfitStudioFrame.maximized"];

	// create single instance checker
	singleChecker = new wxSingleInstanceChecker(wxString("OutfitStudioInstance"));

	// Headless automation mode: skip single-instance/IPC, do not show the frame,
	// run the named automation script and then close the frame so the app exits
	// cleanly through the normal wx event loop / destructor chain.
	if (!cmdAutomation.IsEmpty()) {
		frame = new OutfitStudioFrame(wxPoint(x, y), wxSize(w, h));
		// Intentionally do not call frame->Show() / Maximize().
		SetTopWindow(frame);

		GameUtil::InitArchives();

		// Defer execution until the main loop is running so wxYield/UI events work.
		CallAfter([this]() {
			automationExitCode = RunAutomationFromCmdLine();
			if (frame) {
				// Suppress the unsaved-changes prompt in headless mode.
				frame->SetPendingChanges(false);
				frame->Close(true);  // fires OnClose -> cleans project/glView, then frame deletion
			}
		});

		return true;
	}

	// If files were passed on the command line, try single-instance IPC via wxWidgets
	if (!cmdFiles.IsEmpty()) {
		if (singleChecker->IsAnotherRunning()) {
			int behavior = OutfitStudioConfig.GetIntValue("SingleInstanceBehavior", 0);

			// Override behavior with command line argument if provided
			if (cmdForceSingleInstanceBehavior >= 0) {
				// cmdForceSingleInstanceBehavior: 0 = force new, 1 = force existing
				behavior = cmdForceSingleInstanceBehavior == 1 ? 1 : 2;  // 1 = Open in Existing, 2 = Open in New
			}

			// 0 = Ask (Message Box), 1 = Open in Existing, 2 = Open in New
			int answer = wxYES;
			if (behavior == 0) {
				wxDialog dlg(nullptr, wxID_ANY, _("Open in existing instance?"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
				wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

				wxBoxSizer* msgSizer = new wxBoxSizer(wxHORIZONTAL);
				msgSizer->Add(new wxStaticBitmap(&dlg, wxID_ANY, wxArtProvider::GetBitmap(wxART_QUESTION, wxART_MESSAGE_BOX)), 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
				msgSizer->Add(new wxStaticText(&dlg, wxID_ANY, _("An instance of Outfit Studio is already running. Open file(s) in the existing instance?")), 1, wxALL | wxALIGN_CENTER_VERTICAL, 10);
				mainSizer->Add(msgSizer, 1, wxEXPAND);

				wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
				wxButton* btnYes = new wxButton(&dlg, wxID_YES, _("Yes"));
				wxButton* btnYesAlways = new wxButton(&dlg, wxID_YES + 100, _("Yes (always)"));
				wxButton* btnNo = new wxButton(&dlg, wxID_NO, _("No"));
				wxButton* btnNoNever = new wxButton(&dlg, wxID_NO + 100, _("No (never)"));
				btnSizer->Add(btnYes, 0, wxALL, 5);
				btnSizer->Add(btnYesAlways, 0, wxALL, 5);
				btnSizer->Add(btnNo, 0, wxALL, 5);
				btnSizer->Add(btnNoNever, 0, wxALL, 5);
				mainSizer->Add(btnSizer, 0, wxALIGN_CENTER | wxBOTTOM, 5);

				btnYes->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { dlg.EndModal(wxID_YES); });
				btnYesAlways->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { dlg.EndModal(wxID_YES + 100); });
				btnNo->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { dlg.EndModal(wxID_NO); });
				btnNoNever->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { dlg.EndModal(wxID_NO + 100); });

				dlg.SetSizerAndFit(mainSizer);
				dlg.CenterOnScreen();

				int result = dlg.ShowModal();
				if (result == wxID_YES || result == wxID_YES + 100) {
					answer = wxYES;
					if (result == wxID_YES + 100) {
						OutfitStudioConfig.SetValue("SingleInstanceBehavior", 1);
						OutfitStudioConfig.SaveConfig(Config["AppDir"] + "/OutfitStudio.xml", "OutfitStudioConfig");
					}
				}
				else {
					answer = wxNO;
					if (result == wxID_NO + 100) {
						OutfitStudioConfig.SetValue("SingleInstanceBehavior", 2);
						OutfitStudioConfig.SaveConfig(Config["AppDir"] + "/OutfitStudio.xml", "OutfitStudioConfig");
					}
				}
			}
			else if (behavior == 1) {
				answer = wxYES;  // Open in existing
			}
			else if (behavior == 2) {
				answer = wxNO;   // Open in new
			}

			if (answer == wxYES) {
				// Build newline-separated list
				wxString concat;
				for (size_t i = 0; i < cmdFiles.GetCount(); ++i) {
					if (i)
						concat.Append("\n");
					concat.Append(cmdFiles[i]);
				}

				wxClient client;
				// Try to connect to the server
				wxConnectionBase* conn = client.MakeConnection("localhost", OS_IPC_SERVICE, OS_IPC_SERVICE);
				if (conn) {
					conn->Execute(concat);
					conn->Disconnect();
				}

				// exit this new instance
				return false;
			}
		}
		else {
			// we'll be the server; create it after frame is created
		}
	}

	frame = new OutfitStudioFrame(wxPoint(x, y), wxSize(w, h));
	if (maximized == "true")
		frame->Maximize();

	frame->Show();
	SetTopWindow(frame);

	// If we are the primary instance create IPC server so subsequent launches can connect
	ipcServer = new OutfitStudioIPCServer();
	if (!ipcServer->Create(OS_IPC_SERVICE)) {
		delete ipcServer;
		ipcServer = nullptr;
	}

	GameUtil::InitArchives();

	if (!Config["GameDataPath"].empty()) {
		bool dirWritable = wxFileName::IsDirWritable(Config["GameDataPath"]);
		bool dirReadable = wxFileName::IsDirReadable(Config["GameDataPath"]);
		if (!dirWritable || !dirReadable)
			wxMessageBox(
				_("No read/write permission for game data path!\n\nPlease launch the program with admin elevation and make sure the game data path in the settings is correct."),
				_("Warning"),
				wxICON_WARNING);
	}

	if (!Config["ProjectPath"].empty()) {
		bool dirWritable = wxFileName::IsDirWritable(Config["ProjectPath"]);
		bool dirReadable = wxFileName::IsDirReadable(Config["ProjectPath"]);
		if (!dirWritable || !dirReadable)
			wxMessageBox(
				_("No read/write permission for project path!\n\nPlease launch the program with admin elevation and make sure the project path in the settings is correct."),
				_("Warning"),
				wxICON_WARNING);
	}

	if (!cmdFiles.IsEmpty()) {
		frame->LoadFiles(cmdFiles, cmdProject);
	}

	Bind(wxEVT_CHAR_HOOK, &OutfitStudio::CharHook, this);

	// Only --mcp asks for the server explicitly. Started from the settings, a second Outfit Studio
	// whose port is taken by the first one just says so in the status bar.
	if (cmdMcp || OutfitStudioConfig.GetBoolValue("MCP/Enabled"))
		frame->StartMcpServer(cmdMcpPort > 0 ? static_cast<int>(cmdMcpPort) : -1, cmdMcp);

	frame->UpdateTitle();
	wxLogMessage("Outfit Studio initialized.");
	return true;
}

static const wxCmdLineEntryDesc g_cmdLineDesc[] = {{wxCMD_LINE_OPTION, "proj", "project", "Project Name", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL},
												   {wxCMD_LINE_OPTION, "single", "single-instance", "Force single instance behavior (yes/no)", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL},
												   {wxCMD_LINE_OPTION, "a", "automation", "Run an automation script by name (no .xml extension); positional args are batch inputs", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL},
												   {wxCMD_LINE_SWITCH, nullptr, "mcp", "Start the MCP server that lets AI agents control Outfit Studio", wxCMD_LINE_VAL_NONE, 0},
												   {wxCMD_LINE_OPTION, nullptr, "mcp-port", "Port of the MCP server", wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL},
												   {wxCMD_LINE_PARAM, nullptr, nullptr, "Files", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE},
												   wxCMD_LINE_DESC_END};

void OutfitStudio::OnInitCmdLine(wxCmdLineParser& parser) {
	parser.SetDesc(g_cmdLineDesc);
	parser.SetSwitchChars("-");
}

bool OutfitStudio::OnCmdLineParsed(wxCmdLineParser& parser) {
	parser.Found("proj", &cmdProject);
	parser.Found("automation", &cmdAutomation);
	cmdMcp = parser.Found("mcp");
	parser.Found("mcp-port", &cmdMcpPort);

	wxString singleInstanceArg;
	if (parser.Found("single", &singleInstanceArg)) {
		wxString lowerArg = singleInstanceArg.Lower();
		if (lowerArg == "yes") {
			cmdForceSingleInstanceBehavior = 1;  // Force open in existing
		}
		else if (lowerArg == "no") {
			cmdForceSingleInstanceBehavior = 0;  // Force open in new
		}
		// Otherwise leave it as -1 (not set)
	}

	for (size_t i = 0; i < parser.GetParamCount(); i++)
		cmdFiles.Add(parser.GetParam(i));

	return true;
}

int OutfitStudio::RunAutomationFromCmdLine() {
	wxLogMessage("Automation: Running script '%s' in headless mode.", cmdAutomation);

	AutomationDialog* dlg = new AutomationDialog(frame, frame->project, true);
	// Dialog must remain unshown.

	int code = dlg->RunHeadless(cmdAutomation, cmdFiles);

	// Dialog is parented to the frame and would be auto-deleted on frame teardown,
	// but explicitly destroying it here releases its resources (XRC unload, status
	// bar, etc.) before the frame closes.
	dlg->Destroy();
	return code;
}

int OutfitStudio::OnRun() {
	int base = wxApp::OnRun();
	// The process exit code comes from here, wx ignores what OnExit returns.
	// In automation mode it is the script's exit code.
	return cmdAutomation.IsEmpty() ? base : automationExitCode;
}

int OutfitStudio::OnExit() {
	return wxApp::OnExit();
}

bool OutfitStudio::OnExceptionInMainLoop() {
	wxString error;
	try {
		throw;
	}
	catch (const std::exception& e) {
		error = e.what();
	}
	catch (...) {
		error = "unknown error";
	}

	wxLog::FlushActive();
	logger.SetFormatter(false);

	wxLogError("Unexpected exception has occurred: %s, the program will terminate.", error);
	wxMessageBox(wxString::Format(_("Unexpected exception has occurred: %s, the program will terminate."), error), _("Unexpected exception"), wxICON_ERROR);
	return false;
}

void OutfitStudio::OnUnhandledException() {
	wxString error;
	try {
		throw;
	}
	catch (const std::exception& e) {
		error = e.what();
	}
	catch (...) {
		error = "unknown error";
	}

	wxLog::FlushActive();
	logger.SetFormatter(false);

	wxLogError("Unhandled exception has occurred: %s, the program will terminate.", error);
	wxMessageBox(wxString::Format(_("Unhandled exception has occurred: %s, the program will terminate."), error), _("Unhandled exception"), wxICON_ERROR);
}

void OutfitStudio::OnFatalException() {
	wxLog::FlushActive();
	logger.SetFormatter(false);

	wxLogError("Fatal exception has occurred, the program will terminate.");
	LogStackTraceFromException();

	wxMessageBox(_("Fatal exception has occurred, the program will terminate."), _("Fatal exception"), wxICON_ERROR);

	wxDebugReport report;
	report.AddExceptionContext();
	report.Process();
}

void OutfitStudio::CharHook(wxKeyEvent& event) {
	wxWindow* w = (wxWindow*)event.GetEventObject();
	if (!w) {
		event.Skip();
		return;
	}

	if (!frame->IsDescendant(w)) {
		event.Skip();
		return;
	}

#ifdef _WINDOWS
	bool isTextCtrl = dynamic_cast<wxTextCtrl*>(w) != nullptr || dynamic_cast<wxComboBox*>(w) != nullptr;
	if (isTextCtrl) {
		BYTE keyState[256];
		GetKeyboardState(keyState);
		WCHAR result[4] = {};
		int len = ToUnicode(event.GetRawKeyCode(), MapVirtualKey(event.GetRawKeyCode(), MAPVK_VK_TO_VSC), keyState, result, 4, 0);
		if (len == 1 && result[0] >= 0x20) {
			HWND hwnd = static_cast<HWND>(w->GetHandle());
			::SendMessage(hwnd, WM_CHAR, result[0], event.GetRawKeyFlags());
			return;
		}
	}
#endif

	event.Skip();
}

bool OutfitStudio::SetDefaultConfig() {
	int currentTarget = -1;
	Config.SetDefaultValue("TargetGame", currentTarget);
	currentTarget = Config.GetIntValue("TargetGame");

	Config.SetDefaultBoolValue("WarnMissingGamePath", true);
	Config.SetDefaultBoolValue("BSATextureScan", true);
	Config.SetDefaultValue("LogLevel", "3");
	SettingsDialogShared::SetDefaultAppearanceMode(Config);
	Config.SetDefaultValue("Input/SliderMinimum", 0);
	Config.SetDefaultValue("Input/SliderMaximum", 100);
	Config.SetDefaultBoolValue("Input/LeftMousePan", false);
	Config.SetDefaultBoolValue("Input/BrushSettingsNearCursor", true);
	Config.SetDefaultBoolValue("Input/MaskHistory", true);
	Config.SetDefaultBoolValue("Input/ShapeHoverHighlight", true);
	Config.SetDefaultValue("Lights/Ambient", 15);
	Config.SetDefaultValue("Lights/Frontal", 100);
	Config.SetDefaultValue("Lights/Directional0", 0);
	Config.SetDefaultValue("Lights/Directional0.x", -90);
	Config.SetDefaultValue("Lights/Directional0.y", 10);
	Config.SetDefaultValue("Lights/Directional0.z", 100);
	Config.SetDefaultValue("Lights/Directional1", 0);
	Config.SetDefaultValue("Lights/Directional1.x", 70);
	Config.SetDefaultValue("Lights/Directional1.y", 10);
	Config.SetDefaultValue("Lights/Directional1.z", 100);
	Config.SetDefaultValue("Lights/Directional2", 0);
	Config.SetDefaultValue("Lights/Directional2.x", 30);
	Config.SetDefaultValue("Lights/Directional2.y", 20);
	Config.SetDefaultValue("Lights/Directional2.z", -100);
	Config.SetDefaultValue("Rendering/HDRIBackground", "");
	const wxSize outfitStudioFrameSize = wxWindow::FromDIP(wxSize(1360, 900), nullptr);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.width", outfitStudioFrameSize.GetWidth());
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.height", outfitStudioFrameSize.GetHeight());
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.x", 100);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.y", 100);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.sashpos", 768);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.sashrightpos", 200);

	constexpr int DEF_PROJECT_HISTORY = 15;
	if (!OutfitStudioConfig.Exists("ProjectHistory.maxcount"))
		OutfitStudioConfig.SetValue("ProjectHistory.maxcount", DEF_PROJECT_HISTORY);

	Config.SetDefaultValue("GameRegKey/Oblivion", "Software\\Bethesda Softworks\\Oblivion");
	Config.SetDefaultValue("GameRegVal/Oblivion", "Installed Path");
	Config.SetDefaultValue("GameRegKey/Fallout3", "Software\\Bethesda Softworks\\Fallout3");
	Config.SetDefaultValue("GameRegVal/Fallout3", "Installed Path");
	Config.SetDefaultValue("GameRegKey/FalloutNewVegas", "Software\\Bethesda Softworks\\FalloutNV");
	Config.SetDefaultValue("GameRegVal/FalloutNewVegas", "Installed Path");
	Config.SetDefaultValue("GameRegKey/Skyrim", "Software\\Bethesda Softworks\\Skyrim");
	Config.SetDefaultValue("GameRegVal/Skyrim", "Installed Path");
	Config.SetDefaultValue("GameRegKey/Fallout4", "Software\\Bethesda Softworks\\Fallout4");
	Config.SetDefaultValue("GameRegVal/Fallout4", "Installed Path");
	Config.SetDefaultValue("GameRegKey/SkyrimSpecialEdition", "Software\\Bethesda Softworks\\Skyrim Special Edition");
	Config.SetDefaultValue("GameRegVal/SkyrimSpecialEdition", "Installed Path");
	Config.SetDefaultValue("GameRegKey/Fallout4VR", "Software\\Bethesda Softworks\\Fallout 4 VR");
	Config.SetDefaultValue("GameRegVal/Fallout4VR", "Installed Path");
	Config.SetDefaultValue("GameRegKey/SkyrimVR", "Software\\Bethesda Softworks\\Skyrim VR");
	Config.SetDefaultValue("GameRegVal/SkyrimVR", "Installed Path");
	Config.SetDefaultValue("GameRegKey/Starfield", "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 1716740");
	Config.SetDefaultValue("GameRegVal/Starfield", "InstallLocation");

	// Target game not set, show setup dialog
	if (currentTarget == -1)
		if (!ShowSetup())
			return false;

	targetGame = (TargetGame)Config.GetIntValue("TargetGame");

	if (Config["GameDataPath"].empty()) {
		wxString installPath = GameUtil::GetRegistryDataPath(targetGame);
		if (!installPath.empty()) {
			Config.SetDefaultValue("GameDataPath", installPath.ToUTF8().data());
			wxLogMessage("Registry game data path: %s", installPath);
		}
		else if (Config["WarnMissingGamePath"] == "true") {
			wxLogWarning("Failed to find game install path registry key or GameDataPath in the config.");
			wxMessageBox(_("Failed to find game install path registry key or GameDataPath in the config."), _("Warning"), wxICON_WARNING);
		}
	}
	else
		wxLogMessage("Game data path in config: %s", Config["GameDataPath"]);

	if (!Config["OutputDataPath"].empty()) {
		wxLogMessage("Output data path in config: %s", Config["OutputDataPath"]);
	}

	if (!Config["ProjectPath"].empty()) {
		wxLogMessage("Project path in config: %s", Config["ProjectPath"]);
	}

	return true;
}

bool OutfitStudio::ShowSetup() {
	return AppSetup::ShowSetupDialog();
}

void OutfitStudio::InitLanguage() {
	AppSetup::InitLanguage(locale);
}
