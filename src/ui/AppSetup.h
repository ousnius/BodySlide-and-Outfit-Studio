/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <array>
#include <wx/intl.h>
#include <wx/string.h>

class wxCheckListBox;
class wxCommandEvent;

// Start-up and game settings that BodySlide and Outfit Studio share
namespace AppSetup {

// The languages the applications can be set to
extern const std::array<wxLanguage, 37> SupportedLangs;

// The first-run dialog that picks the target game and its data folder, and saves them to Config.xml.
// Returns false if it was cancelled.
bool ShowSetupDialog();

// Replaces locale with the language set in the config, or English if that isn't available
void InitLanguage(wxLocale*& locale);

// Fills the archive list of the settings dialog with the archives in dataDir, checked unless the
// game's "GameDataFiles" config turned them off
void FillDataFileList(wxCheckListBox* dataFileList, const wxString& dataDir, int targetGame);

// Choice event of the settings dialog's target game: shows the game's skeleton, data folder and archives
void OnChooseTargetGame(wxCommandEvent& event);

} // namespace AppSetup
