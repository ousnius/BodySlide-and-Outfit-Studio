/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Outfit Studio window: packing projects into a folder or archive

#include "OutfitStudioFrame.h"
#include "OutfitStudioApp.h"
#include "utils/TargetGame.h"
#include "components/SliderPresets.h"
#include "files/HkxFile.h"
#include "files/TriFile.h"
#include "ui/GroupManager.h"
#include "utils/ProjectUtil.h"
#include "utils/StringStuff.h"

#include <cmath>
#include <wx/zipstrm.h>
#include <cstdlib>
#include <iterator>
#include <sstream>
#include <wx/dir.h>
#include <wx/textctrl.h>
#include <wx/wfstream.h>

using namespace nifly;

namespace {

// A packed variant or add-on refers to its base project in the merged file when that's packed along.
// Otherwise the base project has to be installed already, which is noted.
void PackBaseProject(SliderSet& set, const std::set<std::string>& packedProjects, const std::string& mergedFile, std::set<std::string>& missingBaseProjects) {
	if (!set.HasBaseProject())
		return;

	const std::string packedFile = "SliderSets" + std::string(PathSepStr) + mergedFile;
	if (packedProjects.find(set.GetBaseProjectName()) != packedProjects.end())
		set.SetBaseProject(packedFile, set.GetBaseProjectName(), set.GetBaseProjectID());
	else
		missingBaseProjects.insert(set.GetBaseProjectName());

	// Other base projects are optional, the ones that are packed too are in the merged file. An entry with the base
	// project's name is a copy of it in another file, e.g. in RefTemplates, which keeps its path.
	std::vector<BaseProjectRef> otherBases = set.GetOtherBaseProjects();
	for (auto& otherBase : otherBases)
		if (otherBase.name != set.GetBaseProjectName() && packedProjects.find(otherBase.name) != packedProjects.end())
			otherBase.file = packedFile;
	set.SetOtherBaseProjects(otherBases);
}

} // namespace

