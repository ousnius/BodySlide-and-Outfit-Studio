/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../components/Automation.h"

#include <wx/collpane.h>
#include <wx/combobox.h>
#include <wx/filepicker.h>
#include <wx/gauge.h>
#include <wx/listctrl.h>
#include <wx/log.h>
#include <wx/radiobox.h>
#include <wx/simplebook.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

class OutfitStudioFrame;
class OutfitProject;

namespace nifly {
class NiShape;
}

class AutomationDialog : public wxDialog {
public:
	AutomationDialog(OutfitStudioFrame* outfitStudio, OutfitProject* project);
	~AutomationDialog() override;

private:
	OutfitStudioFrame* outfitStudio = nullptr;
	OutfitProject* project = nullptr;
	AutomationScript script;
	int selectedStep = -1;
	int varRowCount = 1;

	wxListCtrl* listSteps = nullptr;
	wxStaticText* lblStepsPlaceholder = nullptr;
	wxSimplebook* bookStepPages = nullptr;
	wxChoice* choiceStepType = nullptr;
	wxCheckBox* chkActive = nullptr;
	wxTextCtrl* txtTargetMeshes = nullptr;
	wxCheckBox* chkTargetRegex = nullptr;
	wxTextCtrl* txtNote = nullptr;
	wxRadioBox* radioBatchMode = nullptr;
	wxPanel* panelStepSettings = nullptr;
	wxComboBox* cmbAutomation = nullptr;
	wxButton* btnSaveScript = nullptr;
	wxButton* btnExecuteAll = nullptr;
	wxButton* btnClose = nullptr;
	wxStatusBar* statusBar = nullptr;
	wxGauge* progressBar = nullptr;
	wxCollapsiblePane* paneOutput = nullptr;
	wxTextCtrl* txtOutput = nullptr;
	wxLog* oldLogTarget = nullptr;
	bool cancelRequested = false;
	bool isExecuting = false;

	// UI helper methods
	void SetCheckboxValue(const char* name, bool value);
	bool GetCheckboxValue(const char* name) const;
	void SetTextValue(const char* name, const std::string& value);
	std::string GetTextValue(const char* name) const;
	void SetFloatValue(const char* name, float value);
	float GetFloatValue(const char* name) const;
	int GetIntValue(const char* name) const;
	void SetVectorValue(const char* name, const std::vector<std::string>& values);
	std::vector<std::string> GetVectorValue(const char* name) const;

	// Progress methods
	void StartProgress(const wxString& msg = "");
	void UpdateProgress(int val, const wxString& msg = "");
	void EndProgress(const wxString& msg = "");
	void SetExecutionUIState(bool running);

	void PopulateStepList();
	void SelectStep(int index);
	void UpdateStepFromUI();
	void UpdateUIFromStep(const AutomationStep& step);
	void RefreshStepRow(int index);
	void ShowStepSettings(bool show);
	void UpdateButtonState();

	std::string GetAutomationsFolder();
	void PopulateAutomationList();
	void LoadAutomation(const wxString& name);
	static wxString SanitizeFileName(const wxString& name);

	void PopulateRefTemplates();
	void PopulateSetsFromFile(const wxString& filePath, const char* choiceName, const char* shapesChoiceName = nullptr);
	void PopulateRefShapesForSet(const wxString& filePath, const wxString& setName);
	wxString MakeRelativeToProject(const wxString& absolutePath) const;
	wxString MakeAbsoluteToProject(const wxString& path) const;
	AutomationBatchMode GetSelectedBatchMode() const;
	bool IsBatchMode(AutomationBatchMode mode) const;
	void ApplyBatchModeDefaults(AutomationStep& step) const;
	void UpdateSaveProjectBatchModeUI(const AutomationStep& step);
	void UpdateExportFileBatchModeUI(const AutomationStep& step);
	void UpdateImportFolderVisibility(bool fromFolder);
	void UpdateSliderDataFolderVisibility(bool fromFolder);
	void UpdateSaveFieldsEnabled(bool useOriginal);
	void UpdateExportFieldsEnabled(bool useOriginal);
	void UpdateSetRefFieldsEnabled(bool enabled);
	void UpdateExportForBatchMode();

	std::map<std::string, std::string> CollectVariables();
	void PopulateVariablesUI();
	void SyncBatchUIFromScript();
	void SyncBatchScriptFromUI();
	void UpdateBatchPanelVisibility();

	void ResetAndClearProject();

	void ExecuteSteps(const std::vector<size_t>& stepIndices);
	void ExecuteBatch(const std::vector<size_t>& stepIndices, const std::vector<std::string>& selectedFiles = {}, const std::vector<std::pair<std::string, std::string>>& selectedSets = {});
	int ExecuteStep(const AutomationStep& step);

