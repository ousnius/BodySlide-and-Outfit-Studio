/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "WeightCopyDialog.h"

#include "../program/OutfitProject.h"
#include "../program/OutfitStudio.h"

#include <wx/xrc/xmlres.h>

using namespace nifly;

WeightCopyDialog::WeightCopyDialog(wxWindow* parent,
								   OutfitProject* project,
								   wxGLPanel* glView,
								   PoseDataCollection& poseDataCollection,
								   const std::unordered_set<std::string>& normalizeBones,
								   const std::vector<NiShape*>& previewShapes,
								   WeightCopyOptions& options,
								   bool silent)
	: project(project)
	, glView(glView)
	, poseDataCollection(poseDataCollection)
	, normalizeBones(normalizeBones)
	, previewShapes(previewShapes)
	, options(options) {
	previewUndoState.undoType = UndoType::Weight;

	if (!wxXmlResource::Get()->LoadDialog(this, parent, "dlgCopyWeights"))
		return;

	SetupControls();
	PopulateBoneList();
	PopulatePoseDropdown();
	SetupEventHandlers();
	ShowSkinTransOptions();
	SavePoseState();

	// Enable bone display and weight colors on dialog open
	prevBonesMode = glView->IsBonesMode();
	prevWeightColors = glView->gls.GetWeightColors();
	glView->ShowBones(true);
	glView->gls.SetWeightColors(true);
	glView->Refresh();

	if (silent) {
		accepted = true;
		CollectOptions();
	}
	else {
		int result = ShowModal();

		UndoPreview();
		RestorePoseState();

		glView->ShowBones(prevBonesMode);
		glView->gls.SetWeightColors(prevWeightColors);
		static_cast<OutfitStudioFrame*>(parent)->RefreshGUIWeightColors();

		if (result == wxID_OK) {
			accepted = true;
			CollectOptions();
		}
	}
}

void WeightCopyDialog::SetupControls() {
	boneListBox = XRCCTRL(*this, "boneList", wxCheckListBox);
	poseChoice = XRCCTRL(*this, "poseChoice", wxChoice);
	previewProgress = XRCCTRL(*this, "previewProgress", wxGauge);

	XRCCTRL(*this, "proximityRadiusSlider", wxSlider)->Bind(wxEVT_SLIDER, [this](wxCommandEvent&) {
		float changed = XRCCTRL(*this, "proximityRadiusSlider", wxSlider)->GetValue() / 1000.0f;
		XRCCTRL(*this, "proximityRadiusText", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", changed));
	});

	XRCCTRL(*this, "proximityRadiusText", wxTextCtrl)->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
		float changed = atof(XRCCTRL(*this, "proximityRadiusText", wxTextCtrl)->GetValue().c_str());
		XRCCTRL(*this, "proximityRadiusSlider", wxSlider)->SetValue(changed * 1000);
	});

	XRCCTRL(*this, "maxResultsSlider", wxSlider)->Bind(wxEVT_SLIDER, [this](wxCommandEvent&) {
		int changed = XRCCTRL(*this, "maxResultsSlider", wxSlider)->GetValue();
		XRCCTRL(*this, "maxResultsText", wxTextCtrl)->ChangeValue(wxString::Format("%d", changed));
	});

	XRCCTRL(*this, "maxResultsText", wxTextCtrl)->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
		int changed = atol(XRCCTRL(*this, "maxResultsText", wxTextCtrl)->GetValue().c_str());
		XRCCTRL(*this, "maxResultsSlider", wxSlider)->SetValue(changed);
	});

	XRCCTRL(*this, "noTargetLimit", wxCheckBox)->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
		bool noTargetLimit = XRCCTRL(*this, "noTargetLimit", wxCheckBox)->IsChecked();
		XRCCTRL(*this, "maxResultsText", wxTextCtrl)->Enable(!noTargetLimit);
		XRCCTRL(*this, "maxResultsSlider", wxSlider)->Enable(!noTargetLimit);
	});
}

void WeightCopyDialog::PopulateBoneList() {
	NiShape* baseShape = project->GetBaseShape();
	if (baseShape) {
		std::vector<std::string> baseBones = project->GetWorkAnim()->shapeBones[baseShape->name.get()];
		std::sort(baseBones.begin(), baseBones.end());
		for (const auto& bone : baseBones) {
			int idx = boneListBox->Append(bone);
			boneListBox->Check(idx, true);
		}

		HighlightBonesWithSelectionWeights();
	}
}

