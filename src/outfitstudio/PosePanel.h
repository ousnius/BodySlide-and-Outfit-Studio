/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <string>
#include <wx/event.h>
#include <wx/string.h>

class AnimBone;
class OutfitStudioFrame;
class wxButton;
class wxCheckBox;
class wxChoice;
class wxScrollEvent;
class wxSlider;
class wxTextCtrl;

// The pose pane of an Outfit Studio window: posing the bones of the skeleton by hand, and the pose list
// with its pose files. The poses themselves are the window's poseDataCollection.
class PosePanel : public wxEvtHandler {
public:
	explicit PosePanel(OutfitStudioFrame& frame);

	// Finds the pose controls in the window and binds their events
	void Init();

	// The bone the pose sliders edit, which the bone list fills
	wxChoice* GetBoneChoice() const { return cPoseBone; }
	bool IsShowPoseChecked() const;
	// Shows whether the project is posed in the Show Pose box and the Pose to Mesh button
	void ShowProjectPoseState();

	// Puts the transform of the selected bone into the pose sliders and fields
	void PoseToGUI();
	void ResetAllPoseBones();
	void ActivatePose(bool checked);
	// Updates enabled state of the Save/Delete pose buttons based on
	// whether the currently selected pose is read-only (e.g. SAM YAML).
	void UpdatePoseButtonStates();
	// Resolves the .hkx file next to the configured reference skeleton, showing
	// an error dialog with the given caption on failure.
	bool GetReferenceSkeletonHkxPath(std::string& outPath, const wxString& caption);

	// Saves the current skeleton pose to the pose list and the PoseData folder, replacing a pose of the
	// same name unless it's read-only. Returns the error, or an empty string.
	std::string SaveCurrentPose(const std::string& poseName);
	// Saves the current skeleton pose to a .hkx, .json, .yaml or .yml file. HKX needs the Havok skeleton
	// next to the reference skeleton. Returns the error, or an empty string.
	std::string SaveCurrentPoseFile(const std::string& fileName);

private:
	AnimBone* GetPoseBonePtr();
	void OnPoseBoneChanged(wxCommandEvent& event);
	void OnPoseValChanged(int cind, float val);
	void OnAnyPoseSlider(wxScrollEvent& e, wxTextCtrl* t, int cind);
	void OnPoseSlider(wxScrollEvent& event);
	void OnAnyPoseTextChanged(wxTextCtrl* t, wxSlider* s, int cind);
	void OnPoseTextChanged(wxCommandEvent& event);
	void OnResetBonePose(wxCommandEvent& event);
	void OnResetAllPose(wxCommandEvent& event);
	void OnPoseToMesh(wxCommandEvent& event);
	void OnPoseCheckBox(wxCommandEvent& event);
	void OnSelectPose(wxCommandEvent& event);
	void OnSavePose(wxCommandEvent& event);
	void OnDeletePose(wxCommandEvent& event);
	void OnSaveHkxPose(wxCommandEvent& event);
	void OnLoadHkxPose(wxCommandEvent& event);

	OutfitStudioFrame& frame;

	wxChoice* cPoseBone = nullptr;
	wxSlider* rxPoseSlider = nullptr;
	wxSlider* ryPoseSlider = nullptr;
	wxSlider* rzPoseSlider = nullptr;
	wxSlider* txPoseSlider = nullptr;
	wxSlider* tyPoseSlider = nullptr;
	wxSlider* tzPoseSlider = nullptr;
	wxSlider* scPoseSlider = nullptr;
	wxTextCtrl* rxPoseText = nullptr;
	wxTextCtrl* ryPoseText = nullptr;
	wxTextCtrl* rzPoseText = nullptr;
	wxTextCtrl* txPoseText = nullptr;
	wxTextCtrl* tyPoseText = nullptr;
	wxTextCtrl* tzPoseText = nullptr;
	wxTextCtrl* scPoseText = nullptr;
	wxCheckBox* cbPose = nullptr;
	wxButton* poseToMesh = nullptr;
};
