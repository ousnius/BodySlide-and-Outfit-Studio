/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../components/Automation.h"

#include <cstddef>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <wx/arrstr.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/collpane.h>
#include <wx/clrpicker.h>
#include <wx/combo.h>
#include <wx/combobox.h>
#include <wx/dialog.h>
#include <wx/event.h>
#include <wx/filepicker.h>
#include <wx/gauge.h>
#include <wx/listctrl.h>
#include <wx/log.h>
#include <wx/panel.h>
#include <wx/radiobox.h>
#include <wx/simplebook.h>
#include <wx/stattext.h>
#include <wx/statusbr.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/window.h>

class OutfitStudioFrame;
class OutfitProject;
class AutomationStepTypePopup;

namespace nifly {
class NiShape;
}

class AutomationDialog : public wxDialog {
public:
	AutomationDialog(OutfitStudioFrame* outfitStudio, OutfitProject* project);
	~AutomationDialog() override;

	// Run an automation script without showing the dialog. Returns 0 on success,
	// non-zero on failure (script not found, no active steps, batch errors, etc.).
	// `batchInputs` are positional CLI args: file paths or directories for FolderScan,
	// slider set project names for SliderSets. Ignored for non-batch scripts.
	int RunHeadless(const wxString& scriptName, const wxArrayString& batchInputs);

private:
	OutfitStudioFrame* outfitStudio = nullptr;
	OutfitProject* project = nullptr;
	AutomationScript script;
	int selectedStep = -1;
	int varRowCount = 1;

	// Variables defined by Set Variable steps while a script runs. Substituted into
	// every following step, on top of the up front pass over the placeholder table.
	std::map<std::string, std::string> runtimeVariables;
	std::map<std::string, std::string> runBaseVariables;

	wxListCtrl* listSteps = nullptr;
	wxStaticText* lblStepsPlaceholder = nullptr;
	wxSimplebook* bookStepPages = nullptr;
	wxComboCtrl* comboStepType = nullptr;
	AutomationStepTypePopup* stepTypePopup = nullptr;
	std::map<AutomationStepType, int> stepTypePageIndex;
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
	bool headlessMode = false;
	int lastRunErrors = 0;

	struct ShaderPropertyRowControls {
		wxPanel* panel = nullptr;
		std::string propertyName;
		wxTextCtrl* value1 = nullptr;
		wxTextCtrl* value2 = nullptr;
		wxColourPickerCtrl* color = nullptr;
		wxChoice* choice = nullptr;
	};
	std::vector<ShaderPropertyRowControls> shaderPropertyRows;

	struct GeometryPropertyRowControls {
		wxPanel* panel = nullptr;
		std::string propertyName;
		wxChoice* value = nullptr;
	};
	std::vector<GeometryPropertyRowControls> geometryPropertyRows;

	struct TexturePathRowControls {
		wxPanel* panel = nullptr;
		int index = -1;
		std::string name;
		wxTextCtrl* path = nullptr;
	};
	std::vector<TexturePathRowControls> texturePathRows;

	// UI helper methods
	void SetCheckboxValue(const char* name, bool value);
	bool GetCheckboxValue(const char* name) const;
	void SetTextValue(const char* name, const std::string& value);
	void SetTextValue(const char* name, const wxString& value);
	std::string GetTextValue(const char* name) const;
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

	// Generic marshalling driven by the step type's field table. Everything a
	// plain control can express is handled here; the rest lives in the per-type
	// UI hooks below, wired up in the step binding table.
	void ApplyFieldsToUI(const AutomationStep& step);
	void ReadFieldsFromUI(AutomationStep& step);

	struct StepBinding {
		AutomationStepType type;
		int (AutomationDialog::*execute)(const AutomationStep&) = nullptr;
		void (AutomationDialog::*toUI)(const AutomationStep&) = nullptr;
		void (AutomationDialog::*fromUI)(AutomationStep&) = nullptr;
	};
	static const StepBinding* FindStepBinding(AutomationStepType type);