void WeightCopyDialog::HighlightBonesWithSelectionWeights() {
	for (unsigned int i = 0; i < boneListBox->GetCount(); i++) {
		bool isChecked = boneListBox->IsChecked(i);
		std::string boneName = ExtractBoneNameFromListEntry(boneListBox->GetString(i));
		bool hasWeights = false;

		for (const auto* shape : previewShapes) {
			if (shape && project->GetWorkAnim()->HasWeights(shape->name.get(), boneName)) {
				hasWeights = true;
				break;
			}
		}

		wxString displayName = wxString::FromUTF8(boneName);

#ifdef __WXMSW__
		if (boneListBox->GetString(i) != displayName)
			boneListBox->SetString(i, displayName);

		if (wxOwnerDrawn* item = boneListBox->GetItem(i)) {
			item->SetTextColour(hasWeights ? wxColour(0, 200, 0) : wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
			boneListBox->RefreshItem(i);
		}
#else
		if (hasWeights)
			displayName += kWeightedBoneSuffix;

		if (boneListBox->GetString(i) != displayName)
			boneListBox->SetString(i, displayName);
#endif

		boneListBox->Check(i, isChecked);
	}
}

void WeightCopyDialog::PopulatePoseDropdown() {
	poseChoice->Append("<No Pose>");
	for (auto& pose : poseDataCollection.poseData)
		poseChoice->Append(wxString::FromUTF8(pose.name), &pose);
	poseChoice->SetSelection(0);
}

void WeightCopyDialog::SetupEventHandlers() {
	XRCCTRL(*this, "btnCheckAll", wxButton)->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		for (unsigned int i = 0; i < boneListBox->GetCount(); i++)
			boneListBox->Check(i, true);
	});

	XRCCTRL(*this, "btnUncheckAll", wxButton)->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		for (unsigned int i = 0; i < boneListBox->GetCount(); i++)
			boneListBox->Check(i, false);
	});

	poseChoice->Bind(wxEVT_CHOICE, &WeightCopyDialog::OnPoseSelected, this);
	boneListBox->Bind(wxEVT_LISTBOX, &WeightCopyDialog::OnBoneSelected, this);
	XRCCTRL(*this, "btnPreview", wxButton)->Bind(wxEVT_BUTTON, &WeightCopyDialog::OnPreview, this);

	Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& event) {
		if (event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_NUMPAD_ENTER)
			EndModal(wxID_OK);
		else
			event.Skip();
	});
}

void WeightCopyDialog::ShowSkinTransOptions() {
	wxCheckBox* cbCopySkinTrans = XRCCTRL(*this, "cbCopySkinTrans", wxCheckBox);
	wxCheckBox* cbTransformGeo = XRCCTRL(*this, "cbTransformGeo", wxCheckBox);
	if (options.showSkinTransOption) {
		cbCopySkinTrans->SetValue(options.doSkinTransCopy);
		cbTransformGeo->SetValue(options.doTransformGeo);
		cbCopySkinTrans->Show();
		cbTransformGeo->Show();
		XRCCTRL(*this, "copyTransDescription", wxStaticText)->Show();
	}
}

void WeightCopyDialog::SavePoseState() {
	prevBPose = project->bPose;

	std::vector<std::string> allBones;
	AnimSkeleton::getInstance().GetBoneNames(allBones);
	for (const auto& boneName : allBones) {
		AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
		if (bone)
			savedPose[boneName] = {bone->poseRotVec, bone->poseTranVec, bone->poseScale};
	}
}

void WeightCopyDialog::RestorePoseState() {
	std::vector<std::string> allBones;
	AnimSkeleton::getInstance().GetBoneNames(allBones);
	for (const auto& boneName : allBones) {
		AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
		if (!bone)
			continue;
		auto it = savedPose.find(boneName);
		if (it != savedPose.end()) {
			bone->poseRotVec = it->second.rotVec;
			bone->poseTranVec = it->second.tranVec;
			bone->poseScale = it->second.scale;
		}
		else {
			bone->poseRotVec = Vector3(0.0f, 0.0f, 0.0f);
			bone->poseTranVec = Vector3(0.0f, 0.0f, 0.0f);
			bone->poseScale = 1.0f;
		}
		bone->UpdatePoseTransform();
	}
	project->bPose = prevBPose;

	// Apply pose to meshes
	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		std::vector<Vector3> verts;
		project->GetLiveVerts(shape, verts);
		glView->UpdateMeshVertices(shape->name.get(), &verts, true, true, false);
	}
	glView->UpdateBones();
	glView->Render();

	static_cast<OutfitStudioFrame*>(GetParent())->PoseToGUI();
}

