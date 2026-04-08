/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../components/Anim.h"
#include "../components/PoseData.h"
#include "../components/UndoState.h"

#include <wx/checklst.h>
#include <wx/gauge.h>
#include <wx/wx.h>

class OutfitProject;
class wxGLPanel;

struct WeightCopyOptions {
	float proximityRadius = 0.0f;
	int maxResults = 0;
	bool showSkinTransOption = false;
	bool doSkinTransCopy = false;
	bool doTransformGeo = false;
	std::vector<std::string> selectedBones;
};

class WeightCopyDialog : public wxDialog {
public:
	WeightCopyDialog(wxWindow* parent,
					 OutfitProject* project,
					 wxGLPanel* glView,
					 PoseDataCollection& poseDataCollection,
					 const std::unordered_set<std::string>& normalizeBones,
					 const std::vector<nifly::NiShape*>& previewShapes,
					 WeightCopyOptions& options,
					 bool silent);

	bool GetResult() const { return accepted; }

private:
	void SetupControls();
	void PopulateBoneList();
	void PopulatePoseDropdown();
	void SetupEventHandlers();
	void ShowSkinTransOptions();
	void SavePoseState();
	void RestorePoseState();

	void OnPreview(wxCommandEvent& event);
	void UndoPreview();
	void ApplyPreviewWeights();
	void ShowBoneWeightColors(const std::string& boneName);

	void OnPoseSelected(wxCommandEvent& event);
	void OnBoneSelected(wxCommandEvent& event);

	void CollectOptions();

	OutfitProject* project = nullptr;
	wxGLPanel* glView = nullptr;
	PoseDataCollection& poseDataCollection;
	const std::unordered_set<std::string>& normalizeBones;
	const std::vector<nifly::NiShape*>& previewShapes;
	WeightCopyOptions& options;

	wxCheckListBox* boneListBox = nullptr;
	wxChoice* poseChoice = nullptr;
	wxGauge* previewProgress = nullptr;

	bool accepted = false;
	bool previewActive = false;
	UndoStateProject previewUndoState;

	// Saved state for restoration on close
	bool prevBonesMode = false;
	bool prevWeightColors = false;
	bool prevBPose = false;

	struct SavedBonePose {
		nifly::Vector3 rotVec;
		nifly::Vector3 tranVec;
		float scale;
	};
	std::unordered_map<std::string, SavedBonePose> savedPose;

	// Normalization helpers (need access to frame)
	void GetNormalizeBones(std::vector<std::string>* normBones, std::vector<std::string>* notNormBones);
};
