/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Outfit Studio window: posing the skeleton and pose files

#include "OutfitStudioFrame.h"
#include "FrameHelpers.h"
#include "GLPanel.h"
#include "OutfitStudioApp.h"
#include "physics/Controller.h"
#include "components/UndoHistory.h"
#include "utils/TargetGame.h"
#include "files/HkxFile.h"
#include "utils/ProjectUtil.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>
#include <wx/textctrl.h>

using namespace nifly;

namespace {

int GetPreferredPoseFileFilterIndex(TargetGame targetGame) {
	switch (targetGame) {
	case SKYRIMSE:
	case SKYRIMVR:
		return 2;
	case FO4:
	case FO4VR:
		return 1;
	default:
		return 0;
	}
}

wxString GetPoseFileExtensionForFilter(int filterIndex) {
	switch (filterIndex) {
	case 0: return "hkx";
	case 1: return "json";
	case 2: return "yaml";
	default: return wxEmptyString;
	}
}

wxString EnsurePoseFileExtension(const wxString& filePath, int filterIndex) {
	wxFileName fn(filePath);
	if (!fn.HasExt()) {
		wxString ext = GetPoseFileExtensionForFilter(filterIndex);
		if (!ext.empty())
			fn.SetExt(ext);
	}
	return fn.GetFullPath();
}

} // namespace

void OutfitStudioFrame::ApplyPose() {
	// While an animation plays there is no separate physics pump; the
	// simulation advances in lockstep with each displayed frame, using the
	// wall-clock time since the last step.
	if (physicsRunning && animPlaying)
		physics->Step(physicsClock.TakeElapsed());

	// Asking for a BVH update queues the shape with wxGLPanel::OnIdle, which
	// rebuilds a whole AABB tree over every triangle of it - far too expensive
	// to pay once per displayed animation frame. The tree only serves picking
	// and brush hit tests, both locked out during playback, so the rebuild is
	// deferred to the single ApplyPose that PauseAnimationPlayback does.
	const bool updateBVH = !animPlaying && !physicsPumpActive;

	for (auto& shape : project->GetWorkNif()->GetShapes())
		UpdateLiveMesh(shape, updateBVH);

	// Bone overlays cost one GL mesh created and destroyed per bone, not worth
	// paying for every frame while they are not being drawn.
	if (!animPlaying || glView->GetBonesMode() || glView->GetNodesMode())
		glView->UpdateBones();

	glView->Render();
}

AnimBone* OutfitStudioFrame::GetPoseBonePtr() {
	int selind = cPoseBone->GetSelection();
	if (selind == wxNOT_FOUND)
		return nullptr;
	std::string poseBone = cPoseBone->GetString(selind).ToStdString();
	return AnimSkeleton::getInstance().GetBonePtr(poseBone);
}

void OutfitStudioFrame::PoseToGUI() {
	AnimBone* bone = GetPoseBonePtr();
	if (bone) {
		rxPoseSlider->SetValue(bone->poseRotVec.x * 100);
		ryPoseSlider->SetValue(bone->poseRotVec.y * 100);
		rzPoseSlider->SetValue(bone->poseRotVec.z * 100);
		txPoseSlider->SetValue(bone->poseTranVec.x * 100);
		tyPoseSlider->SetValue(bone->poseTranVec.y * 100);
		tzPoseSlider->SetValue(bone->poseTranVec.z * 100);
		scPoseSlider->SetValue(std::log(bone->poseScale) / std::log(2) * 500);
		rxPoseText->ChangeValue(wxString() << bone->poseRotVec.x);
		ryPoseText->ChangeValue(wxString() << bone->poseRotVec.y);
		rzPoseText->ChangeValue(wxString() << bone->poseRotVec.z);
		txPoseText->ChangeValue(wxString() << bone->poseTranVec.x);
		tyPoseText->ChangeValue(wxString() << bone->poseTranVec.y);
		tzPoseText->ChangeValue(wxString() << bone->poseTranVec.z);
		scPoseText->ChangeValue(wxString() << bone->poseScale);
	}
	else {
		rxPoseSlider->SetValue(0);
		ryPoseSlider->SetValue(0);
		rzPoseSlider->SetValue(0);
		txPoseSlider->SetValue(0);
		tyPoseSlider->SetValue(0);
		tzPoseSlider->SetValue(0);
		scPoseSlider->SetValue(0);
		rxPoseText->ChangeValue("0");
		ryPoseText->ChangeValue("0");
		rzPoseText->ChangeValue("0");
		txPoseText->ChangeValue("0");
		tyPoseText->ChangeValue("0");
		tzPoseText->ChangeValue("0");
		scPoseText->ChangeValue("1");
	}
	if (project->bPose != cbPose->GetValue())
		cbPose->SetValue(project->bPose);
}

