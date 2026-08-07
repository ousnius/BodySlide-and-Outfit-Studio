/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "ConfigurationManager.h"

#include <cstddef>
#include <functional>
#include <wx/app.h>

class wxDialog;
class wxChoice;
class wxDirPickerCtrl;
class wxCheckBox;
class wxColourPickerCtrl;
class wxFilePickerCtrl;
class wxCheckListBox;
class wxString;

enum wxLanguage;

namespace SettingsDialogShared {

extern const char* const kAppearanceModeConfigKey;

#ifdef __WXMSW__
wxAppBase::Appearance GetConfiguredAppearance(ConfigurationManager& config);
wxAppBase::Appearance GetAppearanceForSelection(int selection);
#endif
int GetAppearanceChoiceSelection(ConfigurationManager& config);
const char* GetAppearanceConfigValueForSelection(int selection);
void SetDefaultAppearanceMode(ConfigurationManager& config);

struct CommonSettingsDialogControls {
	wxChoice* choiceTargetGame;
	wxDirPickerCtrl* dpGameDataPath;
	wxDirPickerCtrl* dpOutputPath;
	wxDirPickerCtrl* dpProjectPath;
	wxCheckBox* cbShowForceBodyNormals;
	wxCheckBox* cbBSATextures;
	wxCheckBox* cbLeftMousePan;
	wxCheckBox* cbBrushSettingsNearCursor;
	wxCheckBox* cbMaskHistory;
	wxCheckBox* cbShapeHoverHighlight;
	wxChoice* choiceLanguage;
	wxChoice* choiceAppearance;
	wxCheckBox* cbPerspectiveView;
	wxCheckBox* cbComplexMaterial;
	wxColourPickerCtrl* cpColorBackground;
	wxColourPickerCtrl* cpColorWire;
	wxColourPickerCtrl* cpColorPoints;
	wxColourPickerCtrl* cpColorPointsMasked;
	wxFilePickerCtrl* fpSkeletonFile;
	wxChoice* choiceSkeletonRoot;
	wxCheckListBox* dataFileList;
};

void InitCommonSettingsDialog(
	wxDialog& settings,
	ConfigurationManager& config,
	ConfigurationManager& appConfig,
	const wxLanguage* supportedLangs,
	size_t supportedLangCount,
	CommonSettingsDialogControls& controls);

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
	bool& outNeedsRestart);

}