bool OutfitStudioFrame::CopyStreamData(wxInputStream& inputStream, wxOutputStream& outputStream, wxFileOffset size) {
	auto buf = std::make_unique<wxChar[]>(128 * 1024);
	int readSize = 128 * 1024;
	wxFileOffset copiedData = 0;

	for (;;) {
		if (size != -1 && copiedData + readSize > size)
			readSize = size - copiedData;

		inputStream.Read(buf.get(), readSize);

		size_t actuallyRead = inputStream.LastRead();
		outputStream.Write(buf.get(), actuallyRead);
		if (outputStream.LastWrite() != actuallyRead) {
			wxLogError("Failed to output data when copying stream.");
			return false;
		}

		if (size == -1) {
			if (inputStream.Eof())
				break;
		}
		else {
			copiedData += actuallyRead;
			if (copiedData >= size)
				break;
		}
	}

	return true;
}

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

		packFolder->Bind(wxEVT_BUTTON, [&](wxCommandEvent& WXUNUSED(event)) {
			wxString dir = wxDirSelector(_("Packing projects to folder..."), wxEmptyString, wxDD_DEFAULT_STYLE, wxDefaultPosition, packProjects);
			if (dir.IsEmpty())
				return;

			wxLogMessage("Packing project to folder...");
			StartProgress(_("Packing projects to folder..."));

			std::string mergedFile{wxString(mergedFileName->GetValue() + ".osp").ToUTF8()};
			std::string mergedFilePath{wxFileName::CreateTempFileName("os").ToUTF8()};
			project->ReplaceForbidden(mergedFile);

			SliderSetFile projectFile;
			projectFile.New(mergedFilePath);
			std::set<std::string> missingBaseProjects;

			for (auto& setName : selectedProjects) {
				if (projectSources.find(setName) == projectSources.end())
					continue;

				SliderSet set = projectSources[setName];
				PackBaseProject(set, selectedProjects, mergedFile, missingBaseProjects);
				projectFile.UpdateSet(set);

				// Add input file to folder
				wxString inputFilePath = wxString::FromUTF8(ProjectUtil::GetProjectPath() + sep + "ShapeData" + sep + set.GetInputFileName());
				wxFileInputStream inputFileStream(inputFilePath);
				if (!inputFileStream.IsOk()) {
					wxLogError("Failed to open input file '%s'!", inputFilePath);
					wxMessageBox(wxString::Format(_("Failed to open input file '%s'!"), inputFilePath), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}

				// Copy input file to destination folder
				wxString inputFileDest = wxString::FromUTF8(dir + sep + baseDir + sep + "ShapeData" + sep + set.GetInputFileName());
				wxFileName::Mkdir(inputFileDest.BeforeLast(wxFileName::GetPathSeparator()), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

				if (!wxCopyFile(inputFilePath, inputFileDest)) {
					wxLogError("Failed to copy input file '%s'!", inputFilePath);
					wxMessageBox(wxString::Format(_("Failed to copy input file '%s'!"), inputFilePath), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}

				std::set<std::string> dataFiles;

				for (size_t i = 0; i < set.size(); i++) {
					for (auto it = set.ShapesBegin(); it != set.ShapesEnd(); ++it) {
						std::string target = set.ShapeToTarget(it->first);
						std::string targetDataName = set[i].TargetDataName(target);
						if (set[i].IsLocalData(targetDataName) && !set[i].IsInheritedData(targetDataName)) {
							std::string dataFileName = set[i].DataFileName(targetDataName);
							if (dataFileName.compare(dataFileName.size() - 4, dataFileName.size(), ".bsd") != 0) {
								// Split target file name to get OSD file name
								int split = dataFileName.find_last_of('/');
								if (split < 0)
									split = dataFileName.find_last_of('\\');
								if (split < 0)
									continue;

								dataFiles.insert(set.GetDefaultDataFolder() + sep + dataFileName.substr(0, split));
							}
							else {
								dataFiles.insert(set.GetDefaultDataFolder() + sep + dataFileName);
							}
						}
					}
				}

				// Add data files to folder
				for (auto& df : dataFiles) {
					wxString dataFilePath = wxString::FromUTF8(ProjectUtil::GetProjectPath() + sep + "ShapeData" + sep + df);
					wxFileInputStream dataFileStream(dataFilePath);
					if (!dataFileStream.IsOk()) {
						wxLogError("Failed to open input file '%s'!", dataFilePath);
						wxMessageBox(wxString::Format(_("Failed to open input file '%s'!"), dataFilePath), _("Error"), wxICON_ERROR);
						EndProgress();
						return;
					}

					// Copy data file to destination folder
					wxString dataFileDest = wxString::FromUTF8(dir + sep + baseDir + sep + "ShapeData" + sep + df);
					wxFileName::Mkdir(dataFileDest.BeforeLast(wxFileName::GetPathSeparator()), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

					if (!wxCopyFile(dataFilePath, dataFileDest)) {
						wxLogError("Failed to copy data file '%s'!", dataFilePath);
						wxMessageBox(wxString::Format(_("Failed to copy data file '%s'!"), dataFilePath), _("Error"), wxICON_ERROR);
						EndProgress();
						return;
					}
				}
			}

			// Save new merged project file
			if (!projectFile.Save()) {
				wxLogError("Failed to save merged project file '%s'!", mergedFilePath);
				wxMessageBox(wxString::Format(_("Failed to save merged project file '%s'!"), mergedFilePath), _("Error"), wxICON_ERROR);
				EndProgress();
				return;
			}

			// Add merged project file to folder
			wxFileInputStream projectFileStream(mergedFilePath);
			if (!projectFileStream.IsOk()) {
				wxLogError("Failed to open project file '%s'!", mergedFilePath);
				wxMessageBox(wxString::Format(_("Failed to open project file '%s'!"), mergedFilePath), _("Error"), wxICON_ERROR);
				EndProgress();
				return;
			}

			// Copy merged project file to destination folder
			wxString projectFileDest = wxString::FromUTF8(dir + sep + baseDir + sep + "SliderSets" + sep + mergedFile);
			wxFileName::Mkdir(projectFileDest.BeforeLast(wxFileName::GetPathSeparator()), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

			if (!wxCopyFile(mergedFilePath, projectFileDest)) {
				wxLogError("Failed to copy merged project file '%s'!", mergedFilePath);
				wxMessageBox(wxString::Format(_("Failed to copy merged project file '%s'!"), mergedFilePath), _("Error"), wxICON_ERROR);
				EndProgress();
				return;
			}

			wxString groupFilePath = groupFile->GetPath();
			if (!groupFilePath.IsEmpty()) {
				// Add group file to folder
				wxFileInputStream groupFileStream(groupFilePath);
				if (!groupFileStream.IsOk()) {
					wxLogError("Failed to open group file '%s'!", groupFilePath);
					wxMessageBox(wxString::Format(_("Failed to open group file '%s'!"), groupFilePath), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}

				// Copy group file to destination folder
				std::string groupFileName{groupFilePath.AfterLast(wxFileName::GetPathSeparator()).ToUTF8()};
				wxString groupFileDest = wxString::FromUTF8(dir + sep + baseDir + sep + "SliderGroups" + sep + groupFileName);
				wxFileName::Mkdir(groupFileDest.BeforeLast(wxFileName::GetPathSeparator()), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

				if (!wxCopyFile(groupFilePath, groupFileDest)) {
					wxLogError("Failed to copy group file '%s'!", groupFilePath);
					wxMessageBox(wxString::Format(_("Failed to copy group file '%s'!"), groupFilePath), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}
			}

			if (!missingBaseProjects.empty())
				wxMessageBox(wxString::Format(_("These base projects of packed variants or add-ons aren't in the pack, so they have to be installed for them: %s"),
											  wxString::FromUTF8(JoinStrings(std::vector<std::string>(missingBaseProjects.begin(), missingBaseProjects.end()), ", "))),
							 _("Pack Projects"),
							 wxOK | wxICON_INFORMATION,
							 packProjects);

			EndProgress(_("Packing finished."));
		});

		packArchive->Bind(wxEVT_BUTTON, [&](wxCommandEvent& WXUNUSED(event)) {
			wxString fileName = wxFileSelector(_("Packing projects to archive..."), wxEmptyString, wxEmptyString, ".zip", "*.zip", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, packProjects);
			if (fileName.IsEmpty())
				return;

			wxLogMessage("Packing project to archive...");
			StartProgress(_("Packing projects to archive..."));

			std::string mergedFile{wxString(mergedFileName->GetValue() + ".osp").ToUTF8()};
			std::string mergedFilePath{wxFileName::CreateTempFileName("os").ToUTF8()};
			project->ReplaceForbidden(mergedFile);

			SliderSetFile projectFile;
			projectFile.New(mergedFilePath);
			std::set<std::string> missingBaseProjects;

			wxFFileOutputStream out(fileName);
			wxZipOutputStream zip(out);

			for (auto& setName : selectedProjects) {
				if (projectSources.find(setName) == projectSources.end())
					continue;

				SliderSet set = projectSources[setName];
				PackBaseProject(set, selectedProjects, mergedFile, missingBaseProjects);
				projectFile.UpdateSet(set);

				// Add input file to archive
				wxString inputFilePath = wxString::FromUTF8(ProjectUtil::GetProjectPath() + sep + "ShapeData" + sep + set.GetInputFileName());
				wxFileInputStream inputFileStream(inputFilePath);
				if (!inputFileStream.IsOk()) {
					wxLogError("Failed to open input file '%s'!", inputFilePath);
					wxMessageBox(wxString::Format(_("Failed to open input file '%s'!"), inputFilePath), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}

				wxString inputFileEntry = wxString::FromUTF8(baseDir + sep + "ShapeData" + sep + set.GetInputFileName());
				if (!zip.PutNextEntry(inputFileEntry, wxDateTime::Now(), inputFileStream.GetLength())) {
					wxLogError("Failed to put new entry into archive!");
					wxMessageBox(_("Failed to put new entry into archive!"), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}

				if (!CopyStreamData(inputFileStream, zip, inputFileStream.GetLength())) {
					wxLogError("Failed to copy file contents to archive!");
					wxMessageBox(_("Failed to copy file contents to archive!"), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}

				std::set<std::string> dataFiles;

				for (size_t i = 0; i < set.size(); i++) {
					for (auto it = set.ShapesBegin(); it != set.ShapesEnd(); ++it) {
						std::string target = set.ShapeToTarget(it->first);
						std::string targetDataName = set[i].TargetDataName(target);
						if (set[i].IsLocalData(targetDataName) && !set[i].IsInheritedData(targetDataName)) {
							std::string dataFileName = set[i].DataFileName(targetDataName);
							if (dataFileName.compare(dataFileName.size() - 4, dataFileName.size(), ".bsd") != 0) {
								// Split target file name to get OSD file name
								int split = dataFileName.find_last_of('/');
								if (split < 0)
									split = dataFileName.find_last_of('\\');
								if (split < 0)
									continue;

								dataFiles.insert(set.GetDefaultDataFolder() + sep + dataFileName.substr(0, split));
							}
							else {
								dataFiles.insert(set.GetDefaultDataFolder() + sep + dataFileName);
							}
						}
					}
				}

				// Add data files to archive
				for (auto& df : dataFiles) {
					wxString dataFilePath = wxString::FromUTF8(ProjectUtil::GetProjectPath() + sep + "ShapeData" + sep + df);
					wxFileInputStream dataFileStream(dataFilePath);
					if (!dataFileStream.IsOk()) {
						wxLogError("Failed to open data file '%s'!", dataFilePath);
						wxMessageBox(wxString::Format(_("Failed to open data file '%s'!"), dataFilePath), _("Error"), wxICON_ERROR);
						EndProgress();
						return;
					}

					wxString dataFileEntry = wxString::FromUTF8(baseDir + sep + "ShapeData" + sep + df);
					if (!zip.PutNextEntry(dataFileEntry, wxDateTime::Now(), dataFileStream.GetLength())) {
						wxLogError("Failed to put new entry into archive!");
						wxMessageBox(_("Failed to put new entry into archive!"), _("Error"), wxICON_ERROR);
						EndProgress();
						return;
					}

					if (!CopyStreamData(dataFileStream, zip, dataFileStream.GetLength())) {
						wxLogError("Failed to copy file contents to archive!");
						wxMessageBox(_("Failed to copy file contents to archive!"), _("Error"), wxICON_ERROR);
						EndProgress();
						return;
					}
				}
			}

			// Save new merged project file
			if (!projectFile.Save()) {
				wxLogError("Failed to save merged project file '%s'!", mergedFilePath);
				wxMessageBox(wxString::Format(_("Failed to save merged project file '%s'!"), mergedFilePath), _("Error"), wxICON_ERROR);
				EndProgress();
				return;
			}

			// Add merged project file to archive
			wxFileInputStream projectFileStream(mergedFilePath);
			if (!projectFileStream.IsOk()) {
				wxLogError("Failed to open project file '%s'!", mergedFilePath);
				wxMessageBox(wxString::Format(_("Failed to open project file '%s'!"), mergedFilePath), _("Error"), wxICON_ERROR);
				EndProgress();
				return;
			}

			wxString projectFileEntry = wxString::FromUTF8(baseDir + sep + "SliderSets" + sep + mergedFile);
			if (!zip.PutNextEntry(projectFileEntry, wxDateTime::Now(), projectFileStream.GetLength())) {
				wxLogError("Failed to put new entry into archive!");
				wxMessageBox(_("Failed to put new entry into archive!"), _("Error"), wxICON_ERROR);
				EndProgress();
				return;
			}

			if (!CopyStreamData(projectFileStream, zip, projectFileStream.GetLength())) {
				wxLogError("Failed to copy file contents to archive!");
				wxMessageBox(_("Failed to copy file contents to archive!"), _("Error"), wxICON_ERROR);
				EndProgress();
				return;
			}

			wxString groupFilePath = groupFile->GetPath();
			if (!groupFilePath.IsEmpty()) {
				// Add group file to archive
				wxFileInputStream groupFileStream(groupFilePath);
				if (!groupFileStream.IsOk()) {
					wxLogError("Failed to open group file '%s'!", groupFilePath);
					wxMessageBox(wxString::Format(_("Failed to open group file '%s'!"), groupFilePath), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}

				std::string groupFileName{groupFilePath.AfterLast(wxFileName::GetPathSeparator()).ToUTF8()};
				wxString groupFileEntry = wxString::FromUTF8(baseDir + sep + "SliderGroups" + sep + groupFileName);
				if (!zip.PutNextEntry(groupFileEntry, wxDateTime::Now(), groupFileStream.GetLength())) {
					wxLogError("Failed to put new entry into archive!");
					wxMessageBox(_("Failed to put new entry into archive!"), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}

				if (!CopyStreamData(groupFileStream, zip, groupFileStream.GetLength())) {
					wxLogError("Failed to copy file contents to archive!");
					wxMessageBox(_("Failed to copy file contents to archive!"), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}
			}

			if (!missingBaseProjects.empty())
				wxMessageBox(wxString::Format(_("These base projects of packed variants or add-ons aren't in the pack, so they have to be installed for them: %s"),
											  wxString::FromUTF8(JoinStrings(std::vector<std::string>(missingBaseProjects.begin(), missingBaseProjects.end()), ", "))),
							 _("Pack Projects"),
							 wxOK | wxICON_INFORMATION,
							 packProjects);

			EndProgress(_("Packing finished."));
		});

		packProjects->ShowModal();
	}
}