	void ShowStepTypePage(AutomationStepType type);
	void SetStepTypeSelection(AutomationStepType type);

	void StepToUILoadReference(const AutomationStep& step);
	void StepFromUILoadReference(AutomationStep& step);
	void StepToUIAddProject(const AutomationStep& step);
	void StepFromUIAddProject(AutomationStep& step);
	void StepToUIImportFile(const AutomationStep& step);
	void StepFromUIImportFile(AutomationStep& step);
	void StepToUIImportSliderData(const AutomationStep& step);
	void StepFromUIImportSliderData(AutomationStep& step);
	void StepToUIExportFile(const AutomationStep& step);
	void StepFromUIExportFile(AutomationStep& step);
	void StepToUISaveProject(const AutomationStep& step);
	void StepToUISetReferenceShape(const AutomationStep& step);
	void StepToUISetExtraData(const AutomationStep& step);
	void StepFromUISetExtraData(AutomationStep& step);
	void StepToUILoadMask(const AutomationStep& step);
	void StepFromUILoadMask(AutomationStep& step);
	void StepToUISetSliderProperties(const AutomationStep& step);
	void StepFromUISetSliderProperties(AutomationStep& step);
	void StepToUISetShaderProperties(const AutomationStep& step);
	void StepFromUISetShaderProperties(AutomationStep& step);
	void StepToUISetGeometryProperties(const AutomationStep& step);
	void StepFromUISetGeometryProperties(AutomationStep& step);
	void StepToUISetTexturePaths(const AutomationStep& step);
	void StepFromUISetTexturePaths(AutomationStep& step);

	void RefreshStepRow(int index);
	void ShowStepSettings(bool show);
	void UpdateButtonState();

	std::string GetAutomationsFolder();
	void PopulateAutomationList();
	void CollectScripts(const wxString& baseFolder, const wxString& currentFolder, std::vector<std::pair<wxString, wxString>>& entries);
	static bool IsSeparatorItem(const wxString& text);
	void LoadAutomation(const wxString& name);
	static wxString SanitizePath(const wxString& name);

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
	void PopulateShaderPropertyChoice();
	void ClearShaderPropertyRows();
	void AddShaderPropertyRow(const AutomationStep::ShaderProperty& prop);
	void RemoveShaderPropertyRow(wxWindow* rowPanel);
	void RebuildShaderPropertyRows(const std::vector<AutomationStep::ShaderProperty>& properties);
	std::vector<AutomationStep::ShaderProperty> ReadShaderPropertyRows() const;
	void PopulateGeometryPropertyChoice();
	void ClearGeometryPropertyRows();
	void AddGeometryPropertyRow(const AutomationStep::GeometryProperty& prop);
	void RemoveGeometryPropertyRow(wxWindow* rowPanel);
	void RebuildGeometryPropertyRows(const std::vector<AutomationStep::GeometryProperty>& properties);
	std::vector<AutomationStep::GeometryProperty> ReadGeometryPropertyRows() const;
	void PopulateTexturePathChoice();
	void ClearTexturePathRows();
	void AddTexturePathRow(const AutomationStep::TexturePath& path);
	void RemoveTexturePathRow(wxWindow* rowPanel);
	void RebuildTexturePathRows(const std::vector<AutomationStep::TexturePath>& paths);
	std::vector<AutomationStep::TexturePath> ReadTexturePathRows() const;

	std::map<std::string, std::string> CollectVariables();
	void PopulateVariablesUI();
	void SyncBatchUIFromScript();
	void SyncBatchScriptFromUI();
	void UpdateBatchPanelVisibility();

