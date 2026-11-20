/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Outfit Studio window: creating, loading, saving and closing projects

#include "OutfitStudioFrame.h"
#include "SegmentPartitionEditor.h"
#include "PhysicsPreview.h"
#include "AnimationPlayer.h"
#include "ui/outfitstudio/EditUV.h"
#include "ShapeTreeItems.h"
#include "GLPanel.h"
#include "OutfitStudioApp.h"
#include "utils/ConfigurationManager.h"
#include "ui/wxStateButton.h"
#include "components/UndoHistory.h"
#include "utils/TargetGame.h"
#include "components/SliderPresets.h"
#include "components/SliderSetIndex.h"
#include "files/HkxFile.h"
#include "files/MaskFile.h"
#include "files/GameDataStream.h"
#include "ui/outfitstudio/ArchiveBrowser.h"
#include "ui/ConfigDialogUtil.h"
#include "utils/PlatformUtil.h"
#include "ui/outfitstudio/AddProjectDialog.h"
#include "ui/outfitstudio/AutomationDialog.h"
#include "utils/ProjectUtil.h"
#include "utils/StringStuff.h"
#include "ui/outfitstudio/ConvertBodyReferenceDialog.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>
#include <wx/textctrl.h>

using namespace nifly;

void OutfitStudioFrame::LoadFiles(const wxArrayString& files, const wxString& projectName) {
	for (auto& f : files) {
		wxFileName loadFile(f);
		if (loadFile.FileExists()) {
			std::string fileName{loadFile.GetFullPath().ToUTF8()};
			wxString fileExt = loadFile.GetExt().MakeLower();
			if (fileExt == "osp") {
				std::string loadProjectName{projectName.ToUTF8()};
				LoadProject(fileName, loadProjectName);
				// only open first project file
				break;
			}
			else if (fileExt == "nif") {
				StartProgress(_("Adding NIF file..."));
				UpdateProgress(1, _("Adding NIF file..."));
				project->ImportNIF(fileName, false);
				project->SetTextures();

				UpdateProgress(60, _("Refreshing GUI..."));
				RefreshGUIFromProj();

				EndProgress();
			}
			else if (fileExt == "obj") {
				StartProgress("Adding OBJ file...");
				UpdateProgress(1, _("Adding OBJ file..."));
				ImportOBJWithDialog(fileName);
				project->SetTextures();

				UpdateProgress(60, _("Refreshing GUI..."));
				RefreshGUIFromProj();

				EndProgress();
			}
			else if (fileExt == "fbx") {
#ifdef USE_FBXSDK
				StartProgress(_("Adding FBX file..."));
				UpdateProgress(1, _("Adding FBX file..."));
				ImportFBXWithDialog(fileName);
				project->SetTextures();

				UpdateProgress(60, _("Refreshing GUI..."));
				RefreshGUIFromProj();

				EndProgress();
#endif
			}
		}
	}
}

bool OutfitStudioFrame::SaveProject() {
	if (project->mFileName.empty())
		return SaveProjectAs();

	if (!project->GetWorkNif()->IsValid()) {
		projectHost.ShowMessage(HostMessageLevel::Error, _("There are no valid shapes loaded!"), _("Error"));
		return false;
	}

	if (HasUnweightedCheck())
		return false;

	wxLogMessage("Saving project '%s'...", wxString::FromUTF8(project->OutfitName()));
	StartProgress(wxString::Format(_("Saving project '%s'..."), wxString::FromUTF8(project->OutfitName())));

	std::vector<Mesh*> shapeMeshes;
	for (auto& s : project->GetWorkNif()->GetShapes()) {
		if (!project->IsBaseShape(s)) {
			Mesh* m = glView->GetMesh(s->name.get());
			if (m)
				shapeMeshes.push_back(m);
		}
	}

	project->UpdateNifNormals(project->GetWorkNif(), shapeMeshes);

	if (projectNotes)
		project->activeSet.SetNotes(projectNotes->GetValue().ToUTF8().data());

	// The files don't change here, so other projects that use them aren't looked for
	std::string error = project->Save(project->mFileName,
									  project->mOutfitName,
									  project->mDataDir,
									  project->mBaseFile,
									  project->mGamePath,
									  project->mGameFile,
									  project->mGenWeights,
									  project->mCopyRef,
									  project->bPreventMorphFile,
									  project->bKeepZappedShapes,
									  project->mSFMorphPath,
									  project->mSFMorphTargetShape,
									  SharedFilesCheck::Skip);

	if (!error.empty()) {
		wxLogError(error.c_str());
		projectHost.ShowMessage(HostMessageLevel::Error, error, _("Error"));
		EndProgress(_("Saving failed."));
		return false;
	}

	SetPendingChanges(false);
	EndProgress(_("Saved."));
	return true;
}

namespace {
// Data folder of the Save Project As dialog, relative to the "ShapeData" folder
wxString SaveAsDataDir(wxDialog& dlg) {
	auto shapeDataFolder = XRCCTRL(dlg, "sssShapeDataFolder", wxDirPickerCtrl);
	wxString dataDir = shapeDataFolder->GetDirName().GetFullName();
	if (dataDir.empty())
		dataDir = shapeDataFolder->GetPath();

	wxFileName relativeFolder(dataDir);
	if (!dataDir.empty() && !relativeFolder.IsRelative()) {
		relativeFolder.MakeRelativeTo(wxString::Format("%s/%s", wxString::FromUTF8(ProjectUtil::GetProjectPath()), "ShapeData"));
		dataDir = relativeFolder.GetFullPath();
	}

	return dataDir;
}
} // namespace

SharedFilesTarget OutfitStudioFrame::SaveAsSharedFilesTarget(wxDialog& dlg, bool hasBase, bool addOn) {
	SharedFilesTarget target;
	target.hasBase = hasBase;
	target.addOn = addOn;
	target.dataDir = SaveAsDataDir(dlg).ToUTF8();

	wxString nifName = XRCCTRL(dlg, "sssShapeDataFile", wxFilePickerCtrl)->GetFileName().GetFullName();
	if (!nifName.EndsWith(".nif"))
		nifName.Append(".nif");
	target.nifName = nifName.ToUTF8();
	project->ReplaceForbidden(target.nifName);

	if (addOn) {
		wxString dataFile = XRCCTRL(dlg, "sssSliderDataFile", wxTextCtrl)->GetValue();
		if (!dataFile.Lower().EndsWith(".osd"))
			dataFile.Append(".osd");
		target.osdName = dataFile.ToUTF8();
		project->ReplaceForbidden(target.osdName);
	}
	else
		target.osdName = target.nifName.substr(0, target.nifName.find_last_of('.')) + ".osd";

	// The project itself isn't one of the others, like when saving
	wxFileName setFile = XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->GetFileName();
	if (setFile.GetExt() != "osp")
		setFile.SetExt("osp");
	if (setFile.GetDirs().Index("SliderSets") == wxNOT_FOUND)
		setFile = wxFileName("SliderSets" + wxString(PathSepStr) + setFile.GetFullName());
	if (setFile.IsRelative())
		setFile.MakeAbsolute(wxString::FromUTF8(ProjectUtil::GetProjectPath()));
	target.setFile = setFile.GetFullPath().ToUTF8();
	target.setName = XRCCTRL(dlg, "sssName", wxTextCtrl)->GetValue().ToUTF8();

	return target;
}