void OutfitStudioFrame::OnPoseBoneChanged(wxCommandEvent& WXUNUSED(event)) {
	PoseToGUI();
}

void OutfitStudioFrame::OnPoseValChanged(int cind, float val) {
	// Called when any pose slider or text control is changed.
	AnimBone* bone = GetPoseBonePtr();
	if (!bone)
		return;
	if (cind < 3)
		bone->poseRotVec[cind] = val;
	else if (cind < 6)
		bone->poseTranVec[cind - 3] = val;
	else
		bone->poseScale = val > 0 ? val : .0001f;

	bone->UpdatePoseTransform();
	ActivatePose(true);
}

void OutfitStudioFrame::OnAnyPoseSlider(wxScrollEvent& e, wxTextCtrl* t, int cind) {
	float val = (cind == 6) ?
		std::exp(e.GetPosition() * 0.002f * std::log(2)) :
		e.GetPosition() * 0.01f;
	t->ChangeValue(wxString() << val);
	OnPoseValChanged(cind, val);
}

// The pose controls of each pose value, in the order OnPoseValChanged numbers them
static const char* const PoseSliderNames[] = {"rxPoseSlider", "ryPoseSlider", "rzPoseSlider", "txPoseSlider", "tyPoseSlider", "tzPoseSlider", "scPoseSlider"};
static const char* const PoseTextNames[] = {"rxPoseText", "ryPoseText", "rzPoseText", "txPoseText", "tyPoseText", "tzPoseText", "scPoseText"};

void OutfitStudioFrame::OnPoseSlider(wxScrollEvent& event) {
	wxTextCtrl* texts[] = {rxPoseText, ryPoseText, rzPoseText, txPoseText, tyPoseText, tzPoseText, scPoseText};
	for (int i = 0; i < 7; i++) {
		if (event.GetId() == XRCID(PoseSliderNames[i])) {
			OnAnyPoseSlider(event, texts[i], i);
			return;
		}
	}
}

void OutfitStudioFrame::OnAnyPoseTextChanged(wxTextCtrl* t, wxSlider* s, int cind) {
	if (!t || !s)
		return;
	double val;
	if (!t->GetValue().ToDouble(&val))
		return;
	if (cind == 6)
		s->SetValue(std::log(val) / std::log(2) * 500);
	else
		s->SetValue(val * 100);
	OnPoseValChanged(cind, val);
}

void OutfitStudioFrame::OnPoseTextChanged(wxCommandEvent& event) {
	wxTextCtrl* texts[] = {rxPoseText, ryPoseText, rzPoseText, txPoseText, tyPoseText, tzPoseText, scPoseText};
	wxSlider* sliders[] = {rxPoseSlider, ryPoseSlider, rzPoseSlider, txPoseSlider, tyPoseSlider, tzPoseSlider, scPoseSlider};
	for (int i = 0; i < 7; i++) {
		if (event.GetId() == XRCID(PoseTextNames[i])) {
			OnAnyPoseTextChanged(texts[i], sliders[i], i);
			return;
		}
	}
}