	void ResetAndClearProject();
	static int TexturePathIndexForName(const std::string& name);
	static int ResolveTexturePathIndex(const AutomationStep::TexturePath& path);
	static std::string TexturePathNameForIndex(int index);
	static bool StepChangesSliderSet(AutomationStepType type);

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
	int ExecuteStepChangePartitions(const AutomationStep& step);
	int ExecuteStepMirrorShape(const AutomationStep& step);
	int ExecuteStepRecalcNormals(const AutomationStep& step);
	int ExecuteStepLoadMask(const AutomationStep& step);
	int ExecuteStepClearMask(const AutomationStep& step);
	int ExecuteStepSetSliderProperties(const AutomationStep& step);
	int ExecuteStepSetShaderProperties(const AutomationStep& step);
	int ExecuteStepSetGeometryProperties(const AutomationStep& step);
	int ExecuteStepSetExtraData(const AutomationStep& step);
	int ExecuteStepDeleteExtraData(const AutomationStep& step);
	int ExecuteStepSetTexturePaths(const AutomationStep& step);
	int ExecuteStepRemoveUnusedNodes(const AutomationStep& step);
	int ExecuteStepFixClipping(const AutomationStep& step);
	int ExecuteStepFixBadBones(const AutomationStep& step);
	int ExecuteStepAddBone(const AutomationStep& step);
	int ExecuteStepTransferWeights(const AutomationStep& step);
	int ExecuteStepSetBoneTransform(const AutomationStep& step);
	int ExecuteStepMakeConversionRef(const AutomationStep& step);
	int ExecuteStepCopySegPart(const AutomationStep& step);
	int ExecuteStepDeleteVertices(const AutomationStep& step);
	int ExecuteStepSeparateVertices(const AutomationStep& step);
	int ExecuteStepMergeGeometry(const AutomationStep& step);
	int ExecuteStepSymmetrizeVertices(const AutomationStep& step);
	int ExecuteStepClearSliderData(const AutomationStep& step);
	int ExecuteStepCloneSlider(const AutomationStep& step);
	int ExecuteStepNegateSlider(const AutomationStep& step);
	int ExecuteStepNewCombinedSlider(const AutomationStep& step);
	int ExecuteStepNewZapSlider(const AutomationStep& step);
	int ExecuteStepGrowShrinkMask(const AutomationStep& step);
	int ExecuteStepInvertMask(const AutomationStep& step);
	int ExecuteStepMaskAsymmetric(const AutomationStep& step);
	int ExecuteStepMaskBoneWeighted(const AutomationStep& step);
	int ExecuteStepMaskSliderAffected(const AutomationStep& step);
	int ExecuteStepMaskWeighted(const AutomationStep& step);
	int ExecuteStepSaveMask(const AutomationStep& step);
	int ExecuteStepSetVariable(const AutomationStep& step);
	int ExecuteStepLogMessage(const AutomationStep& step);

	// Rebuilds the render meshes after a step changed vertex or triangle counts,
	// carrying the remapped masks over to the new meshes.
	void RefreshMeshesWithMasks(std::unordered_map<std::string, std::vector<float>>& maskStash);

	std::vector<std::string> GatherBatchFiles();
	std::vector<std::pair<std::string, std::string>> GatherBatchSliderSets();

	bool ShowCheckableListDialog(const wxString& title, const wxString& labelText, const wxArrayString& items, std::vector<size_t>& checkedIndices);

	nifly::NiShape* FindShapeByName(const std::string& name);
	std::vector<nifly::NiShape*> ResolveTargetShapes(const AutomationStep& step, bool includeBaseShapeOnEmpty = true);

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
	void OnStepTypeChanged(AutomationStepType type);
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
	void OnAddShaderProperty(wxCommandEvent& event);
	void OnAddGeometryProperty(wxCommandEvent& event);
	void OnAddTexturePath(wxCommandEvent& event);
	void OnBatchModeChanged(wxCommandEvent& event);
	void OnCharHook(wxKeyEvent& event);
	void OnAddShapeToField(wxCommandEvent& event);
	void OnAddSliderToField(wxCommandEvent& event);
	void AppendFromList(const char* textCtrlName, const wxArrayString& items, const wxString& title);

	wxDECLARE_EVENT_TABLE();
};