void WeightCopyDialog::OnPoseSelected(wxCommandEvent&) {
	int sel = poseChoice->GetSelection();
	if (sel == 0) {
		std::vector<std::string> bones;
		AnimSkeleton::getInstance().GetBoneNames(bones);
		for (const auto& boneName : bones) {
			AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
			if (bone) {
				bone->poseRotVec = Vector3(0.0f, 0.0f, 0.0f);
				bone->poseTranVec = Vector3(0.0f, 0.0f, 0.0f);
				bone->poseScale = 1.0f;
				bone->UpdatePoseTransform();
			}
		}
		project->bPose = false;
	}
	else {
		auto poseData = reinterpret_cast<PoseData*>(poseChoice->GetClientData(sel));
		poseData->ApplyToSkeleton();
		project->bPose = true;
	}

	// Apply pose to meshes
	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		std::vector<Vector3> verts;
		project->GetLiveVerts(shape, verts);
		glView->UpdateMeshVertices(shape->name.get(), &verts, true, true, false);
	}
	glView->UpdateBones();
	glView->Render();
}

void WeightCopyDialog::OnBoneSelected(wxCommandEvent&) {
	int sel = boneListBox->GetSelection();
	if (sel == wxNOT_FOUND)
		return;
	ShowBoneWeightColors(ExtractBoneNameFromListEntry(boneListBox->GetString(sel)));
}

std::string WeightCopyDialog::ExtractBoneNameFromListEntry(const wxString& entry) const {
	std::string name = entry.ToStdString();
	const size_t suffixPos = name.find(kWeightedBoneSuffix);
	if (suffixPos != std::string::npos)
		name.erase(suffixPos);
	return name;
}

void WeightCopyDialog::ShowBoneWeightColors(const std::string& boneName) {
	for (auto& s : project->GetWorkNif()->GetShapeNames()) {
		Mesh* m = glView->GetMesh(s);
		if (m) {
			m->WeightFill(0.0f);
			auto weights = project->GetWorkAnim()->GetWeightsPtr(s, boneName);
			if (weights) {
				for (auto& bw : *weights)
					m->weight[bw.first] = bw.second;
			}
		}
	}
	glView->Refresh();
}

void WeightCopyDialog::UndoPreview() {
	if (!previewActive)
		return;

	for (auto& uss : previewUndoState.usss) {
		Mesh* m = glView->GetMesh(uss.shapeName);
		if (!m)
			continue;

		for (auto& bw : uss.boneWeights) {
			if (bw.weights.empty())
				continue;

			project->GetWorkAnim()->AddShapeBone(m->shapeName, bw.boneName);
			auto weights = project->GetWorkAnim()->GetWeightsPtr(m->shapeName, bw.boneName);
			if (!weights)
				continue;

			for (auto& p : bw.weights) {
				if (p.second.startVal == 0.0f)
					weights->erase(p.first);
				else
					(*weights)[p.first] = p.second.startVal;
			}
		}

		if (project->bPose) {
			auto shape = project->GetWorkNif()->FindBlockByName<NiShape>(m->shapeName);
			if (shape) {
				std::vector<Vector3> verts;
				project->GetLiveVerts(shape, verts);
				glView->UpdateMeshVertices(shape->name.get(), &verts, true, true, false);
			}
		}
	}

	previewUndoState.usss.clear();
	previewActive = false;
	project->morpher.ClearProximityCache();
}

void WeightCopyDialog::ApplyPreviewWeights() {
	for (auto& uss : previewUndoState.usss) {
		Mesh* m = glView->GetMesh(uss.shapeName);
		if (!m)
			continue;

		for (auto& bw : uss.boneWeights) {
			if (bw.weights.empty())
				continue;

			project->GetWorkAnim()->AddShapeBone(m->shapeName, bw.boneName);
			auto weights = project->GetWorkAnim()->GetWeightsPtr(m->shapeName, bw.boneName);
			if (!weights)
				continue;

			for (auto& p : bw.weights) {
				if (p.second.endVal == 0.0f)
					weights->erase(p.first);
				else
					(*weights)[p.first] = p.second.endVal;
			}
		}

		if (project->bPose) {
			auto shape = project->GetWorkNif()->FindBlockByName<NiShape>(m->shapeName);
			if (shape) {
				std::vector<Vector3> verts;
				project->GetLiveVerts(shape, verts);
				glView->UpdateMeshVertices(shape->name.get(), &verts, true, true, false);
			}
		}
	}
}

