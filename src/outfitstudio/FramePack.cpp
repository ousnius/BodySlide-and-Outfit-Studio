/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Outfit Studio window: the Pack Projects dialog, see ProjectPacker for the packing

#include "OutfitStudioFrame.h"
#include "OutfitStudioApp.h"
#include "project/ProjectPacker.h"
#include "ui/GroupManager.h"
#include "utils/ProjectUtil.h"
#include "utils/StringStuff.h"

#include <algorithm>
#include <iterator>
#include <wx/checklst.h>
#include <wx/dir.h>
#include <wx/filepicker.h>
#include <wx/srchctrl.h>
#include <wx/textctrl.h>

using namespace nifly;

void OutfitStudioFrame::OnPackProjects(wxCommandEvent& WXUNUSED(event)) {
	CloseBrushSettings();

	wxXmlResource* xrc = wxXmlResource::Get();
	wxDialog* packProjects = xrc->LoadDialog(this, "dlgPackProjects");
	if (packProjects) {
		auto projectFilter = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, FromDIP(wxSize(200, -1)), wxTE_PROCESS_ENTER);
		projectFilter->ShowSearchButton(true);
		projectFilter->SetDescriptiveText("Project Filter");
		projectFilter->SetToolTip("Filter project list by name");

		xrc->AttachUnknownControl("projectFilter", projectFilter, packProjects);

		packProjects->SetSize(FromDIP(wxSize(550, 300)));
		packProjects->SetMinSize(FromDIP(wxSize(400, 200)));
		packProjects->CenterOnParent();

		std::map<std::string, SliderSet> projectSources;
		std::set<std::string> selectedProjects;

		auto projectList = XRCCTRL(*packProjects, "projectList", wxCheckListBox);
		projectList->Bind(wxEVT_RIGHT_UP, [&](wxMouseEvent& WXUNUSED(event)) {
			wxMenu* menu = wxXmlResource::Get()->LoadMenu("projectListContext");
			if (menu) {
				menu->Bind(wxEVT_MENU, [&](wxCommandEvent& event) {
					if (event.GetId() == XRCID("projectListNone")) {
						for (uint32_t i = 0; i < projectList->GetCount(); i++) {
							std::string name{projectList->GetString(i).ToUTF8()};
							projectList->Check(i, false);
							selectedProjects.erase(name);
						}
					}
					else if (event.GetId() == XRCID("projectListAll")) {
						for (uint32_t i = 0; i < projectList->GetCount(); i++) {
							std::string name{projectList->GetString(i).ToUTF8()};
							projectList->Check(i);
							selectedProjects.insert(name);
						}
					}
					else if (event.GetId() == XRCID("projectListInvert")) {
						for (uint32_t i = 0; i < projectList->GetCount(); i++) {
							std::string name{projectList->GetString(i).ToUTF8()};

							bool check = !projectList->IsChecked(i);
							projectList->Check(i, check);

							if (check)
								selectedProjects.insert(name);
							else
								selectedProjects.erase(name);
						}
					}
				});

				PopupMenu(menu);
				delete menu;
			}
		});

		projectList->Bind(wxEVT_CHECKLISTBOX, [&](wxCommandEvent& event) {
			std::string name{event.GetString().ToUTF8()};
			int item = event.GetInt();
			if (projectList->IsChecked(item))
				selectedProjects.insert(name);
			else
				selectedProjects.erase(name);
		});

		projectFilter->Bind(wxEVT_TEXT, [&](wxCommandEvent& event) {
			wxString filterStr = event.GetString();
			filterStr.MakeLower();

			projectList->Clear();

			// Add outfits that are no members to list
			for (auto& project : projectSources) {
				// Filter outfit by name
				wxString projectStr = wxString::FromUTF8(project.first);
				if (projectStr.Lower().Contains(filterStr)) {
					int item = projectList->Append(projectStr);
					if (selectedProjects.find(project.first) != selectedProjects.end())
						projectList->Check(item);
				}
			}
		});

		wxArrayString files;
		wxDir::GetAllFiles(wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderSets", &files, "*.osp");
		wxDir::GetAllFiles(wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderSets", &files, "*.xml");

		for (auto& file : files) {
			std::string fileName{file.ToUTF8()};

			SliderSetFile sliderDoc;
			sliderDoc.Open(fileName);
			if (sliderDoc.fail())
				continue;

			std::vector<std::string> setNames;
			sliderDoc.GetSetNamesUnsorted(setNames, false);

			for (auto& setName : setNames) {
				if (projectSources.find(setName) != projectSources.end())
					continue;

				SliderSet set;
				if (sliderDoc.GetSet(setName, set) == 0) {
					projectSources[setName] = set;
					projectList->Append(wxString::FromUTF8(setName));
				}
			}
		}

		std::string sep{wxString(wxFileName::GetPathSeparator()).ToUTF8()};
		wxString baseDir = "Tools" + sep + "BodySlide";

		TargetGame targetGame = wxGetApp().targetGame;
		if (targetGame == SKYRIM || targetGame == SKYRIMSE || targetGame == SKYRIMVR)
			baseDir = "CalienteTools" + sep + "BodySlide";

		auto groupManager = XRCCTRL(*packProjects, "groupManager", wxButton);
		groupManager->Bind(wxEVT_BUTTON, [&](wxCommandEvent& WXUNUSED(event)) {
			std::vector<std::string> gmOutfits;
			gmOutfits.reserve(projectSources.size());

			std::transform(std::begin(projectSources), std::end(projectSources), std::back_inserter(gmOutfits), [](auto const& pair) { return pair.first; });

			GroupManager gm(packProjects, gmOutfits);
			gm.ShowModal();
		});

		auto groupFile = XRCCTRL(*packProjects, "groupFile", wxFilePickerCtrl);
		groupFile->SetInitialDirectory(wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderGroups");

		auto mergedFileName = XRCCTRL(*packProjects, "mergedFileName", wxTextCtrl);
		auto packFolder = XRCCTRL(*packProjects, "packFolder", wxButton);
		auto packArchive = XRCCTRL(*packProjects, "packArchive", wxButton);

		mergedFileName->Bind(wxEVT_TEXT, [&](wxCommandEvent& WXUNUSED(event)) {
			packFolder->Enable(!mergedFileName->GetValue().IsEmpty());
			packArchive->Enable(!mergedFileName->GetValue().IsEmpty());
		});

		auto pack = [&](PackTarget& target) {
			std::string mergedFile{wxString(mergedFileName->GetValue() + ".osp").ToUTF8()};
			project->ReplaceForbidden(mergedFile);

			PackResult result = PackProjects(projectSources, selectedProjects, mergedFile, groupFile->GetPath(), std::string{baseDir.ToUTF8()}, target);
			if (!result.error.empty()) {
				wxMessageBox(result.error, _("Error"), wxICON_ERROR);
				EndProgress();
				return;
			}

			if (!result.missingBaseProjects.empty())
				wxMessageBox(wxString::Format(_("These base projects of packed variants or add-ons aren't in the pack, so they have to be installed for them: %s"),
											  wxString::FromUTF8(JoinStrings(std::vector<std::string>(result.missingBaseProjects.begin(), result.missingBaseProjects.end()), ", "))),
							 _("Pack Projects"),
							 wxOK | wxICON_INFORMATION,
							 packProjects);

			EndProgress(_("Packing finished."));
		};

		packFolder->Bind(wxEVT_BUTTON, [&](wxCommandEvent& WXUNUSED(event)) {
			wxString dir = wxDirSelector(_("Packing projects to folder..."), wxEmptyString, wxDD_DEFAULT_STYLE, wxDefaultPosition, packProjects);
			if (dir.IsEmpty())
				return;

			wxLogMessage("Packing project to folder...");
			StartProgress(_("Packing projects to folder..."));

			FolderPackTarget target(dir);
			pack(target);
		});

		packArchive->Bind(wxEVT_BUTTON, [&](wxCommandEvent& WXUNUSED(event)) {
			wxString fileName = wxFileSelector(_("Packing projects to archive..."), wxEmptyString, wxEmptyString, ".zip", "*.zip", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, packProjects);
			if (fileName.IsEmpty())
				return;

			wxLogMessage("Packing project to archive...");
			StartProgress(_("Packing projects to archive..."));

			ZipPackTarget target(fileName);
			pack(target);
		});

		packProjects->ShowModal();
	}
}