void OutfitStudioFrame::OnResetBonePose(wxCommandEvent& WXUNUSED(event)) {
	AnimBone* bone = GetPoseBonePtr();
	if (!bone)
		return;
	bone->poseRotVec = Vector3(0, 0, 0);
	bone->poseTranVec = Vector3(0, 0, 0);
	bone->poseScale = 1.0f;
	bone->UpdatePoseTransform();
	PoseToGUI();
	ApplyPose();
}

void OutfitStudioFrame::OnResetAllPose(wxCommandEvent& WXUNUSED(event)) {
	wxMessageDialog dlg(this, _("Reset all bone poses?"), _("Reset Pose"), wxOK | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);
	dlg.SetOKCancelLabels(_("Reset"), _("Cancel"));
	if (dlg.ShowModal() != wxID_OK)
		return;

	ResetAllPoseBones();
	PoseToGUI();
	ApplyPose();
}

void OutfitStudioFrame::ResetAllPoseBones() {
	ProjectOperations(*project, editor).ResetPose();
}

void OutfitStudioFrame::OnPoseToMesh(wxCommandEvent& WXUNUSED(event)) {
	if (project->bPose) {
		UndoStateProject* usp = glView->GetUndoHistory()->PushState();
		project->ApplyPoseTransformsToAllShapeGeometry(*usp);

		std::vector<std::string> bones;
		AnimSkeleton::getInstance().GetBoneNames(bones);

		for (const std::string& boneName : bones) {
			AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
			if (!bone)
				continue;

			if (bone->IsUnposed())
				continue;

			bone->poseRotVec = Vector3(0.0f, 0.0f, 0.0f);
			bone->poseTranVec = Vector3(0.0f, 0.0f, 0.0f);
			bone->poseScale = 1.0f;
			bone->UpdatePoseTransform();
		}

		PoseToGUI();
		glView->UpdateBones();
		glView->ApplyUndoState(usp, false);
		SetPendingChanges();
	}
}

void OutfitStudioFrame::ActivatePose(bool checked) {
	if (cbPose->IsChecked() != checked)
		cbPose->SetValue(checked);

	project->bPose = checked;
	poseToMesh->Enable(checked);

	UpdatePhysicsState();
	ApplyPose();
}

void OutfitStudioFrame::OnPoseCheckBox(wxCommandEvent& e) {
	ActivatePose(e.IsChecked());
}

void OutfitStudioFrame::OnSelectPose(wxCommandEvent& WXUNUSED(event)) {
	// Selecting a pose takes over the skeleton from any selected animation.
	if (GetSelectedAnimation()) {
		PauseAnimationPlayback();
		animCurrentFrame = 0;
		cAnimationName->SetSelection(0); // "<None>"
		UpdateAnimationPlayerUI();
	}

	wxComboBox* cPoseName = (wxComboBox*)FindWindowByName("cPoseName");
	int poseSel = cPoseName->GetSelection();
	if (poseSel != wxNOT_FOUND) {
		auto poseData = reinterpret_cast<PoseData*>(cPoseName->GetClientData(poseSel));

		if (!poseData) {
			// "<New>" sentinel: reset all bones to an unposed state.
			ResetAllPoseBones();

			PoseToGUI();
			ApplyPose();
			UpdatePoseButtonStates();
			return;
		}

		poseData->ApplyToSkeleton();

		PoseToGUI();
		ActivatePose(true);

		UpdatePoseButtonStates();
	}
}

void OutfitStudioFrame::UpdatePoseButtonStates() {
	wxComboBox* cPoseName = (wxComboBox*)FindWindowByName("cPoseName");
	wxWindow* savePose = FindWindow(XRCID("savePose"));
	wxWindow* deletePose = FindWindow(XRCID("deletePose"));
	if (!cPoseName)
		return;

	bool enableSave = true;
	bool enableDelete = true;
	int poseSel = cPoseName->GetSelection();
	if (poseSel != wxNOT_FOUND) {
		auto poseData = reinterpret_cast<PoseData*>(cPoseName->GetClientData(poseSel));
		if (!poseData) {
			// "<New>" sentinel: nothing to delete.
			enableDelete = false;
		}
		else if (poseData->readOnly) {
			enableSave = false;
			enableDelete = false;
		}
	}

	if (savePose)
		savePose->Enable(enableSave);
	if (deletePose)
		deletePose->Enable(enableDelete);
}