bool OutfitStudioFrame::SaveProjectAs() {
	wxDialog dlg;
	int result = wxID_CANCEL;

	// Looks for other projects that use the files while the dialog is open
	std::unique_ptr<SliderSetIndexTask> sharedFilesTask;

	// Base project chosen or cleared in the dialog
	struct {
		bool changed = false;
		std::string file;
		std::string set;
		bool addOn = false;
		std::string baseFolder;
		std::string baseNif;
	} baseChoice;

	if (!project->GetWorkNif()->IsValid()) {
		projectHost.ShowMessage(HostMessageLevel::Error, _("There are no valid shapes loaded!"), _("Error"));
		return false;
	}

	if (HasUnweightedCheck())
		return false;

	CloseBrushSettings();

	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgSaveProject", "wxDialog")) {
		XRCCTRL(dlg, "sssNameCopy", wxButton)->Bind(wxEVT_BUTTON, &OutfitStudioFrame::OnSSSNameCopy, this);
		XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->Bind(wxEVT_RADIOBUTTON, &OutfitStudioFrame::OnSSSGenWeightsTrue, this);
		XRCCTRL(dlg, "sssGenWeightsFalse", wxRadioButton)->Bind(wxEVT_RADIOBUTTON, &OutfitStudioFrame::OnSSSGenWeightsFalse, this);

		XRCCTRL(dlg, "sssOutputDataPathBrowse", wxButton)->Bind(wxEVT_BUTTON, [&dlg](wxCommandEvent&) {
			wxString dataPath = wxString::FromUTF8(Config["GameDataPath"]);
			wxString result = wxDirSelector(_("Select output data path"), dataPath, 0, wxDefaultPosition, &dlg);
			if (!result.empty()) {
				wxFileName relPath(result);
				if (!dataPath.empty())
					relPath.MakeRelativeTo(dataPath);
				XRCCTRL(dlg, "sssOutputDataPath", wxTextCtrl)->ChangeValue(relPath.GetFullPath());
			}
		});

		XRCCTRL(dlg, "sssSFMorphPathBrowse", wxButton)->Bind(wxEVT_BUTTON, [&dlg](wxCommandEvent&) {
			wxString dataPath = wxString::FromUTF8(Config["GameDataPath"]);
			wxString result = wxDirSelector(_("Select morph.dat output folder"), dataPath, 0, wxDefaultPosition, &dlg);
			if (!result.empty()) {
				wxFileName relPath(result);
				if (!dataPath.empty())
					relPath.MakeRelativeTo(dataPath);
				XRCCTRL(dlg, "sssSFMorphPath", wxTextCtrl)->ChangeValue(relPath.GetFullPath());
			}
		});

		std::string outName;
		if (!project->mOutfitName.empty())
			outName = project->mOutfitName.ToUTF8();
		else if (!project->OutfitName().empty())
			outName = project->OutfitName();
		else
			outName = "New Outfit";

		project->ReplaceForbidden(outName);

		wxString sssName = wxString::FromUTF8(outName);

		XRCCTRL(dlg, "sssName", wxTextCtrl)->SetValue(sssName);
		XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->SetInitialDirectory(wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderSets");

		if (!project->mFileName.empty())
			XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->SetPath(project->mFileName);
		else
			XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->SetPath(sssName + ".osp");

		if (!project->mDataDir.empty())
			XRCCTRL(dlg, "sssShapeDataFolder", wxDirPickerCtrl)->SetPath(project->mDataDir);
		else
			XRCCTRL(dlg, "sssShapeDataFolder", wxDirPickerCtrl)->SetPath(sssName);

		if (!project->mBaseFile.empty())
			XRCCTRL(dlg, "sssShapeDataFile", wxFilePickerCtrl)->SetPath(project->mBaseFile);
		else
			XRCCTRL(dlg, "sssShapeDataFile", wxFilePickerCtrl)->SetPath(sssName + ".nif");

		if (!project->mGamePath.empty())
			XRCCTRL(dlg, "sssOutputDataPath", wxTextCtrl)->ChangeValue(project->mGamePath);
		else
			XRCCTRL(dlg, "sssOutputDataPath", wxTextCtrl)->ChangeValue(wxString::Format("meshes%carmor%c%s", PathSepChar, PathSepChar, sssName));

		if (!project->mGameFile.empty())
			XRCCTRL(dlg, "sssOutputFileName", wxTextCtrl)->ChangeValue(project->mGameFile);
		else
			XRCCTRL(dlg, "sssOutputFileName", wxTextCtrl)->ChangeValue(sssName);

		if (project->mGenWeights) {
			XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->SetValue(true);
			XRCCTRL(dlg, "sssGenWeightsFalse", wxRadioButton)->SetValue(false);
		}
		else {
			XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->SetValue(false);
			XRCCTRL(dlg, "sssGenWeightsFalse", wxRadioButton)->SetValue(true);
		}

		XRCCTRL(dlg, "sssPreventMorphFile", wxCheckBox)->SetValue(project->bPreventMorphFile);
		XRCCTRL(dlg, "sssKeepZappedShapes", wxCheckBox)->SetValue(project->bKeepZappedShapes);

		auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");
		if (targetGame == SF) {
			XRCCTRL(dlg, "sssSFMorphPath", wxTextCtrl)->ChangeValue(project->mSFMorphPath);

			// Populate morph target shape dropdown
			wxChoice* morphShapeChoice = XRCCTRL(dlg, "sssSFMorphTargetShape", wxChoice);
			morphShapeChoice->Append("(None)");
			for (auto& s : project->GetWorkNif()->GetShapes())
				morphShapeChoice->Append(wxString::FromUTF8(s->name.get()));

			// Select previously saved shape, or default to (None)
			if (!project->mSFMorphTargetShape.empty()) {
				int sel = morphShapeChoice->FindString(project->mSFMorphTargetShape);
				morphShapeChoice->SetSelection(sel != wxNOT_FOUND ? sel : 0);
			}
			else {
				morphShapeChoice->SetSelection(0);
			}
		}
		else {
			XRCCTRL(dlg, "m_SFMorphPathLabel", wxStaticText)->Hide();
			XRCCTRL(dlg, "sssSFMorphPath", wxTextCtrl)->Hide();
			XRCCTRL(dlg, "sssSFMorphPathBrowse", wxButton)->Hide();
			XRCCTRL(dlg, "m_SFMorphTargetShapeLabel", wxStaticText)->Hide();
			XRCCTRL(dlg, "sssSFMorphTargetShape", wxChoice)->Hide();
		}

		if (!project->GetBaseShape()) {
			XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->Disable();
		}
		else
			XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->SetValue(project->mCopyRef);

		// The base project changes when saving, not before
		auto shapeDataFolder = XRCCTRL(dlg, "sssShapeDataFolder", wxDirPickerCtrl);
		auto shapeDataFile = XRCCTRL(dlg, "sssShapeDataFile", wxFilePickerCtrl);
		auto sliderDataFile = XRCCTRL(dlg, "sssSliderDataFile", wxTextCtrl);
		const wxString ownDataFolder = shapeDataFolder->GetPath();
		const wxString ownDataFile = shapeDataFile->GetPath();
		sliderDataFile->ChangeValue(wxString::FromUTF8(project->activeSet.IsAddOn() ? project->activeSet.GetSliderDataFile() : outName + ".osd"));

		auto showBaseProject = [&]() {
			bool hasBase = baseChoice.changed ? !baseChoice.set.empty() : project->activeSet.HasBaseProject();
			bool addOn = baseChoice.changed ? hasBase && baseChoice.addOn : project->activeSet.IsAddOn();
			std::string baseName = baseChoice.changed ? baseChoice.set : project->activeSet.GetBaseProjectName();

			wxString baseText;
			if (hasBase)
				baseText = wxString::Format(addOn ? _("%s (add-on)") : _("%s (variant)"), wxString::FromUTF8(baseName));
			XRCCTRL(dlg, "sssBaseProject", wxTextCtrl)->ChangeValue(baseText);
			XRCCTRL(dlg, "sssBaseProjectClear", wxButton)->Enable(hasBase);

			// An add-on keeps the data folder and NIF file of its base project
			if (addOn && baseChoice.changed) {
				shapeDataFolder->SetPath(wxString::FromUTF8(baseChoice.baseFolder));
				shapeDataFile->SetPath(wxString::FromUTF8(baseChoice.baseNif));
			}
			else if (!addOn && project->activeSet.IsAddOn()) {
				// Without the add-on, the project needs a NIF file of its own
				shapeDataFolder->SetPath(ownDataFolder);
				wxString dataFile = sliderDataFile->GetValue();
				shapeDataFile->SetPath(dataFile.BeforeLast('.') + ".nif");
			}
			else if (!addOn) {
				shapeDataFolder->SetPath(ownDataFolder);
				shapeDataFile->SetPath(ownDataFile);
			}

			shapeDataFolder->Enable(!addOn);
			shapeDataFile->Enable(!addOn);
			XRCCTRL(dlg, "m_sssSliderDataFile", wxStaticText)->Show(addOn);
			sliderDataFile->Show(addOn);

			wxSize size = dlg.GetSize();
			dlg.Layout();
			dlg.Fit();
			dlg.SetSize(std::max(size.x, dlg.GetSize().x), dlg.GetSize().y);
		};

		XRCCTRL(dlg, "sssBaseProjectChoose", wxButton)->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
			wxFileDialog baseDialog(&dlg, _("Select the base project"), wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderSets", wxEmptyString, "Slider Set Files (*.osp;*.xml)|*.osp;*.xml", wxFD_FILE_MUST_EXIST);
			if (baseDialog.ShowModal() == wxID_CANCEL)
				return;

			std::string baseFile{baseDialog.GetPath().ToUTF8()};
			std::string baseSet = ChooseSliderSetName(baseFile);
			if (baseSet.empty())
				return;

			SliderSet base;
			SliderSetFile baseSetFile(baseFile);
			if (baseSetFile.GetSet(baseSet, base) != 0)
				return;

			// An add-on needs the shapes of its base project as they are
			wxArrayString kinds;
			kinds.Add(_("Variant, with a NIF file of its own"));
			if (project->BaseGeometryMatches(baseFile, baseSet))
				kinds.Add(_("Add-on, which uses the shapes and NIF file of the base project"));

			int kind = kinds.GetCount() > 1 ? 1 : 0;
			if (!projectHost.ChooseOne(wxString::Format(_("Make this project a variant or add-on of '%s'?\n\nSliders and slider data that equal the base project's are inherited from it then, and only the rest is saved with this project."),
														wxString::FromUTF8(baseSet)),
									   _("Base Project"),
									   kinds,
									   kind))
				return;

			baseChoice.changed = true;
			baseChoice.file = baseFile;
			baseChoice.set = baseSet;
			baseChoice.addOn = kind == 1;
			baseChoice.baseFolder = base.GetDefaultDataFolder();
			baseChoice.baseNif = base.GetInputFile();
			showBaseProject();
		});

		XRCCTRL(dlg, "sssBaseProjectClear", wxButton)->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
			baseChoice.changed = true;
			baseChoice.set.clear();
			showBaseProject();
		});

		// Other projects that use the NIF and .osd file are looked for while the dialog is open, and again
		// when the files change. The fields are read on a timer, because some of them change without events.
		auto sharedFilesHint = XRCCTRL(dlg, "sssSharedNifHint", wxStaticText);
		std::string sharedFilesKey;	  // Files and project the hint was made for
		std::string pendingFilesKey;  // Files that changed and are looked for once they stay the same
		wxLongLong pendingSince = 0;
		bool sharedFilesShown = false;

		// Green when no other project uses the files, orange when some do
		enum class SharedFilesState { Checking, Unused, Used };
		auto setSharedFilesHint = [&](const wxString& text, SharedFilesState state) {
			bool dark = dlg.GetBackgroundColour().GetLuminance() < 0.5;
			if (state == SharedFilesState::Unused)
				sharedFilesHint->SetForegroundColour(dark ? wxColour(110, 200, 110) : wxColour(0, 128, 0));
			else if (state == SharedFilesState::Used)
				sharedFilesHint->SetForegroundColour(dark ? wxColour(255, 170, 80) : wxColour(196, 98, 0));
			else
				sharedFilesHint->SetForegroundColour(dlg.GetForegroundColour());

			// Wrap() does nothing for the width it wrapped at before, even with a new label
			sharedFilesHint->SetLabel(text);
			sharedFilesHint->Wrap(-1);
			sharedFilesHint->Wrap(FromDIP(440));
			sharedFilesHint->Show(!text.empty());

			wxSize size = dlg.GetSize();
			dlg.Layout();
			dlg.Fit();
			dlg.SetSize(std::max(size.x, dlg.GetSize().x), dlg.GetSize().y);
		};

		auto listSets = [](const std::vector<const SliderSetIndexEntry*>& sets) {
			std::vector<std::string> names;
			for (auto* entry : sets)
				if (std::find(names.begin(), names.end(), entry->setName) == names.end())
					names.push_back(entry->setName);

			const size_t maxNames = 5;
			wxString list = wxString::FromUTF8(JoinStrings(std::vector<std::string>(names.begin(), names.begin() + std::min(names.size(), maxNames)), ", "));
			if (names.size() > maxNames)
				list += wxString::Format(_(" and %zu more"), names.size() - maxNames);
			return list;
		};

		auto showSharedFiles = [&](const SharedFilesTarget& target) {
			const SliderSetIndex& index = sharedFilesTask->Index();
			std::string projectPath = ProjectUtil::GetProjectPath();
			std::string folderPath = projectPath + PathSepStr + "ShapeData" + PathSepStr + target.dataDir;

			wxString text;
			if (!target.addOn) {
				auto nifSets = index.SetsUsingNif(folderPath + PathSepStr + target.nifName, target.setFile, target.setName);
				if (!nifSets.empty()) {
					text = wxString::Format(_("Other projects use this NIF file too: %s."), listSets(nifSets));
					if (!target.hasBase)
						text += " " + _("If this project only adds sliders to one of them, make it an add-on of that project with Base Project > Choose..., so that it saves only its own slider data.");
				}
			}

			auto dataSets = index.SetsUsingDataFile(folderPath + PathSepStr + target.osdName, target.setFile, target.setName);
			if (!dataSets.empty()) {
				if (!text.empty())
					text += "\n\n";
				text += wxString::Format(_("Other projects have slider data in '%s', which stays in it: %s."), wxString::FromUTF8(target.osdName), listSets(dataSets));
			}

			if (text.empty())
				setSharedFilesHint(_("No other project uses these files."), SharedFilesState::Unused);
			else
				setSharedFilesHint(text, SharedFilesState::Used);
		};

		wxTimer sharedFilesTimer(&dlg);
		dlg.Bind(wxEVT_TIMER, [&](wxTimerEvent&) {
			SharedFilesTarget target = SaveAsSharedFilesTarget(dlg, baseChoice.changed ? !baseChoice.set.empty() : project->activeSet.HasBaseProject(),
															   baseChoice.changed ? !baseChoice.set.empty() && baseChoice.addOn : project->activeSet.IsAddOn());
			std::string filesKey = target.nifName + "|" + target.osdName;
			std::string key = filesKey + "|" + target.dataDir + "|" + target.setFile + "|" + target.setName + (target.addOn ? "|addon" : "") + (target.hasBase ? "|base" : "");

			if (!sharedFilesTask || filesKey != JoinStrings(sharedFilesTask->FileNames(), "|")) {
				// Wait until the files stay the same for a moment before looking again
				if (sharedFilesTask && filesKey != pendingFilesKey) {
					pendingFilesKey = filesKey;
					pendingSince = wxGetLocalTimeMillis();
					return;
				}

				if (sharedFilesTask && wxGetLocalTimeMillis() - pendingSince < 400)
					return;

				sharedFilesTask.reset();
				sharedFilesTask = std::make_unique<SliderSetIndexTask>(ProjectUtil::GetProjectPath(), std::vector<std::string>{target.nifName, target.osdName});
				sharedFilesShown = false;
				sharedFilesKey.clear();
				setSharedFilesHint(_("Looking for other projects that use these files..."), SharedFilesState::Checking);
			}

			if (sharedFilesTask->IsDone() && (!sharedFilesShown || key != sharedFilesKey)) {
				showSharedFiles(target);
				sharedFilesShown = true;
				sharedFilesKey = key;
			}
		});

		showBaseProject();

		sharedFilesTimer.Start(150);
		wxTimerEvent firstCheck(sharedFilesTimer);
		dlg.ProcessWindowEvent(firstCheck);

		result = dlg.ShowModal();
		sharedFilesTimer.Stop();
	}
	if (result == wxID_CANCEL)
		return false;

	wxString strOutfitName;
	wxString strDataDir;
	wxString strBaseFile;
	wxString strGamePath;
	wxString strGameFile;

	wxFileName sliderSetFile = XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->GetFileName();
	if (!sliderSetFile.IsOk()) {
		wxMessageBox(_("Invalid or no slider set file specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return false;
	}

	if (sliderSetFile.GetExt() != "osp")
		sliderSetFile.SetExt("osp");

	strOutfitName = XRCCTRL(dlg, "sssName", wxTextCtrl)->GetValue();
	if (strOutfitName.empty()) {
		wxMessageBox(_("No outfit name specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return false;
	}

	strDataDir = SaveAsDataDir(dlg);
	if (strDataDir.empty()) {
		wxMessageBox(_("No data folder specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return false;
	}

	strBaseFile = XRCCTRL(dlg, "sssShapeDataFile", wxFilePickerCtrl)->GetFileName().GetFullName();
	if (strBaseFile.length() <= 4) {
		wxMessageBox(_("An invalid or no base outfit .nif file name specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return false;
	}

	if (!strBaseFile.EndsWith(".nif"))
		strBaseFile = strBaseFile.Append(".nif");

	strGamePath = XRCCTRL(dlg, "sssOutputDataPath", wxTextCtrl)->GetValue();
	if (strGamePath.empty()) {
		wxMessageBox(_("No game file path specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return false;
	}

	strGameFile = XRCCTRL(dlg, "sssOutputFileName", wxTextCtrl)->GetValue();
	if (strGameFile.empty()) {
		wxMessageBox(_("No game file name specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return false;
	}

	if (baseChoice.changed) {
		if (baseChoice.set.empty())
			project->ClearBaseProject();
		else {
			std::string baseError;
			if (!project->SetBaseProject(baseChoice.file, baseChoice.set, baseChoice.addOn, &baseError)) {
				wxMessageBox(wxString::FromUTF8(baseError), _("Error"), wxOK | wxICON_ERROR);
				return false;
			}
		}
	}

	if (project->activeSet.IsAddOn()) {
		wxString dataFile = XRCCTRL(dlg, "sssSliderDataFile", wxTextCtrl)->GetValue();
		if (dataFile.length() <= 4 && !dataFile.Lower().EndsWith(".osd"))
			dataFile = strOutfitName;
		if (!dataFile.Lower().EndsWith(".osd"))
			dataFile.Append(".osd");

		std::string dataFileName{dataFile.ToUTF8()};
		project->ReplaceForbidden(dataFileName);
		project->SetAddOnDataFile(dataFileName);
	}

	bool copyRef = XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->GetValue();
	bool genWeights = XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->GetValue();
	bool preventMorphFile = XRCCTRL(dlg, "sssPreventMorphFile", wxCheckBox)->GetValue();
	bool keepZappedShapes = XRCCTRL(dlg, "sssKeepZappedShapes", wxCheckBox)->GetValue();
	wxString strSFMorphPath = XRCCTRL(dlg, "sssSFMorphPath", wxTextCtrl)->GetValue();
	wxString strSFMorphTargetShape;
	wxChoice* morphShapeChoice = XRCCTRL(dlg, "sssSFMorphTargetShape", wxChoice);
	int sel = morphShapeChoice->GetSelection();
	if (sel > 0) // 0 = "(None)"
		strSFMorphTargetShape = morphShapeChoice->GetString(sel);

	wxLogMessage("Saving project '%s'...", strOutfitName);
	StartProgress(wxString::Format(_("Saving project '%s'..."), strOutfitName));

	std::vector<Mesh*> shapeMeshes;
	for (auto& s : project->GetWorkNif()->GetShapes()) {
		if (!project->IsBaseShape(s)) {
			Mesh* m = glView->GetMesh(s->name.get());
			if (m)
				shapeMeshes.push_back(m);
		}
	}

	project->UpdateNifNormals(project->GetWorkNif(), shapeMeshes);

	if (projectNotes)
		project->activeSet.SetNotes(projectNotes->GetValue().ToUTF8().data());

	// Saving doesn't wait for the look for other projects that use the files
	SharedFilesCheck sharedFilesCheck = SharedFilesCheck::Skip;
	const SliderSetIndex* sharedFilesIndex = nullptr;
	if (sharedFilesTask && sharedFilesTask->IsDone()) {
		sharedFilesCheck = SharedFilesCheck::Index;
		sharedFilesIndex = &sharedFilesTask->Index();
	}
	else if (sharedFilesTask)
		sharedFilesTask->Cancel();

	std::string error = project->Save(sliderSetFile,
									  strOutfitName,
									  strDataDir,
									  strBaseFile,
									  strGamePath,
									  strGameFile,
									  genWeights,
									  copyRef,
									  preventMorphFile,
									  keepZappedShapes,
									  strSFMorphPath,
									  strSFMorphTargetShape,
									  sharedFilesCheck,
									  sharedFilesIndex);

	if (error.empty()) {
		SetPendingChanges(false);
		menuBar->Enable(XRCID("fileSave"), true);

		RenameProject(strOutfitName.ToUTF8().data());
		EndProgress(_("Saved."));
	}
	else {
		wxLogError(error.c_str());
		projectHost.ShowMessage(HostMessageLevel::Error, error, _("Error"));
		EndProgress(_("Saving failed."));
		return false;
	}

	return true;
}

std::string OutfitStudioFrame::ChooseSliderSetName(const std::string& fileName, const std::string& preferredName) {
	SliderSetFile inFile(fileName);
	if (inFile.fail()) {
		wxLogError("Failed to open '%s' as a slider set file!", fileName);
		projectHost.ShowMessage(HostMessageLevel::Error, wxString::Format(_("Failed to open '%s' as a slider set file!"), fileName), _("Slider Set Error"));
		return "";
	}

	std::vector<std::string> setnames;
	inFile.GetSetNames(setnames);

	if (!preferredName.empty()) {
		auto it = std::find(setnames.begin(), setnames.end(), preferredName);
		if (it != setnames.end())
			return preferredName;
	}

	if (setnames.size() > 1) {
		wxArrayString choices;
		for (auto& s : setnames)
			choices.Add(wxString::FromUTF8(s));

		// Unattended callers have to name the set, there is no sensible default
		int sel = projectHost.IsInteractive() ? 0 : -1;
		if (!projectHost.ChooseOne(_("Please choose an outfit to load"), _("Load a slider set"), choices, sel) || sel < 0)
			return "";

		return choices[sel].ToUTF8().data();
	}

	if (setnames.size() == 1)
		return setnames.front();

	return "";
}

bool OutfitStudioFrame::LoadProject(const std::string& fileName,
									const std::string& projectName,
									bool clearProject,
									bool newDataLocal,
									bool appendNewSliders,
									bool setAsReference) {
	std::string outfit = ChooseSliderSetName(fileName, projectName);
	if (outfit.empty())
		return false;

	wxLogMessage("Loading project '%s' from file '%s'...", outfit, fileName);
	StartProgress(_("Loading project..."));

	if (clearProject) {
		ClearProject();
		project->ClearReference();
		project->ClearOutfit();

		glView->Cleanup();
		glView->GetUndoHistory()->ClearHistory();

		editor.activeSlider.clear();
		editor.lastActiveSlider.clear();
		editor.bEditSlider = false;
		MenuExitSliderEdit();

		physicsPreview->Shutdown();
		delete project;
		project = new OutfitProject(skeleton, &projectHost);
	}

	wxLogMessage("Loading outfit data...");
	UpdateProgress(10, _("Loading outfit data..."));
	StartSubProgress(10, 40);

	int error = 0;
	std::vector<std::string> origShapeOrder;

	if (clearProject)
		error = project->LoadFromSliderSet(fileName, outfit, &origShapeOrder);
	else
		error = project->AddFromSliderSet(fileName, outfit, newDataLocal, appendNewSliders, setAsReference);

	if (error) {
		EndProgress();
		wxLogError("Failed to create project (%d)!", error);
		projectHost.ShowMessage(HostMessageLevel::Error, wxString::Format(_("Failed to create project '%s' from file '%s' (%d)!"), outfit, fileName, error), _("Slider Set Error"));
		RefreshGUIFromProj();
		return false;
	}

	if (clearProject) {
		error = project->ReloadReferenceShape();
		if (error) {
			EndProgress();
			RefreshGUIFromProj();
			return false;
		}

		project->GetWorkNif()->SetShapeOrder(origShapeOrder);
	}

	wxLogMessage("Loading textures...");
	UpdateProgress(60, _("Loading textures..."));

	project->SetTextures();

	wxLogMessage("Creating outfit...");
	UpdateProgress(80, _("Creating outfit..."));
	RefreshGUIFromProj();

	wxLogMessage("Creating %zu slider(s)...", project->SliderCount());
	UpdateProgress(90, wxString::Format(_("Creating %zu slider(s)..."), project->SliderCount()));
	StartSubProgress(90, 99);
	CreateSetSliders();

	if (projectNotes && notesPane) {
		wxString notesText = wxString::FromUTF8(project->activeSet.GetNotes());
		projectNotes->SetValue(notesText);
		notesPane->Collapse(notesText.empty());

		UpdateToolScrollLayout();
	}

	UpdateTitle();
	AddProjectHistory(fileName, outfit);

	wxLogMessage("Project loaded.");
	menuBar->Enable(XRCID("fileSave"), true);
	EndProgress();
	return true;
}

void OutfitStudioFrame::UpdateTitle() {
	wxString name = wxString::FromUTF8(project->OutfitName());
	if (name.empty() || name == "New Outfit")
		name = project->mBaseFile;

	if (!name.empty()) {
		wxString baseLabel = BaseProjectLabel();
		if (!baseLabel.empty())
			name += " [" + baseLabel + "]";

		if (pendingChanges)
			SetTitle(name + "* - Outfit Studio");
		else
			SetTitle(name + " - Outfit Studio");
	}
	else
		SetTitle("Outfit Studio");

	UpdateProjectNode();
}

wxString OutfitStudioFrame::BaseProjectLabel() const {
	const SliderSet& set = project->activeSet;
	if (!set.HasBaseProject())
		return wxEmptyString;

	wxString baseName = wxString::FromUTF8(set.GetBaseProjectName());
	if (set.GetResolveStatus().baseMissing)
		return wxString::Format(set.IsAddOn() ? _("Add-on of %s, not found") : _("Variant of %s, not found"), baseName);

	return wxString::Format(set.IsAddOn() ? _("Add-on of %s") : _("Variant of %s"), baseName);
}

void OutfitStudioFrame::UpdateProjectNode() {
	if (!outfitRoot.IsOk())
		return;

	const SliderSet& set = project->activeSet;
	wxString baseLabel = BaseProjectLabel();

	auto shapes = project->GetWorkNif()->GetShapes();
	wxString text;
	if (baseLabel.empty() && shapes.size() == 1 && project->IsBaseShape(shapes.front()))
		text = "Reference Only";
	else
		text = wxString::FromUTF8(project->OutfitName());

	if (!baseLabel.empty())
		text += "  [" + baseLabel + "]";

	outfitShapes->SetItemText(outfitRoot, text);

	if (baseLabel.empty()) {
		outfitShapes->SetItemBold(outfitRoot, false);
		outfitShapes->SetItemTextColour(outfitRoot, outfitShapes->GetForegroundColour());
	}
	else {
		wxColour colour;
		if (set.GetResolveStatus().baseMissing)
			colour = wxColour(255, 110, 110);
		else if (set.IsAddOn())
			colour = wxColour(255, 190, 90);
		else
			colour = wxColour(120, 190, 255);

		outfitShapes->SetItemBold(outfitRoot);
		outfitShapes->SetItemTextColour(outfitRoot, colour);
	}
}

void OutfitStudioFrame::OnShapeTooltip(wxTreeEvent& event) {
	// The tree's own tooltip is replaced only when the next one shows, so OnShapeTreeMotion sets it instead
	event.Veto();
}

wxString OutfitStudioFrame::ProjectNodeTooltip() const {
	const SliderSet& set = project->activeSet;
	if (!set.HasBaseProject())
		return wxEmptyString;

	wxString baseName = wxString::FromUTF8(set.GetBaseProjectName());
	wxString tip = wxString::Format(set.IsAddOn() ? _("Slider add-on of project '%s'") : _("Variant of project '%s'"), baseName);

	if (!set.GetBaseProjectFile().empty())
		tip += "\n" + wxString::FromUTF8(set.GetBaseProjectFile());

	if (set.GetResolveStatus().baseMissing)
		tip += "\n\n" + _("The base project wasn't found, so this project is used as it was last saved.");
	else if (set.IsAddOn())
		tip += "\n\n" + _("Its shapes and NIF file belong to the base project. Sliders that come from the base project change with it.");
	else
		tip += "\n\n" + _("Sliders that come from the base project change with it.");

	return tip;
}

void OutfitStudioFrame::AddProjectHistory(const std::string& fileName, const std::string& projectName) {
	projectHistory.erase(std::remove_if(projectHistory.begin(),
										projectHistory.end(),
										[&fileName, &projectName](const ProjectHistoryEntry& rt) { return rt.fileName == fileName && rt.projectName == projectName; }),
						 projectHistory.end());

	ProjectHistoryEntry projectHistoryEntry{};
	projectHistoryEntry.fileName = fileName;
	projectHistoryEntry.projectName = projectName;

	constexpr int DEF_PROJECT_HISTORY = 15;
	constexpr int MAX_PROJECT_HISTORY = 100;

	int maxCount = OutfitStudioConfig.GetIntValue("ProjectHistory.maxcount", DEF_PROJECT_HISTORY);
	if (maxCount <= 0)
		maxCount = DEF_PROJECT_HISTORY;
	else if (maxCount > MAX_PROJECT_HISTORY)
		maxCount = MAX_PROJECT_HISTORY;

	if (projectHistory.size() == static_cast<size_t>(maxCount))
		projectHistory.pop_back();

	projectHistory.push_front(projectHistoryEntry);
	UpdateProjectHistory();
}

void OutfitStudioFrame::UpdateProjectHistory() {
	menuBar->Freeze();

	wxMenuItem* menuItemRecentProjects = menuBar->FindItem(XRCID("menuRecentProjects"));
	if (menuItemRecentProjects && menuItemRecentProjects->IsSubMenu()) {
		wxMenu* menuRecentProjects = menuItemRecentProjects->GetSubMenu();
		if (menuRecentProjects) {
			auto menuItemList = menuRecentProjects->GetMenuItems();
			for (auto menuItem : menuItemList)
				menuRecentProjects->Delete(menuItem);

			int itemId = 0;
			for (const auto& projectHistoryEntry : projectHistory) {
				menuRecentProjects->Append(1000 + itemId, wxString::FromUTF8(projectHistoryEntry.projectName), wxString::FromUTF8(projectHistoryEntry.fileName));
				++itemId;
			}

			menuBar->Enable(XRCID("menuRecentProjects"), itemId > 0);
		}
	}

	menuBar->Thaw();
}

void OutfitStudioFrame::SetPendingChanges(bool pending) {
	if (pendingChanges != pending) {
		pendingChanges = pending;
		UpdateTitle();
	}
}

bool OutfitStudioFrame::CheckPendingChanges() {
	if (pendingChanges) {
		wxMessageDialog dlg(this,
							wxString::Format(_("You have unsaved changes to '%s'. Would you like to save them now?"), project->OutfitName()),
							_("Unsaved Changes"),
							wxYES_NO | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);

		int res = dlg.ShowModal();
		if (res == wxID_YES) {
			if (!SaveProject())
				return false;
		}
		else if (res == wxID_CANCEL)
			return false;
	}

	return true;
}

void OutfitStudioFrame::OnNewProject(wxCommandEvent& WXUNUSED(event)) {
	wxWizard wiz;
	wxWizardPage* pg1;
	bool result = false;

	if (!CheckPendingChanges())
		return;

	CloseBrushSettings();
	UpdateReferenceTemplates();

	if (wxXmlResource::Get()->LoadObject((wxObject*)&wiz, this, "wizNewProject", "wxWizard")) {
		pg1 = (wxWizardPage*)XRCCTRL(wiz, "wizpgNewProj1", wxWizardPageSimple);
		XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnNPWizChangeSliderSetFile, this);
		XRCCTRL(wiz, "npSliderSetName", wxChoice)->Bind(wxEVT_CHOICE, &OutfitStudioFrame::OnNPWizChangeSetNameChoice, this);

		XRCCTRL(wiz, "npWorkFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnLoadOutfitFP_File, this);
		XRCCTRL(wiz, "npTexFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnLoadOutfitFP_Texture, this);

		ConfigDialogUtil::LoadDialogChoices(OutfitStudioConfig, wiz, "LoadReference", "npTemplateChoice", refTemplates);
		ConfigDialogUtil::LoadDialogCheckBox(OutfitStudioConfig, wiz, "LoadReference", "npLoadAddOns");

		// An add-on or variant picks its base project where a new project picks its reference
		auto intro2 = XRCCTRL(wiz, "lbNewProj2", wxStaticText);
		const wxString newProjectIntro2 = intro2->GetLabel();

		wxStaticBox* referenceBox = nullptr;
		for (auto* child : pg1->GetChildren())
			if (auto* box = wxDynamicCast(child, wxStaticBox))
				if (box->GetLabel() == _("Reference"))
					referenceBox = box;

		auto updateProjectType = [&](wxCommandEvent&) {
			bool derived = !XRCCTRL(wiz, "npTypeNew", wxRadioButton)->GetValue();

			// Shorter than the label it replaces, so it fits the page as it was sized
			intro2->SetLabel(derived ? _("Next, enter a display name for the new project.") : newProjectIntro2);
			intro2->Wrap(FromDIP(380));

			if (referenceBox)
				referenceBox->SetLabel(derived ? _("Base Project") : _("Reference"));

			auto refIsBlank = XRCCTRL(wiz, "npRefIsBlank", wxRadioButton);
			if (derived && refIsBlank->GetValue())
				XRCCTRL(wiz, "npRefIsTemplate", wxRadioButton)->SetValue(true);
			refIsBlank->Enable(!derived);
			XRCCTRL(wiz, "lbRefShapeName", wxStaticText)->Enable(!derived);
			XRCCTRL(wiz, "npRefShapeName", wxChoice)->Enable(!derived);

			for (auto name : {"npWorkFile", "npWorkFilename", "npWorkNone", "npTexAuto", "npTexFile", "npTexFilename", "npLoadAddOns"})
				if (wxWindow* control = wiz.FindWindow(XRCID(name)))
					control->Enable(!derived);

			pg1->Layout();
		};

		for (auto name : {"npTypeNew", "npTypeAddOn", "npTypeVariant"})
			XRCCTRL(wiz, name, wxRadioButton)->Bind(wxEVT_RADIOBUTTON, updateProjectType);

		// The base project has to be a set of a project file
		wiz.Bind(wxEVT_WIZARD_PAGE_CHANGING, [&](wxWizardEvent& event) {
			event.Skip();
			if (!event.GetDirection() || event.GetPage() != pg1 || XRCCTRL(wiz, "npTypeNew", wxRadioButton)->GetValue())
				return;

			std::string baseFile;
			std::string baseSet;
			if (!GetNewProjectBase(wiz, baseFile, baseSet)) {
				wxMessageBox(_("Please choose a template or a project file (.osp) with a slider set as the base project."), _("Base Project"), wxOK | wxICON_ERROR, &wiz);
				event.Veto();
				return;
			}

			auto outfitNameCtrl = XRCCTRL(wiz, "npOutfitName", wxTextCtrl);
			if (outfitNameCtrl->GetValue() == "New Outfit") {
				bool addOn = XRCCTRL(wiz, "npTypeAddOn", wxRadioButton)->GetValue();
				outfitNameCtrl->SetValue(wxString::FromUTF8(baseSet) + (addOn ? _(" Add-on") : _(" Variant")));
			}
		});

		wiz.FitToPage(pg1);
		wiz.CenterOnParent();

		result = wiz.RunWizard(pg1);
	}
	if (!result)
		return;

	std::string outfitName{XRCCTRL(wiz, "npOutfitName", wxTextCtrl)->GetValue().ToUTF8()};

	// A variant or add-on starts out as its base project, with everything inherited
	bool newAddOn = XRCCTRL(wiz, "npTypeAddOn", wxRadioButton)->GetValue();
	if (newAddOn || XRCCTRL(wiz, "npTypeVariant", wxRadioButton)->GetValue()) {
		std::string baseFile;
		std::string baseSet;
		if (!GetNewProjectBase(wiz, baseFile, baseSet))
			return;

		if (XRCCTRL(wiz, "npRefIsTemplate", wxRadioButton)->GetValue())
			ConfigDialogUtil::SetStringFromDialogChoice(OutfitStudioConfig, wiz, "LoadReference", "npTemplateChoice");

		wxLogMessage("Creating %s '%s' of project '%s'...", newAddOn ? "add-on" : "variant", outfitName, baseSet);
		if (!LoadProject(baseFile, baseSet))
			return;

		project->MakeDerivedProject(baseFile, baseSet, newAddOn, outfitName);
		if (projectNotes)
			projectNotes->SetValue("");

		menuBar->Enable(XRCID("fileSave"), false);
		SetPendingChanges(true);
		UpdateTitle();
		return;
	}

	menuBar->Enable(XRCID("fileSave"), false);

	wxLogMessage("Creating project '%s'...", outfitName);
	StartProgress(wxString::Format(_("Creating project '%s'..."), outfitName));

	ClearProject();
	project->ClearReference();
	project->ClearOutfit();

	glView->Cleanup();
	glView->GetUndoHistory()->ClearHistory();

	editor.activeSlider.clear();
	editor.lastActiveSlider.clear();
	editor.bEditSlider = false;
	MenuExitSliderEdit();

	physicsPreview->Shutdown();
	delete project;
	project = new OutfitProject(skeleton, &projectHost);

	UpdateProgress(10, _("Loading reference..."));

	int error = 0;
	if (XRCCTRL(wiz, "npRefIsTemplate", wxRadioButton)->GetValue() == true) {
		wxString refTemplate = ConfigDialogUtil::SetStringFromDialogChoice(OutfitStudioConfig, wiz, "LoadReference", "npTemplateChoice");
		wxLogMessage("Loading reference template '%s'...", refTemplate);

		std::string tmplName{refTemplate.ToUTF8()};
		auto tmpl = find_if(refTemplates.begin(), refTemplates.end(), [&tmplName](const RefTemplate& rt) { return rt.GetName() == tmplName; });
		if (tmpl != refTemplates.end()) {
			if (wxFileName(wxString::FromUTF8(tmpl->GetSource())).IsRelative())
				error = project->LoadReferenceTemplate(ProjectUtil::GetProjectPath() + PathSepStr + tmpl->GetSource(), tmpl->GetSetName(), tmpl->GetShape(), tmpl->GetLoadAll());
			else
				error = project->LoadReferenceTemplate(tmpl->GetSource(), tmpl->GetSetName(), tmpl->GetShape(), tmpl->GetLoadAll());
		}
		else
			error = 1;
	}
	else if (XRCCTRL(wiz, "npRefIsSliderset", wxRadioButton)->GetValue() == true) {
		wxString fileName = XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->GetPath();
		wxString refShape = XRCCTRL(wiz, "npRefShapeName", wxChoice)->GetStringSelection();

		if (fileName.EndsWith(".osp") || fileName.EndsWith(".xml")) {
			wxString sliderSetName = XRCCTRL(wiz, "npSliderSetName", wxChoice)->GetStringSelection();
			wxLogMessage("Loading reference '%s' from set '%s' of file '%s'...", refShape, sliderSetName, fileName);

			error = project->LoadReference(fileName.ToUTF8().data(), sliderSetName.ToUTF8().data(), refShape.ToUTF8().data());
		}
		else if (fileName.EndsWith(".nif")) {
			wxLogMessage("Loading reference '%s' from '%s'...", refShape, fileName);
			error = project->LoadReferenceNif(fileName.ToUTF8().data(), refShape.ToUTF8().data());
		}
	}

	if (error) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	bool loadAddOns = ConfigDialogUtil::SetBoolFromDialogCheckbox(OutfitStudioConfig, wiz, "LoadReference", "npLoadAddOns");
	bool refFromNif = XRCCTRL(wiz, "npRefIsSliderset", wxRadioButton)->GetValue() && XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->GetPath().Lower().EndsWith(".nif");
	if (loadAddOns && !refFromNif && XRCCTRL(wiz, "npTypeNew", wxRadioButton)->GetValue())
		ProjectOperations(*project, editor).LoadReferenceAddOns();

	UpdateProgress(40, _("Loading outfit..."));

	error = 0;
	if (XRCCTRL(wiz, "npWorkFile", wxRadioButton)->GetValue() == true) {
		wxString fileName = XRCCTRL(wiz, "npWorkFilename", wxFilePickerCtrl)->GetPath();
		wxLogMessage("Loading outfit '%s' from '%s'...", outfitName, fileName);
		if (fileName.Lower().EndsWith(".nif"))
			error = project->ImportNIF(fileName.ToUTF8().data(), true, outfitName);
		else if (fileName.Lower().EndsWith(".obj"))
			error = ImportOBJWithDialog(fileName.ToUTF8().data(), outfitName);
#ifdef USE_FBXSDK
		else if (fileName.Lower().EndsWith(".fbx"))
			error = ImportFBXWithDialog(fileName.ToUTF8().data(), outfitName);
#endif
	}

	if (error) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	wxLogMessage("Creating outfit...");
	UpdateProgress(80, _("Creating outfit..."));

	if (XRCCTRL(wiz, "npTexAuto", wxRadioButton)->GetValue() == false)
		project->SetTextures({XRCCTRL(wiz, "npTexFilename", wxFilePickerCtrl)->GetPath().ToUTF8().data()});
	else
		project->SetTextures();

	RefreshGUIFromProj();

	wxLogMessage("Creating %zu slider(s)...", project->SliderCount());
	UpdateProgress(90, wxString::Format(_("Creating %zu slider(s)..."), project->SliderCount()));
	StartSubProgress(90, 99);
	CreateSetSliders();

	if (!outfitName.empty())
		UpdateTitle();

	wxLogMessage("Project created.");
	UpdateProgress(100, _("Finished"));

	EndProgress();
}

bool OutfitStudioFrame::ConfirmRenameFromBase(const std::string& sliderName, const std::string& shapeName) {
	if (!project->activeSet.HasBaseProject())
		return true;

	wxString baseName = wxString::FromUTF8(project->activeSet.GetBaseProjectName());
	wxString msg;
	if (!sliderName.empty()) {
		if (!project->SliderFromBase(sliderName))
			return true;

		msg = wxString::Format(_("'%s' comes from base project '%s'. If you rename it, its data is copied into this project and later changes to the base slider won't reach it. '%s' will be hidden in this project.\n\nRename anyway?"),
							   wxString::FromUTF8(sliderName),
							   baseName,
							   wxString::FromUTF8(sliderName));
	}
	else {
		std::set<std::string> sliders;
		for (auto& key : project->RenameDisconnectsFromBase("", shapeName))
			sliders.insert(key.sliderName);

		if (sliders.empty())
			return true;

		msg = wxString::Format(_("Shape '%s' uses slider data from base project '%s' (%zu sliders). If you rename it, that data is copied into this project and later changes to the base won't reach it.\n\nRename anyway?"),
							   wxString::FromUTF8(shapeName),
							   baseName,
							   sliders.size());
	}

	return wxMessageBox(msg, _("Rename"), wxYES_NO | wxICON_WARNING, this) == wxYES;
}

bool OutfitStudioFrame::GetNewProjectBase(wxWizard& wiz, std::string& outFile, std::string& outSet) {
	outFile.clear();
	outSet.clear();

	if (XRCCTRL(wiz, "npRefIsTemplate", wxRadioButton)->GetValue()) {
		std::string tmplName{XRCCTRL(wiz, "npTemplateChoice", wxChoice)->GetStringSelection().ToUTF8()};
		auto tmpl = find_if(refTemplates.begin(), refTemplates.end(), [&tmplName](const RefTemplate& rt) { return rt.GetName() == tmplName; });
		if (tmpl == refTemplates.end())
			return false;

		outFile = tmpl->GetSource();
		if (wxFileName(wxString::FromUTF8(outFile)).IsRelative())
			outFile = ProjectUtil::GetProjectPath() + PathSepStr + outFile;
		outSet = tmpl->GetSetName();
	}
	else if (XRCCTRL(wiz, "npRefIsSliderset", wxRadioButton)->GetValue()) {
		wxString fileName = XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->GetPath();
		if (!fileName.Lower().EndsWith(".osp") && !fileName.Lower().EndsWith(".xml"))
			return false;

		outFile = fileName.ToUTF8().data();
		outSet = XRCCTRL(wiz, "npSliderSetName", wxChoice)->GetStringSelection().ToUTF8().data();
	}

	return !outFile.empty() && !outSet.empty() && SliderSetFile(outFile).HasSet(outSet);
}

void OutfitStudioFrame::OnLoadProject(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog loadProjectDialog(this,
								   _("Select a slider set to load"),
								   wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderSets",
								   wxEmptyString,
								   "Slider Set Files (*.osp;*.xml)|*.osp;*.xml",
								   wxFD_FILE_MUST_EXIST);
	if (loadProjectDialog.ShowModal() == wxID_CANCEL)
		return;

	if (!CheckPendingChanges())
		return;

	std::string fileName{loadProjectDialog.GetPath().ToUTF8()};
	LoadProject(fileName);
}

void OutfitStudioFrame::OnAddProject(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog addProjectDialog(this,
								  _("Select a slider set to add"),
								  wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderSets",
								  wxEmptyString,
								  "Slider Set Files (*.osp;*.xml)|*.osp;*.xml",
								  wxFD_FILE_MUST_EXIST);
	if (addProjectDialog.ShowModal() == wxID_CANCEL)
		return;

	if (!CheckPendingChanges())
		return;

	std::string fileName{addProjectDialog.GetPath().ToUTF8()};

	// Choose the slider set to add before showing the options dialog
	std::string outfit = ChooseSliderSetName(fileName);
	if (outfit.empty())
		return;

	AddProjectDialog optionsDialog(this, OutfitStudioConfig);
	if (optionsDialog.ShowModal() != wxID_OK)
		return;

	const auto& options = optionsDialog.GetOptions();

	LoadProject(fileName, outfit, false, options.sliderDataLocal, options.appendNewSliders, options.setAsReference);
}

void OutfitStudioFrame::OnLoadReference(wxCommandEvent& WXUNUSED(event)) {
	if (editor.bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	CloseBrushSettings();
	UpdateReferenceTemplates();

	wxDialog dlg;
	int result = wxID_CANCEL;
	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgLoadRef", "wxDialog")) {
		XRCCTRL(dlg, "npSliderSetFile", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnNPWizChangeSliderSetFile, this);
		XRCCTRL(dlg, "npSliderSetName", wxChoice)->Bind(wxEVT_CHOICE, &OutfitStudioFrame::OnNPWizChangeSetNameChoice, this);

		ConfigDialogUtil::LoadDialogChoices(OutfitStudioConfig, dlg, "LoadReference", "npTemplateChoice", refTemplates);

		ConfigDialogUtil::LoadDialogCheckBox(OutfitStudioConfig, dlg, "LoadReference", "chkMergeSliders");
		ConfigDialogUtil::LoadDialogCheckBox(OutfitStudioConfig, dlg, "LoadReference", "chkMergeZaps");
		ConfigDialogUtil::LoadDialogCheckBox(OutfitStudioConfig, dlg, "LoadReference", "chkAppendNewSliders");
		ConfigDialogUtil::LoadDialogCheckBox(OutfitStudioConfig, dlg, "LoadReference", "npLoadAddOns");

		dlg.Fit();
		result = dlg.ShowModal();
	}
	if (result == wxID_CANCEL)
		return;

	StartProgress(_("Loading reference..."));

	NiShape* baseShape = project->GetBaseShape();
	if (baseShape)
		glView->DeleteMesh(baseShape->name.get());

	UpdateProgress(10, _("Loading reference set..."));
	bool mergeSliders = ConfigDialogUtil::SetBoolFromDialogCheckbox(OutfitStudioConfig, dlg, "LoadReference", "chkMergeSliders");
	bool mergeZaps = ConfigDialogUtil::SetBoolFromDialogCheckbox(OutfitStudioConfig, dlg, "LoadReference", "chkMergeZaps");
	bool appendNewSliders = ConfigDialogUtil::SetBoolFromDialogCheckbox(OutfitStudioConfig, dlg, "LoadReference", "chkAppendNewSliders");

	int error = 0;
	if (XRCCTRL(dlg, "npRefIsTemplate", wxRadioButton)->GetValue() == true) {
		wxString refTemplate = ConfigDialogUtil::SetStringFromDialogChoice(OutfitStudioConfig, dlg, "LoadReference", "npTemplateChoice");
		wxLogMessage("Loading reference template '%s'...", refTemplate);

		std::string tmplName{refTemplate.ToUTF8()};
		auto tmpl = find_if(refTemplates.begin(), refTemplates.end(), [&tmplName](const RefTemplate& rt) { return rt.GetName() == tmplName; });
		if (tmpl != refTemplates.end()) {
			if (wxFileName(wxString::FromUTF8(tmpl->GetSource())).IsRelative())
				error = project->LoadReferenceTemplate(ProjectUtil::GetProjectPath() + PathSepStr + tmpl->GetSource(),
													   tmpl->GetSetName(),
													   tmpl->GetShape(),
													   tmpl->GetLoadAll(),
													   mergeSliders,
													   mergeZaps,
													   appendNewSliders);
			else
				error = project->LoadReferenceTemplate(tmpl->GetSource(), tmpl->GetSetName(), tmpl->GetShape(), tmpl->GetLoadAll(), mergeSliders, mergeZaps, appendNewSliders);
		}
		else
			error = 1;
	}
	else if (XRCCTRL(dlg, "npRefIsSliderset", wxRadioButton)->GetValue() == true) {
		wxString fileName = XRCCTRL(dlg, "npSliderSetFile", wxFilePickerCtrl)->GetPath();
		wxString refShape = XRCCTRL(dlg, "npRefShapeName", wxChoice)->GetStringSelection();

		if (fileName.EndsWith(".osp") || fileName.EndsWith(".xml")) {
			wxString sliderSetName = XRCCTRL(dlg, "npSliderSetName", wxChoice)->GetStringSelection();
			wxLogMessage("Loading reference '%s' from set '%s' of file '%s'...", refShape, sliderSetName, fileName);

			error = project->LoadReference(fileName.ToUTF8().data(), sliderSetName.ToUTF8().data(), refShape.ToUTF8().data(), mergeSliders, mergeZaps, appendNewSliders);
		}
		else if (fileName.EndsWith(".nif")) {
			wxLogMessage("Loading reference '%s' from '%s'...", refShape, fileName);
			error = project->LoadReferenceNif(fileName.ToUTF8().data(), refShape.ToUTF8().data(), mergeSliders, mergeZaps);
		}
	}
	else
		project->ClearReference();

	if (error) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	bool loadAddOns = ConfigDialogUtil::SetBoolFromDialogCheckbox(OutfitStudioConfig, dlg, "LoadReference", "npLoadAddOns");
	bool refFromSet = XRCCTRL(dlg, "npRefIsTemplate", wxRadioButton)->GetValue()
		|| (XRCCTRL(dlg, "npRefIsSliderset", wxRadioButton)->GetValue() && !XRCCTRL(dlg, "npSliderSetFile", wxFilePickerCtrl)->GetPath().Lower().EndsWith(".nif"));
	if (loadAddOns && refFromSet)
		ProjectOperations(*project, editor).LoadReferenceAddOns();

	project->SetTextures(project->GetBaseShape());

	wxLogMessage("Creating reference...");
	UpdateProgress(60, _("Creating reference..."));
	RefreshGUIFromProj();

	wxLogMessage("Creating %zu slider(s)...", project->SliderCount());
	UpdateProgress(70, wxString::Format(_("Creating %zu slider(s)..."), project->SliderCount()));
	StartSubProgress(70, 99);
	CreateSetSliders();

	wxLogMessage("Reference loaded.");
	EndProgress();
}

void OutfitStudioFrame::OnConvertBodyReference(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	auto shapes = project->GetWorkNif()->GetShapes();
	auto baseShape = project->GetBaseShape();
	if (shapes.size() == 0 || (baseShape && shapes.size() == 1)) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	if (editor.bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	UpdateReferenceTemplates();

	ConvertBodyReferenceDialog dlg(this, project, OutfitStudioConfig, refTemplates);
	if (!dlg.Load())
		return;

	dlg.ConvertBodyReference();
}

void OutfitStudioFrame::OnRunAutomation(wxCommandEvent& WXUNUSED(event)) {
	if (editor.bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	AutomationDialog dlg(this, project);
	dlg.ShowModal();
}

void OutfitStudioFrame::OnLoadOutfit(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	int result = wxID_CANCEL;

	CloseBrushSettings();

	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgLoadOutfit", "wxDialog")) {
		XRCCTRL(dlg, "npWorkFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnLoadOutfitFP_File, this);
		XRCCTRL(dlg, "npTexFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnLoadOutfitFP_Texture, this);
		if (project->GetWorkNif()->IsValid())
			XRCCTRL(dlg, "npWorkAdd", wxCheckBox)->Enable();

		result = dlg.ShowModal();
	}
	if (result == wxID_CANCEL)
		return;

	std::string outfitName = XRCCTRL(dlg, "npOutfitName", wxTextCtrl)->GetValue().ToStdString();

	menuBar->Enable(XRCID("fileSave"), false);

	wxLogMessage("Loading outfit...");
	StartProgress(_("Loading outfit..."));

	for (auto& s : project->GetWorkNif()->GetShapes()) {
		if (!project->IsBaseShape(s)) {
			glView->DeleteMesh(s->name.get());
		}
	}

	bool keepShapes = XRCCTRL(dlg, "npWorkAdd", wxCheckBox)->IsChecked();
	UpdateProgress(1, _("Loading outfit..."));

	int ret = 0;
	if (XRCCTRL(dlg, "npWorkFile", wxRadioButton)->GetValue() == true) {
		wxString fileName = XRCCTRL(dlg, "npWorkFilename", wxFilePickerCtrl)->GetPath();
		if (fileName.Lower().EndsWith(".nif")) {
			if (!keepShapes)
				ret = project->ImportNIF(fileName.ToUTF8().data(), true, outfitName);
			else
				ret = project->ImportNIF(fileName.ToUTF8().data(), false);
		}
		else if (fileName.Lower().EndsWith(".obj"))
			ret = ImportOBJWithDialog(fileName.ToUTF8().data(), outfitName);
#ifdef USE_FBXSDK
		else if (fileName.Lower().EndsWith(".fbx"))
			ret = ImportFBXWithDialog(fileName.ToUTF8().data(), outfitName);
#endif
	}
	else
		project->ClearOutfit();

	if (ret) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	if (XRCCTRL(dlg, "npTexAuto", wxRadioButton)->GetValue() == true)
		project->SetTextures();
	else {
		std::vector<std::string> texVec = {XRCCTRL(dlg, "npTexFilename", wxFilePickerCtrl)->GetPath().ToUTF8().data()};
		project->SetTextures(texVec);
	}

	wxLogMessage("Creating outfit...");
	UpdateProgress(50, _("Creating outfit..."));
	RefreshGUIFromProj();

	UpdateTitle();

	wxLogMessage("Outfit loaded.");
	EndProgress();
}

void OutfitStudioFrame::OnBrowseArchives(wxCommandEvent& WXUNUSED(event)) {
	if (!archiveBrowser) {
		archiveBrowser = new ArchiveBrowser(this);
		archiveBrowser->Bind(wxEVT_CLOSE_WINDOW, [&](wxCloseEvent& event) {
			archiveBrowser = nullptr;
			event.Skip();
		});
	}

	archiveBrowser->Show();
	archiveBrowser->Raise();
}

bool OutfitStudioFrame::LoadMeshFromArchive(const std::string& relPath, const std::string& archivePath, bool import) {
	auto stream = GameDataStream::OpenArchive(relPath, archivePath);
	if (!stream) {
		wxLogError("Could not read '%s' from archive '%s'!", relPath, archivePath);
		wxMessageBox(wxString::Format(_("Could not read '%s' from archive '%s'!"), wxString::FromUTF8(relPath), wxString::FromUTF8(archivePath)),
					 _("Archive Error"),
					 wxICON_ERROR,
					 this);
		return false;
	}

	CloseBrushSettings();

	if (import) {
		StartProgress(_("Importing NIF file..."));
		UpdateProgress(1, _("Importing NIF file..."));

		bool loaded = project->ImportNIF(*stream, relPath, false) == 0;

		UpdateProgress(60, _("Refreshing GUI..."));
		project->SetTextures();

		SetPendingChanges();
		RefreshGUIFromProj();

		UpdateTitle();
		EndProgress();
		return loaded;
	}

	menuBar->Enable(XRCID("fileSave"), false);

	wxLogMessage("Loading outfit from archive...");
	StartProgress(_("Loading outfit..."));

	for (auto& s : project->GetWorkNif()->GetShapes()) {
		if (!project->IsBaseShape(s)) {
			glView->DeleteMesh(s->name.get());
		}
	}

	UpdateProgress(1, _("Loading outfit..."));

	if (project->ImportNIF(*stream, relPath, true)) {
		EndProgress();
		RefreshGUIFromProj();
		return false;
	}

	project->SetTextures();

	wxLogMessage("Creating outfit...");
	UpdateProgress(50, _("Creating outfit..."));
	RefreshGUIFromProj();

	UpdateTitle();

	wxLogMessage("Outfit loaded.");
	EndProgress();
	return true;
}

void OutfitStudioFrame::OnUnloadProject(wxCommandEvent& WXUNUSED(event)) {
	wxMessageDialog dlg(this, _("Unload the project? All unsaved changes will be lost"), _("Unload Project"), wxOK | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);
	dlg.SetOKCancelLabels(_("Unload"), _("Cancel"));
	if (dlg.ShowModal() != wxID_OK)
		return;

	wxLogMessage("Unloading project...");
	menuBar->Enable(XRCID("fileSave"), false);

	ResetProject();

	physicsPreview->Shutdown();
	delete project;
	project = new OutfitProject(skeleton, &projectHost);

	CreateSetSliders();
	RefreshGUIFromProj(false);
	glView->Render();

	statusBar->SetStatusText(_("Ready!"));
}

void OutfitStudioFrame::ResetProject() {
	ClearProject();
	project->ClearReference();
	project->ClearOutfit();

	glView->Cleanup();
	glView->GetUndoHistory()->ClearHistory();

	editor.activeSlider.clear();
	editor.lastActiveSlider.clear();
	editor.bEditSlider = false;
	MenuExitSliderEdit();

	CreateSetSliders();
	RefreshGUIFromProj(false);
}

void OutfitStudioFrame::UpdateReferenceTemplates() {
	refTemplates.clear();

	std::string fileName = ProjectUtil::GetProjectPath() + "/RefTemplates.xml";
	if (wxFileName::IsFileReadable(fileName)) {
		RefTemplateFile refTemplateFile(fileName);
		refTemplateFile.GetAll(refTemplates);
	}

	RefTemplateCollection refTemplateCol;
	refTemplateCol.Load(ProjectUtil::GetProjectPath() + "/RefTemplates");
	refTemplateCol.GetAll(refTemplates);
}

void OutfitStudioFrame::ClearProject() {
	if (editUV)
		editUV->Close();

	for (auto& s : project->GetWorkNif()->GetShapeNames())
		glView->DeleteMesh(s);

	project->mFileName.clear();
	project->mOutfitName.clear();
	project->mDataDir.clear();
	project->mBaseFile.clear();
	project->mGamePath.clear();
	project->mGameFile.clear();

	if (wxGetApp().targetGame == SKYRIM || wxGetApp().targetGame == SKYRIMSE || wxGetApp().targetGame == SKYRIMVR)
		project->mGenWeights = true;
	else
		project->mGenWeights = false;

	project->mCopyRef = true;

	glView->gls.ClearOverlays();
	segPartEditor->ClearActive();

	lastCheckedSlider.clear();
	editor.lastSelectedBones.clear();
	editor.lastNormalizeBones.clear();

	if (currentTabButton)
		currentTabButton->SetPendingChanges(false);

	auto cMaskName = (wxComboBox*)FindWindowByName("cMaskName");
	cMaskName->Clear();

	auto cPoseName = (wxComboBox*)FindWindowByName("cPoseName");
	cPoseName->Clear();
	cPoseName->Append("<New>", (void*)nullptr);
	cPoseName->SetStringSelection("<New>");

	animation->ResetList();

	if (projectNotes && notesPane) {
		projectNotes->Clear();
		notesPane->Collapse();

		UpdateToolScrollLayout();
	}

	project->outfitName.clear();
	pendingChanges = false;
	UpdateTitle();
}

void OutfitStudioFrame::RenameProject(const std::string& projectName) {
	project->outfitName = projectName;
	UpdateTitle();
}

void OutfitStudioFrame::RefreshGUIFromProj(bool render, bool stashMasks) {
	LockShapeSelect();

	selectedItems.clear();
	std::vector<ShapeItemState> prevStates;

	if (outfitRoot.IsOk()) {
		// Collect names from tree items using the tree label text (safe even if NiShape* is dangling)
		wxTreeItemIdValue cookie;
		wxTreeItemId child = outfitShapes->GetFirstChild(outfitRoot, cookie);
		while (child.IsOk()) {
			auto itemData = (ShapeItemData*)outfitShapes->GetItemData(child);
			if (itemData) {
				ShapeItemState prevState{};
				prevState.shapeName = outfitShapes->GetItemText(child).ToUTF8().data();
				prevState.state = outfitShapes->GetItemState(child);

				if (outfitShapes->IsSelected(child))
					prevState.selected = true;

				prevStates.push_back(prevState);
			}

			child = outfitShapes->GetNextChild(outfitRoot, cookie);
		}

		outfitShapes->UnselectAll();
		outfitShapes->DeleteChildren(outfitRoot);
		outfitShapes->Delete(outfitRoot);
		outfitRoot.Unset();
	}

	auto shapes = project->GetWorkNif()->GetShapes();
	if (shapes.size() > 0) {
		outfitRoot = outfitShapes->AppendItem(shapesRoot, wxEmptyString);
		UpdateProjectNode();
	}

	wxTreeItemId item;
	wxTreeItemId firstItem;
	wxTreeItemId prevFirstSelItem;
	for (auto& shape : shapes) {
		auto itemData = new ShapeItemData(shape);
		item = outfitShapes->AppendItem(outfitRoot, wxString::FromUTF8(shape->name.get()));
		outfitShapes->SetItemState(item, 0);
		outfitShapes->SetItemData(item, itemData);

		if (project->IsBaseShape(shape)) {
			outfitShapes->SetItemBold(item);
			outfitShapes->SetItemTextColour(item, wxColour(0, 255, 0));
		}

		auto it = std::find_if(prevStates.begin(), prevStates.end(), [&shape](const ShapeItemState& state) { return state.shapeName == shape->name.get(); });

		if (it != prevStates.end()) {
			outfitShapes->SetItemState(item, it->state);

			if (it->selected) {
				outfitShapes->SelectItem(item);
				selectedItems.push_back(itemData);

				if (!prevFirstSelItem.IsOk())
					prevFirstSelItem = item;
			}
		}

		if (!firstItem.IsOk())
			firstItem = item;
	}

	UnlockShapeSelect();

	if (prevFirstSelItem.IsOk())
		activeItem = (ShapeItemData*)outfitShapes->GetItemData(prevFirstSelItem);
	else if (firstItem.IsOk())
		outfitShapes->SelectItem(firstItem);
	else
		activeItem = nullptr;

	outfitShapes->ExpandAll();

	if (stashMasks) {
		auto maskStash = glView->StashMasks();
		MeshesFromProj();
		glView->UnstashMasks(maskStash);
	}
	else
		MeshesFromProj();

	UpdateAnimationGUI();

	if (outfitRoot.IsOk()) {
		wxTreeItemIdValue cookie;
		wxTreeItemId child = outfitShapes->GetFirstChild(outfitRoot, cookie);
		while (child.IsOk()) {
			bool vis = true;
			bool ghost = false;

			int state = outfitShapes->GetItemState(child);
			switch (state) {
				case 1:
					vis = false;
					ghost = false;
					break;
				case 2:
					vis = true;
					ghost = true;
					break;
				default:
					vis = true;
					ghost = false;
					break;
			}

			std::string shapeName{outfitShapes->GetItemText(child).ToUTF8()};
			glView->SetShapeGhostMode(shapeName, ghost);
			glView->ShowShape(shapeName, vis);
			child = outfitShapes->GetNextChild(outfitRoot, cookie);
		}
	}

	UpdateUndoTools();
	glView->UpdateFloor();

	if (render)
		glView->Render();
}

void OutfitStudioFrame::OnSSSNameCopy(wxCommandEvent& event) {
	wxWindow* win = ((wxButton*)event.GetEventObject())->GetParent();
	std::string copyStr{XRCCTRL(*win, "sssName", wxTextCtrl)->GetValue().ToUTF8()};

	project->ReplaceForbidden(copyStr);

	wxString defStr = wxString::FromUTF8(copyStr);
	wxString defSliderSetFile = defStr + ".osp";
	wxString defShapeDataDir = defStr;
	wxString defOutputFile = defStr + ".nif";

	wxFilePickerCtrl* fp = (wxFilePickerCtrl*)win->FindWindowByName("sssSliderSetFile");
	fp->SetPath(defSliderSetFile);

	wxDirPickerCtrl* dp = (wxDirPickerCtrl*)win->FindWindowByName("sssShapeDataFolder");
	dp->SetPath(defShapeDataDir);

	fp = (wxFilePickerCtrl*)win->FindWindowByName("sssShapeDataFile");
	fp->SetPath(defOutputFile);
}

void OutfitStudioFrame::OnSSSGenWeightsTrue(wxCommandEvent& event) {
	wxWindow* win = ((wxRadioButton*)event.GetEventObject())->GetParent();
	XRCCTRL(*win, "m_lowHighInfo", wxStaticText)->SetLabel("_0/_1.nif");
}

void OutfitStudioFrame::OnSSSGenWeightsFalse(wxCommandEvent& event) {
	wxWindow* win = ((wxRadioButton*)event.GetEventObject())->GetParent();
	XRCCTRL(*win, "m_lowHighInfo", wxStaticText)->SetLabel(".nif");
}

void OutfitStudioFrame::OnSaveSliderSet(wxCommandEvent& WXUNUSED(event)) {
	SaveProject();
}

void OutfitStudioFrame::OnSaveSliderSetAs(wxCommandEvent& WXUNUSED(event)) {
	SaveProjectAs();
}

void OutfitStudioFrame::OnSetBaseShape(wxCommandEvent& WXUNUSED(event)) {
	wxLogMessage("Setting new base shape.");
	SetBaseShape();
}

void OutfitStudioFrame::SetBaseShape() {
	for (auto& s : project->GetWorkNif()->GetShapes())
		UpdateShapeSource(s);

	ZeroSliders();
	if (!editor.activeSlider.empty()) {
		editor.bEditSlider = false;
		auto it = sliderPanels.find(editor.activeSlider);
		if (it != sliderPanels.end() && it->second)
			it->second->slider->SetFocus();
		HighlightSlider("");
		editor.activeSlider.clear();
	}
}

void OutfitStudioFrame::OnNPWizChangeSliderSetFile(wxFileDirPickerEvent& event) {
	std::string fn = event.GetPath().ToStdString();
	std::vector<std::string> shapes;
	wxWindow* npWiz = ((wxFilePickerCtrl*)event.GetEventObject())->GetParent();
	wxChoice* setNameChoice = (wxChoice*)XRCCTRL((*npWiz), "npSliderSetName", wxChoice);
	wxChoice* refShapeChoice = (wxChoice*)XRCCTRL((*npWiz), "npRefShapeName", wxChoice);
	XRCCTRL((*npWiz), "npRefIsSliderset", wxRadioButton)->SetValue(true);
	setNameChoice->Clear();
	refShapeChoice->Clear();

	if (fn.rfind(".osp") != std::string::npos || fn.rfind(".xml") != std::string::npos) {
		SliderSetFile ssf(fn);
		if (ssf.fail())
			return;

		std::vector<std::string> setNames;
		ssf.GetSetNames(setNames);

		for (auto& sn : setNames)
			setNameChoice->AppendString(sn);

		if (!setNames.empty()) {
			setNameChoice->SetSelection(0);
			ssf.SetShapes(setNames.front(), shapes);
			for (auto& rsn : shapes)
				refShapeChoice->AppendString(rsn);

			refShapeChoice->SetSelection(0);
		}
	}
	else if (fn.rfind(".nif") != std::string::npos) {
		std::fstream file;
		PlatformUtil::OpenFileStream(file, fn, std::ios::in | std::ios::binary);

		NifFile checkFile;
		if (checkFile.Load(file))
			return;

		for (auto& rsn : checkFile.GetShapeNames())
			refShapeChoice->AppendString(rsn);

		refShapeChoice->SetSelection(0);
	}
}

void OutfitStudioFrame::OnNPWizChangeSetNameChoice(wxCommandEvent& event) {
	wxWindow* npWiz = ((wxChoice*)event.GetEventObject())->GetParent();
	wxFilePickerCtrl* file = (wxFilePickerCtrl*)XRCCTRL((*npWiz), "npSliderSetFile", wxFilePickerCtrl);
	if (!file)
		return;

	std::string fn = file->GetPath().ToStdString();
	SliderSetFile ssf(fn);
	if (ssf.fail())
		return;

	std::vector<std::string> shapes;
	wxChoice* chooser = (wxChoice*)event.GetEventObject();
	ssf.SetShapes(chooser->GetStringSelection().ToStdString(), shapes);
	wxChoice* refShapeChoice = (wxChoice*)XRCCTRL((*npWiz), "npRefShapeName", wxChoice);
	refShapeChoice->Clear();

	for (auto& rsn : shapes)
		refShapeChoice->AppendString(rsn);

	refShapeChoice->SetSelection(0);
}

void OutfitStudioFrame::OnLoadOutfitFP_File(wxFileDirPickerEvent& event) {
	wxWindow* win = ((wxDialog*)event.GetEventObject())->GetParent();
	XRCCTRL((*win), "npWorkFile", wxRadioButton)->SetValue(true);
}

void OutfitStudioFrame::OnLoadOutfitFP_Texture(wxFileDirPickerEvent& event) {
	wxWindow* win = ((wxDialog*)event.GetEventObject())->GetParent();
	XRCCTRL((*win), "npTexFile", wxRadioButton)->SetValue(true);
}
