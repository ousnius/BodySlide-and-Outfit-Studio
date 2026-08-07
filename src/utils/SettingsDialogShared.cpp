/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SettingsDialogShared.h"

#include <wx/choice.h>
#include <wx/clrpicker.h>
#include <wx/checkbox.h>
#include <wx/checklst.h>
#include <wx/filepicker.h>
#include <wx/filename.h>
#include <wx/listbox.h>
#include <wx/textctrl.h>
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>

namespace SettingsDialogShared {

const char* const kAppearanceModeConfigKey = "AppearanceMode";

#ifdef __WXMSW__
wxAppBase::Appearance GetConfiguredAppearance(ConfigurationManager& config) {
	wxString appearanceMode = wxString::FromUTF8(config[kAppearanceModeConfigKey]);
	if (appearanceMode.CmpNoCase("System") == 0)
		return wxAppBase::Appearance::System;
	if (appearanceMode.CmpNoCase("Dark") == 0)
		return wxAppBase::Appearance::Dark;
	return wxAppBase::Appearance::Light;
}
wxAppBase::Appearance GetAppearanceForSelection(int selection) {
	switch (selection) {
		case 0: return wxAppBase::Appearance::System;
		case 2: return wxAppBase::Appearance::Dark;
		default: return wxAppBase::Appearance::Light;
	}
}
#endif

int GetAppearanceChoiceSelection(ConfigurationManager& config) {
	wxString appearanceMode = wxString::FromUTF8(config[kAppearanceModeConfigKey]);
	if (appearanceMode.CmpNoCase("System") == 0)
		return 0;
	if (appearanceMode.CmpNoCase("Dark") == 0)
		return 2;
	return 1;
}

const char* GetAppearanceConfigValueForSelection(int selection) {
	switch (selection) {
		case 0: return "System";
		case 2: return "Dark";
		default: return "Light";
	}
}

void SetDefaultAppearanceMode(ConfigurationManager& config) {
	config.SetDefaultValue(kAppearanceModeConfigKey, "Light");
}

void InitCommonSettingsDialog(
	wxDialog& settings,
	ConfigurationManager& config,
	ConfigurationManager& appConfig,
	const wxLanguage* supportedLangs,
	size_t supportedLangCount,
	CommonSettingsDialogControls& controls) {
	controls.choiceTargetGame = XRCCTRL(settings, "choiceTargetGame", wxChoice);
	controls.choiceTargetGame->Select(config.GetIntValue("TargetGame"));

	controls.dpGameDataPath = XRCCTRL(settings, "dpGameDataPath", wxDirPickerCtrl);
	controls.dpGameDataPath->SetPath(wxString::FromUTF8(config["GameDataPath"]));

	controls.dpOutputPath = XRCCTRL(settings, "dpOutputPath", wxDirPickerCtrl);
	controls.dpOutputPath->SetPath(wxString::FromUTF8(config["OutputDataPath"]));
	if (wxTextCtrl* outputPathText = controls.dpOutputPath->GetTextCtrl())
		outputPathText->SetHint(_("Optional (uses Game Data Path if empty)"));

	controls.dpProjectPath = XRCCTRL(settings, "dpProjectPath", wxDirPickerCtrl);
	controls.dpProjectPath->SetPath(wxString::FromUTF8(config["ProjectPath"]));
	if (wxTextCtrl* projectPathText = controls.dpProjectPath->GetTextCtrl())
		projectPathText->SetHint(_("Optional (uses executable directory if empty)"));

	controls.cbShowForceBodyNormals = XRCCTRL(settings, "cbShowForceBodyNormals", wxCheckBox);
	controls.cbShowForceBodyNormals->SetValue(config.GetBoolValue("ShowForceBodyNormals"));

	controls.cbBSATextures = XRCCTRL(settings, "cbBSATextures", wxCheckBox);
	controls.cbBSATextures->SetValue(config.GetBoolValue("BSATextureScan"));

	controls.cbLeftMousePan = XRCCTRL(settings, "cbLeftMousePan", wxCheckBox);
	controls.cbLeftMousePan->SetValue(config.GetBoolValue("Input/LeftMousePan"));

	controls.cbBrushSettingsNearCursor = XRCCTRL(settings, "cbBrushSettingsNearCursor", wxCheckBox);
	controls.cbBrushSettingsNearCursor->SetValue(config.GetBoolValue("Input/BrushSettingsNearCursor"));

	controls.cbMaskHistory = XRCCTRL(settings, "cbMaskHistory", wxCheckBox);
	controls.cbMaskHistory->SetValue(config.GetBoolValue("Input/MaskHistory"));

	controls.cbShapeHoverHighlight = XRCCTRL(settings, "cbShapeHoverHighlight", wxCheckBox);
	controls.cbShapeHoverHighlight->SetValue(config.GetBoolValue("Input/ShapeHoverHighlight"));

	controls.choiceLanguage = XRCCTRL(settings, "choiceLanguage", wxChoice);
	controls.choiceLanguage->Clear();
	for (size_t i = 0; i < supportedLangCount; i++)
		controls.choiceLanguage->AppendString(wxLocale::GetLanguageName(supportedLangs[i]));
	if (!controls.choiceLanguage->SetStringSelection(wxLocale::GetLanguageName(config.GetIntValue("Language"))))
		controls.choiceLanguage->SetStringSelection("English");

	controls.choiceAppearance = XRCCTRL(settings, "choiceAppearance", wxChoice);
	controls.choiceAppearance->SetSelection(GetAppearanceChoiceSelection(config));

	controls.cbPerspectiveView = XRCCTRL(settings, "cbPerspectiveView", wxCheckBox);
	controls.cbPerspectiveView->SetValue(appConfig.GetBoolValue("Rendering/PerspectiveView", true));

	controls.cpColorBackground = XRCCTRL(settings, "cpColorBackground", wxColourPickerCtrl);
	if (config.Exists("Rendering/ColorBackground")) {
		int colorR = config.GetIntValue("Rendering/ColorBackground.r");
		int colorG = config.GetIntValue("Rendering/ColorBackground.g");
		int colorB = config.GetIntValue("Rendering/ColorBackground.b");
		controls.cpColorBackground->SetColour(wxColour(colorR, colorG, colorB));
	}

	controls.cpColorWire = XRCCTRL(settings, "cpColorWire", wxColourPickerCtrl);
	if (config.Exists("Rendering/ColorWire")) {
		int colorR = config.GetIntValue("Rendering/ColorWire.r");
		int colorG = config.GetIntValue("Rendering/ColorWire.g");
		int colorB = config.GetIntValue("Rendering/ColorWire.b");
		controls.cpColorWire->SetColour(wxColour(colorR, colorG, colorB));
	}

	controls.cpColorPoints = XRCCTRL(settings, "cpColorPoints", wxColourPickerCtrl);
	if (config.Exists("Rendering/ColorPoints")) {
		int colorR = config.GetIntValue("Rendering/ColorPoints.r");
		int colorG = config.GetIntValue("Rendering/ColorPoints.g");
		int colorB = config.GetIntValue("Rendering/ColorPoints.b");
		controls.cpColorPoints->SetColour(wxColour(colorR, colorG, colorB));
	}

	controls.cpColorPointsMasked = XRCCTRL(settings, "cpColorPointsMasked", wxColourPickerCtrl);
	if (config.Exists("Rendering/ColorPointsMasked")) {
		int colorR = config.GetIntValue("Rendering/ColorPointsMasked.r");
		int colorG = config.GetIntValue("Rendering/ColorPointsMasked.g");
		int colorB = config.GetIntValue("Rendering/ColorPointsMasked.b");
		controls.cpColorPointsMasked->SetColour(wxColour(colorR, colorG, colorB));
	}

	controls.fpSkeletonFile = XRCCTRL(settings, "fpSkeletonFile", wxFilePickerCtrl);
	controls.fpSkeletonFile->SetPath(wxString::FromUTF8(config["Anim/DefaultSkeletonReference"]));

	controls.choiceSkeletonRoot = XRCCTRL(settings, "choiceSkeletonRoot", wxChoice);
	controls.choiceSkeletonRoot->SetStringSelection(config["Anim/SkeletonRootName"]);

	controls.dataFileList = XRCCTRL(settings, "DataFileList", wxCheckListBox);
}

void SaveCommonSettingsDialog(
	ConfigurationManager& config,
	ConfigurationManager& appConfig,
	const wxString* targetGames,
	size_t targetGameCount,
	const wxLanguage* supportedLangs,
	size_t supportedLangCount,
	const CommonSettingsDialogControls& controls,
	const std::function<void()>& onLanguageChanged,
	int& outTargetGameSelection,
	bool& outNeedsRestart) {
	outNeedsRestart = false;
	outTargetGameSelection = controls.choiceTargetGame->GetSelection();
	if (outTargetGameSelection < 0)
		outTargetGameSelection = 0;

	config.SetValue("TargetGame", outTargetGameSelection);

	if (!controls.dpGameDataPath->GetPath().IsEmpty()) {
		wxFileName gameDataDir = controls.dpGameDataPath->GetDirName();
		config.SetValue("GameDataPath", gameDataDir.GetFullPath().ToUTF8().data());
		if ((size_t)outTargetGameSelection < targetGameCount)
			config.SetValue("GameDataPaths/" + targetGames[outTargetGameSelection].ToStdString(), gameDataDir.GetFullPath().ToUTF8().data());
	}

	wxFileName outputDataDir = controls.dpOutputPath->GetDirName();
	config.SetValue("OutputDataPath", outputDataDir.GetFullPath().ToUTF8().data());

	wxFileName projectDir = controls.dpProjectPath->GetDirName();
	config.SetValue("ProjectPath", projectDir.GetFullPath().ToUTF8().data());

	wxString selectedfiles;
	for (uint32_t i = 0; i < controls.dataFileList->GetCount(); i++) {
		if (!controls.dataFileList->IsChecked(i))
			selectedfiles += controls.dataFileList->GetString(i) + "; ";
	}
	selectedfiles = selectedfiles.BeforeLast(';');
	if ((size_t)outTargetGameSelection < targetGameCount)
		config.SetValue("GameDataFiles/" + targetGames[outTargetGameSelection].ToStdString(), selectedfiles.ToUTF8().data());

	config.SetBoolValue("ShowForceBodyNormals", controls.cbShowForceBodyNormals->IsChecked());
	config.SetBoolValue("BSATextureScan", controls.cbBSATextures->IsChecked());
	config.SetBoolValue("Input/LeftMousePan", controls.cbLeftMousePan->IsChecked());
	config.SetBoolValue("Input/BrushSettingsNearCursor", controls.cbBrushSettingsNearCursor->IsChecked());
	config.SetBoolValue("Input/MaskHistory", controls.cbMaskHistory->IsChecked());
	config.SetBoolValue("Input/ShapeHoverHighlight", controls.cbShapeHoverHighlight->IsChecked());

	int oldLang = config.GetIntValue("Language");
	int selection = controls.choiceLanguage->GetSelection();
	if (selection >= 0 && (size_t)selection < supportedLangCount) {
		int newLang = supportedLangs[selection];
		if (oldLang != newLang) {
			outNeedsRestart = true;
			config.SetValue("Language", newLang);
			if (onLanguageChanged)
				onLanguageChanged();
		}
	}

	wxString oldAppearance = wxString::FromUTF8(config[kAppearanceModeConfigKey]);
	int appearanceSelection = controls.choiceAppearance->GetSelection();
	wxString newAppearance = wxString::FromUTF8(GetAppearanceConfigValueForSelection(appearanceSelection));
	if (oldAppearance.CmpNoCase(newAppearance) != 0) {
		outNeedsRestart = true;
	}
	config.SetValue(kAppearanceModeConfigKey, GetAppearanceConfigValueForSelection(appearanceSelection));

	appConfig.SetBoolValue("Rendering/PerspectiveView", controls.cbPerspectiveView->IsChecked());

	wxColour colorBackground = controls.cpColorBackground->GetColour();
	config.SetValue("Rendering/ColorBackground.r", colorBackground.Red());
	config.SetValue("Rendering/ColorBackground.g", colorBackground.Green());
	config.SetValue("Rendering/ColorBackground.b", colorBackground.Blue());

	wxColour colorWire = controls.cpColorWire->GetColour();
	config.SetValue("Rendering/ColorWire.r", colorWire.Red());
	config.SetValue("Rendering/ColorWire.g", colorWire.Green());
	config.SetValue("Rendering/ColorWire.b", colorWire.Blue());

	wxColour colorPoints = controls.cpColorPoints->GetColour();
	config.SetValue("Rendering/ColorPoints.r", colorPoints.Red());
	config.SetValue("Rendering/ColorPoints.g", colorPoints.Green());
	config.SetValue("Rendering/ColorPoints.b", colorPoints.Blue());

	wxColour colorPointsMasked = controls.cpColorPointsMasked->GetColour();
	config.SetValue("Rendering/ColorPointsMasked.r", colorPointsMasked.Red());
	config.SetValue("Rendering/ColorPointsMasked.g", colorPointsMasked.Green());
	config.SetValue("Rendering/ColorPointsMasked.b", colorPointsMasked.Blue());

	wxFileName skeletonFile = controls.fpSkeletonFile->GetFileName();
	config.SetValue("Anim/DefaultSkeletonReference", skeletonFile.GetFullPath().ToUTF8().data());
	config.SetValue("Anim/SkeletonRootName", controls.choiceSkeletonRoot->GetStringSelection().ToUTF8().data());
}

}