void OutfitStudioFrame::OnSavePose(wxCommandEvent& WXUNUSED(event)) {
	wxComboBox* cPoseName = (wxComboBox*)FindWindowByName("cPoseName");

	wxString poseName = cPoseName->GetValue();
	poseName.Trim(true).Trim(false);
	if (poseName.empty() || poseName == "<New>") {
		wxMessageBox(_("Please enter a name for the pose."), _("Save Pose"), wxICON_INFORMATION);
		return;
	}

	std::string error = SaveCurrentPose(poseName.ToUTF8().data());
	if (!error.empty())
		wxMessageBox(wxString::FromUTF8(error), _("Save Pose"), wxICON_ERROR);
}

void OutfitStudioFrame::OnDeletePose(wxCommandEvent& WXUNUSED(event)) {
	wxComboBox* cPoseName = (wxComboBox*)FindWindowByName("cPoseName");
	int poseSel = cPoseName->GetSelection();
	if (poseSel != wxNOT_FOUND) {
		auto poseData = reinterpret_cast<PoseData*>(cPoseName->GetClientData(poseSel));
		if (!poseData)
			return; // "<New>" sentinel, nothing to delete
		if (poseData->readOnly)
			return;

		wxString prompt = wxString::Format(_("Are you sure you wish to delete the pose '%s'?"), cPoseName->GetStringSelection());
		int result = wxMessageBox(prompt, _("Confirm pose delete"), wxYES_NO | wxICON_WARNING, this);
		if (result != wxYES)
			return;

		wxString fileName = wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/PoseData/" + wxString::FromUTF8(PoseDataCollection::SanitizeFileStem(poseData->name).c_str()) + ".xml";
		wxRemoveFile(fileName);

		cPoseName->Delete(poseSel);
		cPoseName->SetStringSelection("<New>");
		UpdatePoseButtonStates();
	}
}