void WeightCopyDialog::GetNormalizeBones(std::vector<std::string>* normBones, std::vector<std::string>* notNormBones) {
	std::vector<std::string> activeBones;
	project->GetActiveBones(activeBones);

	for (auto& boneName : activeBones) {
		if (normalizeBones.find(boneName) != normalizeBones.end()) {
			if (normBones)
				normBones->push_back(boneName);
		}
		else {
			if (notNormBones)
				notNormBones->push_back(boneName);
		}
	}
}

void WeightCopyDialog::OnPreview(wxCommandEvent&) {
	UndoPreview();

	float proximityRadius = atof(XRCCTRL(*this, "proximityRadiusText", wxTextCtrl)->GetValue().c_str());
	bool noTargetLimit = XRCCTRL(*this, "noTargetLimit", wxCheckBox)->IsChecked();
	int maxResults = noTargetLimit ? std::numeric_limits<int>::max() : atol(XRCCTRL(*this, "maxResultsText", wxTextCtrl)->GetValue().c_str());

	std::vector<std::string> checkedBones;
	for (unsigned int i = 0; i < boneListBox->GetCount(); i++) {
		if (boneListBox->IsChecked(i))
			checkedBones.push_back(ExtractBoneNameFromListEntry(boneListBox->GetString(i)));
	}

	if (checkedBones.empty() || previewShapes.empty())
		return;

	std::sort(checkedBones.begin(), checkedBones.end());

	int nCopyBones = static_cast<int>(checkedBones.size());
	std::vector<std::string> lockedBones;
	bool bSpreadWeight = false;

	std::unordered_set<std::string> selBones{checkedBones.begin(), checkedBones.end()};
	std::vector<std::string> normBones, notNormBones;
	GetNormalizeBones(&normBones, &notNormBones);

	for (auto& bone : normBones)
		if (!selBones.count(bone))
			checkedBones.push_back(bone);

	bSpreadWeight = static_cast<int>(checkedBones.size()) > nCopyBones;

	if (bSpreadWeight) {
		for (auto& bone : notNormBones)
			if (!selBones.count(bone))
				lockedBones.push_back(bone);
	}
	else {
		for (auto& bone : notNormBones)
			if (!selBones.count(bone))
				checkedBones.push_back(bone);
	}

	// Show progress gauge
	previewProgress->Show();
	previewProgress->SetValue(0);
	Layout();
	wxYield();

	std::unordered_map<uint16_t, float> mask;
	int numShapes = static_cast<int>(previewShapes.size());

	for (int i = 0; i < numShapes; i++) {
		auto shape = previewShapes[i];
		previewUndoState.usss.resize(previewUndoState.usss.size() + 1);
		previewUndoState.usss.back().shapeName = shape->name.get();

		mask.clear();
		glView->GetShapeMask(mask, shape->name.get());

		project->CopyBoneWeights(shape, proximityRadius, maxResults, mask, checkedBones, nCopyBones, lockedBones, previewUndoState.usss.back(), bSpreadWeight);

		previewProgress->SetValue((i + 1) * 100 / numShapes);
		wxYield();
	}

	ApplyPreviewWeights();
	previewActive = true;
	HighlightBonesWithSelectionWeights();

	project->morpher.ClearProximityCache();

	previewProgress->Hide();
	Layout();
	wxYield();

	int sel = boneListBox->GetSelection();
	if (sel != wxNOT_FOUND)
		ShowBoneWeightColors(ExtractBoneNameFromListEntry(boneListBox->GetString(sel)));
	else
		glView->Refresh();
}

void WeightCopyDialog::CollectOptions() {
	options.proximityRadius = atof(XRCCTRL(*this, "proximityRadiusText", wxTextCtrl)->GetValue().c_str());

	bool noTargetLimit = XRCCTRL(*this, "noTargetLimit", wxCheckBox)->IsChecked();
	if (!noTargetLimit)
		options.maxResults = atol(XRCCTRL(*this, "maxResultsText", wxTextCtrl)->GetValue().c_str());
	else
		options.maxResults = std::numeric_limits<int>::max();

	if (options.showSkinTransOption) {
		options.doSkinTransCopy = XRCCTRL(*this, "cbCopySkinTrans", wxCheckBox)->IsChecked();
		options.doTransformGeo = XRCCTRL(*this, "cbTransformGeo", wxCheckBox)->IsChecked();
	}

	options.selectedBones.clear();
	for (unsigned int i = 0; i < boneListBox->GetCount(); i++) {
		if (boneListBox->IsChecked(i))
			options.selectedBones.push_back(ExtractBoneNameFromListEntry(boneListBox->GetString(i)));
	}
}