	int ExecuteStepClearProject(const AutomationStep& step);
	int ExecuteStepLoadReference(const AutomationStep& step);
	int ExecuteStepAddProject(const AutomationStep& step);
	int ExecuteStepSetSliderValues(const AutomationStep& step);
	int ExecuteStepConformSliders(const AutomationStep& step);
	int ExecuteStepCopyBoneWeights(const AutomationStep& step);
	int ExecuteStepSetBaseShape(const AutomationStep& step);
	int ExecuteStepClearReference(const AutomationStep& step);
	int ExecuteStepTransformShape(const AutomationStep& step);
	int ExecuteStepInvertUVs(const AutomationStep& step);
	int ExecuteStepDeleteBones(const AutomationStep& step);
	int ExecuteStepAddCustomBone(const AutomationStep& step);
	int ExecuteStepEditBone(const AutomationStep& step);
	int ExecuteStepRemoveSkinning(const AutomationStep& step);
	int ExecuteStepApplyPose(const AutomationStep& step);
	int ExecuteStepImportSliderData(const AutomationStep& step);
	int ExecuteStepImportFile(const AutomationStep& step);
	int ExecuteStepDeleteShape(const AutomationStep& step);
	int ExecuteStepRenameShape(const AutomationStep& step);
	int ExecuteStepSaveProject(const AutomationStep& step);
	int ExecuteStepExportFile(const AutomationStep& step);
	int ExecuteStepRefineMesh(const AutomationStep& step);
	int ExecuteStepDeleteSlider(const AutomationStep& step);
	int ExecuteStepSetReferenceShape(const AutomationStep& step);
	int ExecuteStepResetTransforms(const AutomationStep& step);
	int ExecuteStepDuplicateShape(const AutomationStep& step);
	int ExecuteStepMirrorShape(const AutomationStep& step);
	int ExecuteStepLoadMask(const AutomationStep& step);
	int ExecuteStepClearMask(const AutomationStep& step);
	int ExecuteStepSetSliderProperties(const AutomationStep& step);
	int ExecuteStepRemoveUnusedNodes(const AutomationStep& step);
	int ExecuteStepFixClipping(const AutomationStep& step);

	std::vector<std::string> GatherBatchFiles();
	std::vector<std::pair<std::string, std::string>> GatherBatchSliderSets();

	bool ShowCheckableListDialog(const wxString& title, const wxString& labelText, const wxArrayString& items, std::vector<size_t>& checkedIndices);

	nifly::NiShape* FindShapeByName(const std::string& name);
	std::vector<nifly::NiShape*> ResolveTargetShapes(const AutomationStep& step);

	void OnSaveScript(wxCommandEvent& event);
	void OnDeleteScript(wxCommandEvent& event);
	void OnOpenFolder(wxCommandEvent& event);
	void OnAutomationSelected(wxCommandEvent& event);
	void OnAddStep(wxCommandEvent& event);
	void OnDuplicateStep(wxCommandEvent& event);
	void OnRemoveStep(wxCommandEvent& event);
	void OnMoveUp(wxCommandEvent& event);
	void OnMoveDown(wxCommandEvent& event);
	void OnStepSelected(wxListEvent& event);
	void OnStepListKeyDown(wxKeyEvent& event);
	void OnStepListContextMenu(wxContextMenuEvent& event);
	void OnStepTypeChanged(wxCommandEvent& event);
	void OnExecuteAll(wxCommandEvent& event);
	void OnExecuteSelected(wxCommandEvent& event);
	void OnClose(wxCommandEvent& event);
	void OnWindowClose(wxCloseEvent& event);
	void OnAddVariable(wxCommandEvent& event);
	void OnRemoveVariable(wxCommandEvent& event);
	void OnRefTemplateChanged(wxCommandEvent& event);
	void OnRefSourceFileChanged(wxFileDirPickerEvent& event);
	void OnRefSetChanged(wxCommandEvent& event);
	void OnAddProjSourceFileChanged(wxFileDirPickerEvent& event);
	void OnAddProjSetChanged(wxCommandEvent& event);
	void OnImportFolderChanged(wxCommandEvent& event);
	void OnSliderDataFolderChanged(wxCommandEvent& event);
	void OnSaveUseOriginalChanged(wxCommandEvent& event);
	void OnExportUseOriginalChanged(wxCommandEvent& event);
	void OnSetRefUnsetChanged(wxCommandEvent& event);
	void OnLoadMaskFileChanged(wxFileDirPickerEvent& event);
	void PopulateMaskNamesFromFile(const wxString& filePath);
	void OnSliderPropZapChanged(wxCommandEvent& event);
	void UpdateSliderPropDefaultVisibility();
	void OnBatchModeChanged(wxCommandEvent& event);
	void OnCharHook(wxKeyEvent& event);
	void OnAddShapeToField(wxCommandEvent& event);
	void OnAddSliderToField(wxCommandEvent& event);
	void AppendFromList(const char* textCtrlName, const wxArrayString& items, const wxString& title);

	wxDECLARE_EVENT_TABLE();
};