void OutfitStudioFrame::OnSaveHkxPose(wxCommandEvent& WXUNUSED(event)) {
	wxString defaultFileStem = "pose";
	if (wxComboBox* cPoseName = (wxComboBox*)FindWindowByName("cPoseName")) {
		wxString poseName = cPoseName->GetValue();
		poseName.Trim(true).Trim(false);
		if (!poseName.empty() && poseName != "<New>")
			defaultFileStem = poseName;
	}
	defaultFileStem = wxString::FromUTF8(PoseDataCollection::SanitizeFileStem(std::string(defaultFileStem.ToUTF8().data())).c_str());

	wxFileDialog saveDlg(this,
					 _("Save pose file"),
					 wxEmptyString,
					 defaultFileStem,
					 "HKX pose files (*.hkx)|*.hkx|SAM JSON pose files (*.json)|*.json|SAM YAML pose files (*.yaml;*.yml)|*.yaml;*.yml",
					 wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	saveDlg.SetFilterIndex(GetPreferredPoseFileFilterIndex(wxGetApp().targetGame));
	if (saveDlg.ShowModal() == wxID_CANCEL)
		return;

	wxString savePath = EnsurePoseFileExtension(saveDlg.GetPath(), saveDlg.GetFilterIndex());
	std::string error = SaveCurrentPoseFile(savePath.ToUTF8().data());
	if (!error.empty()) {
		wxMessageBox(wxString::FromUTF8(error), _("Save Pose File"), wxOK | wxICON_ERROR, this);
		return;
	}

	if (statusBar)
		statusBar->SetStatusText(_("Pose file saved."), 0);
}

std::string OutfitStudioFrame::SaveCurrentPose(const std::string& poseName) {
	wxComboBox* cPoseName = (wxComboBox*)FindWindowByName("cPoseName");
	int existingSel = cPoseName ? cPoseName->FindString(wxString::FromUTF8(poseName)) : wxNOT_FOUND;

	PoseData* poseData = nullptr;
	if (existingSel != wxNOT_FOUND)
		poseData = reinterpret_cast<PoseData*>(cPoseName->GetClientData(existingSel));
	if (!poseData) {
		for (auto& pose : poseDataCollection.poseData)
			if (pose.name == poseName)
				poseData = &pose;
	}

	if (poseData && poseData->readOnly)
		return "The pose '" + poseName + "' is read-only.";

	// Kept in the collection, so the pose list can point at it
	if (!poseData)
		poseData = poseDataCollection.AddPose(PoseData(poseName));

	poseData->boneData.clear();
	poseData->absoluteLocal = false;

	std::vector<std::string> bones;
	AnimSkeleton::getInstance().GetBoneNames(bones);

	for (const auto& boneName : bones) {
		AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
		if (!bone)
			continue;

		if (bone->IsUnposed())
			continue;

		PoseBoneData poseBoneData{};
		poseBoneData.name = bone->boneName;
		poseBoneData.rotation = bone->poseRotVec;
		poseBoneData.translation = bone->poseTranVec;
		poseBoneData.scale = bone->poseScale;
		poseData->boneData.push_back(poseBoneData);
	}

	if (cPoseName) {
		if (existingSel != wxNOT_FOUND) {
			cPoseName->SetClientData(existingSel, poseData);
			cPoseName->SetSelection(existingSel);
		}
		else {
			int poseSel = cPoseName->Append(wxString::FromUTF8(poseName), poseData);
			cPoseName->SetSelection(poseSel);
		}
	}

	wxString dirName = wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/PoseData";
	wxFileName::Mkdir(dirName, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	wxString fileName = dirName + "/" + wxString::FromUTF8(PoseDataCollection::SanitizeFileStem(poseData->name).c_str()) + ".xml";

	PoseDataFile poseDataFile;
	poseDataFile.New(fileName.ToUTF8().data());

	std::vector<PoseData> poses;
	poses.push_back(*poseData);

	poseDataFile.SetData(poses);
	poseDataFile.Save();

	UpdatePoseButtonStates();
	return {};
}

std::string OutfitStudioFrame::SaveCurrentPoseFile(const std::string& fileName) {
	PoseFileFormat format = PoseDataCollection::GetPoseFileFormat(fileName);
	std::string skeletonHkxPath;
	HKX::Format hkxFormat = HKX::Format::Unknown;

	if (format == PoseFileFormat::Hkx) {
		if (!GetPoseHkxFormat(wxGetApp().targetGame, &hkxFormat))
			return _("Saving HKX poses is currently only supported for Skyrim Legendary Edition, Skyrim Special Edition, Skyrim VR, Fallout 4 and Fallout 4 VR.").ToUTF8().data();

		wxString error;
		if (!PoseDataCollection::FindReferenceSkeletonHkx(skeletonHkxPath, error))
			return error.ToUTF8().data();
	}
	else if (format != PoseFileFormat::Json && format != PoseFileFormat::Yaml)
		return _("Please save the pose with a .hkx, .json, .yaml or .yml extension.").ToUTF8().data();

	PoseData pd;
	PoseDataCollection::CaptureCurrentPose(wxFileName(wxString::FromUTF8(fileName)).GetName().ToUTF8().data(), format != PoseFileFormat::Yaml, pd);
	if (pd.boneData.empty())
		return _("No skeleton bones are available to export a pose.").ToUTF8().data();

	std::string saveError;
	if (!PoseDataCollection::SavePoseFile(fileName, pd, skeletonHkxPath, hkxFormat, &saveError))
		return saveError.empty() ? std::string(_("Failed to save the pose file.").ToUTF8().data()) : saveError;

	return {};
}

void OutfitStudioFrame::OnLoadHkxPose(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog loadDlg(this,
						 _("Select pose file"),
						 wxEmptyString,
						 wxEmptyString,
						 "HKX pose files (*.hkx)|*.hkx|SAM JSON pose files (*.json)|*.json|SAM YAML pose files (*.yaml;*.yml)|*.yaml;*.yml",
						 wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	loadDlg.SetFilterIndex(GetPreferredPoseFileFilterIndex(wxGetApp().targetGame));
	if (loadDlg.ShowModal() == wxID_CANCEL)
		return;

	wxString srcPath = loadDlg.GetPath();
	PoseFileFormat format = PoseDataCollection::GetPoseFileFormat(std::string(srcPath.ToUTF8().data()));
	std::string skeletonHkxPath;

	if (format == PoseFileFormat::Hkx) {
		if (!GetReferenceSkeletonHkxPath(skeletonHkxPath, _("Load Pose File")))
			return;
	}
	else if (format != PoseFileFormat::Json && format != PoseFileFormat::Yaml) {
		wxMessageBox(_("Please choose a pose file with a .hkx, .json, .yaml or .yml extension."), _("Load Pose File"), wxOK | wxICON_ERROR, this);
		return;
	}

	PoseData pd;
	std::string loadError;
	if (!PoseDataCollection::LoadPoseFile(std::string(srcPath.ToUTF8().data()), pd, skeletonHkxPath, &loadError)) {
		wxString message = loadError.empty() ? _("Failed to load the pose file.") : wxString::FromUTF8(loadError);
		wxMessageBox(message, _("Load Pose File"), wxOK | wxICON_ERROR, this);
		return;
	}

	wxComboBox* cPoseName = (wxComboBox*)FindWindowByName("cPoseName");
	if (!cPoseName)
		return;

	auto makeUniquePoseName = [cPoseName](const std::string& baseName) {
		wxString uniqueName = wxString::FromUTF8(baseName);
		if (cPoseName->FindString(uniqueName) == wxNOT_FOUND)
			return uniqueName;

		for (int suffix = 1;; ++suffix) {
			wxString candidate = wxString::Format("%s (%d)", uniqueName, suffix);
			if (cPoseName->FindString(candidate) == wxNOT_FOUND)
				return candidate;
		}
	};

	int existingSel = cPoseName->FindString(wxString::FromUTF8(pd.name));
	if (existingSel != wxNOT_FOUND) {
		auto existing = reinterpret_cast<PoseData*>(cPoseName->GetClientData(existingSel));
		if (existing && !existing->readOnly) {
			existing->boneData = std::move(pd.boneData);
			existing->absoluteLocal = pd.absoluteLocal;
			cPoseName->SetSelection(existingSel);
		}
		else {
			pd.name = std::string(makeUniquePoseName(pd.name).ToUTF8().data());
			PoseData* added = poseDataCollection.AddPose(std::move(pd));
			int idx = cPoseName->Append(wxString::FromUTF8(added->name), added);
			cPoseName->SetSelection(idx);
		}
	}
	else {
		PoseData* added = poseDataCollection.AddPose(std::move(pd));
		int idx = cPoseName->Append(wxString::FromUTF8(added->name), added);
		cPoseName->SetSelection(idx);
	}

	wxCommandEvent dummy;
	OnSelectPose(dummy);
	if (statusBar)
		statusBar->SetStatusText(_("Pose file loaded."), 0);
}

bool OutfitStudioFrame::GetReferenceSkeletonHkxPath(std::string& outPath, const wxString& caption) {
	if (!GetPoseHkxFormat(wxGetApp().targetGame, nullptr)) {
		wxMessageBox(_("HKX support is currently only available for Skyrim Legendary Edition, Skyrim Special Edition, Skyrim VR, Fallout 4 and Fallout 4 VR."),
					 caption,
					 wxOK | wxICON_INFORMATION,
					 this);
		return false;
	}

	wxString error;
	if (PoseDataCollection::FindReferenceSkeletonHkx(outPath, error))
		return true;

	wxMessageBox(error, caption, wxOK | wxICON_ERROR, this);
	return false;
}
