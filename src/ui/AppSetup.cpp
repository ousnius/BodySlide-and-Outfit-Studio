/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "AppSetup.h"
#include "SettingsDialogShared.h"
#include "utils/ConfigurationManager.h"
#include "utils/GameUtil.h"
#include "utils/TargetGame.h"

#include <map>
#include <wx/checklst.h>
#include <wx/choice.h>
#include <wx/dir.h>
#include <wx/filepicker.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/tokenzr.h>
#include <wx/xrc/xmlres.h>

namespace AppSetup {

const std::array<wxLanguage, 37> SupportedLangs = {wxLANGUAGE_ENGLISH,	  wxLANGUAGE_AFRIKAANS,		   wxLANGUAGE_ARABIC,  wxLANGUAGE_CATALAN,	  wxLANGUAGE_CZECH,
												   wxLANGUAGE_DANISH,	  wxLANGUAGE_GERMAN,		   wxLANGUAGE_GREEK,   wxLANGUAGE_SPANISH,	  wxLANGUAGE_BASQUE,
												   wxLANGUAGE_FINNISH,	  wxLANGUAGE_FRENCH,		   wxLANGUAGE_HINDI,   wxLANGUAGE_HUNGARIAN,  wxLANGUAGE_INDONESIAN,
												   wxLANGUAGE_ITALIAN,	  wxLANGUAGE_JAPANESE,		   wxLANGUAGE_KOREAN,  wxLANGUAGE_LITHUANIAN, wxLANGUAGE_LATVIAN,
												   wxLANGUAGE_MALAY,	  wxLANGUAGE_NORWEGIAN_BOKMAL, wxLANGUAGE_NEPALI,  wxLANGUAGE_DUTCH,	  wxLANGUAGE_POLISH,
												   wxLANGUAGE_PORTUGUESE, wxLANGUAGE_ROMANIAN,		   wxLANGUAGE_RUSSIAN, wxLANGUAGE_SLOVAK,	  wxLANGUAGE_SLOVENIAN,
												   wxLANGUAGE_ALBANIAN,	  wxLANGUAGE_SWEDISH,		   wxLANGUAGE_TAMIL,   wxLANGUAGE_TURKISH,	  wxLANGUAGE_UKRAINIAN,
												   wxLANGUAGE_VIETNAMESE, wxLANGUAGE_CHINESE};

bool ShowSetupDialog() {
	wxXmlResource* xrc = wxXmlResource::Get();
	bool loaded = xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/Setup.xrc");
	if (!loaded) {
		wxMessageBox("Failed to load Setup.xrc file!", _("Error"), wxICON_ERROR);
		return false;
	}

	wxDialog* setup = xrc->LoadDialog(nullptr, "dlgSetup");
	if (!setup)
		return true;

	setup->SetSize(setup->FromDIP(wxSize(700, -1)));
	setup->CenterOnScreen();

	// The button that picks each game and the folder picker of its data folder
	struct GameControls {
		TargetGame game;
		const char* button;
		const char* dir;
		wxButton* btn = nullptr;
		wxDirPickerCtrl* dirPicker = nullptr;
	};
	GameControls games[] = {
		{OB, "btOblivion", "dirOblivion"},
		{FO3, "btFallout3", "dirFallout3"},
		{FONV, "btFalloutNV", "dirFalloutNV"},
		{SKYRIM, "btSkyrim", "dirSkyrim"},
		{FO4, "btFallout4", "dirFallout4"},
		{SKYRIMSE, "btSkyrimSE", "dirSkyrimSE"},
		{FO4VR, "btFallout4VR", "dirFallout4VR"},
		{SKYRIMVR, "btSkyrimVR", "dirSkyrimVR"},
		{SF, "btStarfield", "dirStarfield"},
	};

	for (auto& g : games) {
		g.btn = XRCCTRL(*setup, g.button, wxButton);
		g.dirPicker = XRCCTRL(*setup, g.dir, wxDirPickerCtrl);

		const TargetGame game = g.game;
		g.btn->Bind(wxEVT_BUTTON, [setup, game](wxCommandEvent&) { setup->EndModal((int)game); });

		wxButton* btn = g.btn;
		wxDirPickerCtrl* dirPicker = g.dirPicker;
		dirPicker->Bind(wxEVT_DIRPICKER_CHANGED, [btn, dirPicker](wxFileDirPickerEvent&) { btn->Enable(dirPicker->GetDirName().DirExists()); });
	}

	for (auto& g : games) {
		wxFileName dir = GameUtil::GetGameDataPath(g.game);
		if (dir.DirExists()) {
			g.dirPicker->SetDirName(dir);
			g.btn->Enable();
		}
	}

	if (setup->ShowModal() == wxID_CANCEL) {
		delete setup;
		return false;
	}

	int targ = setup->GetReturnCode();
	Config.SetValue("TargetGame", targ);

	wxFileName dataDir;
	for (auto& g : games) {
		if (g.game == targ) {
			dataDir = g.dirPicker->GetDirName();
			GameUtil::DefaultSkeleton skeleton = GameUtil::GetDefaultSkeleton(targ);
			Config.SetValue("Anim/DefaultSkeletonReference", skeleton.file);
			Config.SetValue("Anim/SkeletonRootName", skeleton.rootName);
		}
	}

	Config.SetValue("GameDataPath", dataDir.GetFullPath().ToUTF8().data());
	Config.SetValue("GameDataPaths/" + GameUtil::TargetGames[targ].ToStdString(), dataDir.GetFullPath().ToUTF8().data());

	Config.SaveConfig(Config["AppDir"] + "/Config.xml");
	delete setup;
	return true;
}

void InitLanguage(wxLocale*& locale) {
	if (locale)
		delete locale;

	int lang = Config.GetIntValue("Language");
	if (lang < 0)
		lang = wxLANGUAGE_ENGLISH;

	// Load language if possible, fall back to English otherwise
	if (wxLocale::IsAvailable(lang)) {
		locale = new wxLocale(lang);
		locale->AddCatalogLookupPathPrefix(wxString::FromUTF8(Config["AppDir"]) + "/lang");
		locale->AddCatalog("BodySlide");

		if (!locale->IsOk()) {
			wxLogError("System language '%d' is wrong.", lang);
			wxMessageBox(wxString::Format(_("System language '%d' is wrong."), lang));

			delete locale;
			locale = new wxLocale(wxLANGUAGE_ENGLISH);
			lang = wxLANGUAGE_ENGLISH;
		}
	}
	else {
		wxLogError("The system language '%d' is not supported by your system. Try installing support for this language.", lang);
		wxMessageBox(wxString::Format(_("The system language '%d' is not supported by your system. Try installing support for this language."), lang));

		locale = new wxLocale(wxLANGUAGE_ENGLISH);
		lang = wxLANGUAGE_ENGLISH;
	}

	wxLogMessage("Using language '%s'.", wxLocale::GetLanguageName(lang));
}

void FillDataFileList(wxCheckListBox* dataFileList, const wxString& dataDir, int targetGame) {
	dataFileList->Clear();

	wxString cp = "GameDataFiles/" + GameUtil::TargetGames[targetGame];
	wxString activatedFiles = Config[cp];

	wxStringTokenizer tokenizer(activatedFiles, ";");
	std::map<wxString, bool> fsearch;
	while (tokenizer.HasMoreTokens()) {
		wxString val = tokenizer.GetNextToken().Trim(false).Trim();
		val.MakeLower();
		fsearch[val] = true;
	}

	wxArrayString files;
	wxDir::GetAllFiles(dataDir, &files, "*.ba2", wxDIR_FILES);
	wxDir::GetAllFiles(dataDir, &files, "*.bsa", wxDIR_FILES);
	for (auto& file : files) {
		file = file.AfterLast('/').AfterLast('\\');
		dataFileList->Insert(file, dataFileList->GetCount());

		if (fsearch.find(file.Lower()) == fsearch.end())
			dataFileList->Check(dataFileList->GetCount() - 1);
	}
}

void OnChooseTargetGame(wxCommandEvent& event) {
	wxChoice* choiceTargetGame = (wxChoice*)event.GetEventObject();
	wxWindow* parent = choiceTargetGame->GetGrandParent();
	wxFilePickerCtrl* fpSkeletonFile = XRCCTRL(*parent, "fpSkeletonFile", wxFilePickerCtrl);
	wxChoice* choiceSkeletonRoot = XRCCTRL(*parent, "choiceSkeletonRoot", wxChoice);

	TargetGame targ = (TargetGame)choiceTargetGame->GetSelection();
	GameUtil::DefaultSkeleton skeleton = GameUtil::GetDefaultSkeleton(targ);
	fpSkeletonFile->SetPath(skeleton.file);
	choiceSkeletonRoot->SetStringSelection(skeleton.rootName);

	wxCheckListBox* dataFileList = XRCCTRL(*parent, "DataFileList", wxCheckListBox);
	wxString dataDir = GameUtil::GetGameDataPath(targ);

	wxDirPickerCtrl* dpGameDataPath = XRCCTRL(*parent, "dpGameDataPath", wxDirPickerCtrl);
	dpGameDataPath->SetPath(dataDir);

	FillDataFileList(dataFileList, dataDir, targ);
	SettingsDialogShared::ShowExtraDataFolders(*wxGetTopLevelParent(choiceTargetGame), targ);
}

int SetSharedConfigDefaults() {
	Config.SetDefaultValue("TargetGame", -1);
	const int currentTarget = Config.GetIntValue("TargetGame");
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

	return currentTarget;
}

void SetGameRegistryDefaults() {
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
}

} // namespace AppSetup
