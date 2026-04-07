/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "AutomationDialog.h"

#include "OutfitProject.h"
#include "OutfitStudio.h"

#include "../files/MaskFile.h"
#include "../files/TriFile.h"
#include "../utils/PlatformUtil.h"

#include <NifFile.hpp>

#include <wx/dir.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/filename.h>

#include <regex>

#include "../components/SliderSet.h"

using namespace nifly;

extern ConfigurationManager Config;
extern ConfigurationManager OutfitStudioConfig;

wxBEGIN_EVENT_TABLE(AutomationDialog, wxDialog)
	EVT_COMBOBOX(XRCID("cmbAutomation"), AutomationDialog::OnAutomationSelected)
	EVT_BUTTON(XRCID("btnSaveScript"), AutomationDialog::OnSaveScript)
	EVT_BUTTON(XRCID("btnDeleteScript"), AutomationDialog::OnDeleteScript)
	EVT_BUTTON(XRCID("btnOpenFolder"), AutomationDialog::OnOpenFolder)
	EVT_LIST_ITEM_SELECTED(XRCID("listSteps"), AutomationDialog::OnStepSelected)
	EVT_CHOICE(XRCID("choiceStepType"), AutomationDialog::OnStepTypeChanged)
	EVT_BUTTON(XRCID("btnExecuteAll"), AutomationDialog::OnExecuteAll)
	EVT_BUTTON(wxID_CLOSE, AutomationDialog::OnClose)
	EVT_BUTTON(XRCID("btnAddVariable"), AutomationDialog::OnAddVariable)
	EVT_BUTTON(XRCID("btnRemoveVariable"), AutomationDialog::OnRemoveVariable)
	EVT_CHOICE(XRCID("choiceRefTemplate"), AutomationDialog::OnRefTemplateChanged)
	EVT_FILEPICKER_CHANGED(XRCID("fpRefSourceFile"), AutomationDialog::OnRefSourceFileChanged)
	EVT_CHOICE(XRCID("choiceRefSet"), AutomationDialog::OnRefSetChanged)
	EVT_FILEPICKER_CHANGED(XRCID("fpAddProjSourceFile"), AutomationDialog::OnAddProjSourceFileChanged)
	EVT_CHOICE(XRCID("choiceAddProjSet"), AutomationDialog::OnAddProjSetChanged)
	EVT_CHECKBOX(XRCID("chkImportFromFolder"), AutomationDialog::OnImportFolderChanged)
	EVT_CHECKBOX(XRCID("chkSliderDataFromFolder"), AutomationDialog::OnSliderDataFolderChanged)
	EVT_CHECKBOX(XRCID("chkSaveUseOriginal"), AutomationDialog::OnSaveUseOriginalChanged)
	EVT_CHECKBOX(XRCID("chkExportUseOriginalPath"), AutomationDialog::OnExportUseOriginalChanged)
	EVT_FILEPICKER_CHANGED(XRCID("fpLoadMaskFile"), AutomationDialog::OnLoadMaskFileChanged)
	EVT_CHOICE(XRCID("choiceSliderPropZap"), AutomationDialog::OnSliderPropZapChanged)
	EVT_RADIOBOX(XRCID("radioBatchMode"), AutomationDialog::OnBatchModeChanged)
wxEND_EVENT_TABLE()

AutomationDialog::AutomationDialog(OutfitStudioFrame* outfitStudio, OutfitProject* project)
	: outfitStudio(outfitStudio), project(project) {
	wxXmlResource* xrc = wxXmlResource::Get();
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/Automation.xrc");
	xrc->LoadDialog(this, outfitStudio, "dlgAutomation");

	listSteps = XRCCTRL(*this, "listSteps", wxListCtrl);
	bookStepPages = XRCCTRL(*this, "bookStepPages", wxSimplebook);
	choiceStepType = XRCCTRL(*this, "choiceStepType", wxChoice);

	// Verify the number of step type pages matches the enum count
	if (bookStepPages && choiceStepType) {
		wxASSERT_MSG(bookStepPages->GetPageCount() == AutomationStepTypeCount,
			"bookStepPages page count must match AutomationStepTypeCount");
		wxASSERT_MSG(choiceStepType->GetCount() == AutomationStepTypeCount,
			"choiceStepType item count must match AutomationStepTypeCount");
	}

	chkActive = XRCCTRL(*this, "chkActive", wxCheckBox);
	txtTargetMeshes = XRCCTRL(*this, "txtTargetMeshes", wxTextCtrl);
	chkTargetRegex = XRCCTRL(*this, "chkTargetRegex", wxCheckBox);
	txtNote = XRCCTRL(*this, "txtNote", wxTextCtrl);
	radioBatchMode = XRCCTRL(*this, "radioBatchMode", wxRadioBox);
	panelStepSettings = XRCCTRL(*this, "panelStepSettings", wxPanel);
	cmbAutomation = XRCCTRL(*this, "cmbAutomation", wxComboBox);
	btnSaveScript = XRCCTRL(*this, "btnSaveScript", wxButton);
	btnExecuteAll = XRCCTRL(*this, "btnExecuteAll", wxButton);

	listSteps->InsertColumn(0, _("Active"), wxLIST_FORMAT_CENTER, 65);
	listSteps->InsertColumn(1, _("Type"), wxLIST_FORMAT_LEFT, 165);
	listSteps->InsertColumn(2, _("Target"), wxLIST_FORMAT_LEFT, 100);
	listSteps->InsertColumn(3, _("Note"), wxLIST_FORMAT_LEFT, 250);

	listSteps->Bind(wxEVT_CONTEXT_MENU, &AutomationDialog::OnStepListContextMenu, this);
	listSteps->Bind(wxEVT_KEY_DOWN, &AutomationDialog::OnStepListKeyDown, this);

	// Hide step settings until a step is selected
	if (panelStepSettings)
		panelStepSettings->Hide();

	// Collapse panes by default
	auto* paneVariables = XRCCTRL(*this, "paneVariables", wxCollapsiblePane);
	auto* paneBatch = XRCCTRL(*this, "paneBatch", wxCollapsiblePane);
	if (paneVariables)
		paneVariables->Collapse(true);
	if (paneBatch)
		paneBatch->Collapse(true);

	// Output log pane (collapsed by default, shown during execution)
	paneOutput = XRCCTRL(*this, "paneOutput", wxCollapsiblePane);
	if (paneOutput) {
		paneOutput->Collapse(true);
		auto* paneWin = paneOutput->GetPane();
		auto* paneSizer = new wxBoxSizer(wxVERTICAL);
		txtOutput = new wxTextCtrl(paneWin, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, 200), wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxHSCROLL);
		txtOutput->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
		paneSizer->Add(txtOutput, 1, wxEXPAND);
		paneWin->SetSizer(paneSizer);
	}

	outfitStudio->UpdateReferenceTemplates();
	PopulateRefTemplates();
	UpdateBatchPanelVisibility();
	PopulateAutomationList();
	UpdateButtonState();

	// Restore last selected automation script
	std::string lastScript = OutfitStudioConfig["AutomationDialog.lastScript"];
	if (!lastScript.empty()) {
		wxString name = wxString::FromUTF8(lastScript);
		if (cmbAutomation && cmbAutomation->FindString(name) != wxNOT_FOUND) {
			cmbAutomation->SetValue(name);
			LoadAutomation(name);
		}
	}

	// Add status bar at the bottom
	statusBar = new wxStatusBar(this, wxID_ANY);
	int widths[] = {-1, 150};
	statusBar->SetFieldsCount(2, widths);
	statusBar->SetStatusText(_("Ready."));
	GetSizer()->Add(statusBar, 0, wxEXPAND);

	Fit();
	SetMinSize(GetSize());
	SetSize(wxSize(std::max(GetSize().GetWidth(), 950), std::max(GetSize().GetHeight(), 680)));
	CenterOnParent();
}

AutomationDialog::~AutomationDialog() {
	// Save last selected automation script
	wxString name = cmbAutomation ? cmbAutomation->GetValue().Trim().Trim(false) : wxString();
	OutfitStudioConfig.SetValue("AutomationDialog.lastScript", name.ToUTF8().data());
	OutfitStudioConfig.SaveConfig(Config["AppDir"] + "/OutfitStudio.xml", "OutfitStudioConfig");

	wxXmlResource::Get()->Unload(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/Automation.xrc");
}

// UI helper methods

void AutomationDialog::SetCheckboxValue(const char* name, bool value) {
	auto* chk = XRCCTRL(*this, name, wxCheckBox);
	if (chk)
		chk->SetValue(value);
}

bool AutomationDialog::GetCheckboxValue(const char* name) const {
	auto* chk = XRCCTRL(*this, name, wxCheckBox);
	return chk ? chk->GetValue() : false;
}

void AutomationDialog::SetTextValue(const char* name, const std::string& value) {
	auto* txt = XRCCTRL(*this, name, wxTextCtrl);
	if (txt)
		txt->SetValue(wxString::FromUTF8(value));
}

std::string AutomationDialog::GetTextValue(const char* name) const {
	auto* txt = XRCCTRL(*this, name, wxTextCtrl);
	return txt ? std::string(txt->GetValue().ToUTF8().data()) : "";
}

AutomationBatchMode AutomationDialog::GetSelectedBatchMode() const {
	if (!radioBatchMode)
		return AutomationBatchMode::None;

	return static_cast<AutomationBatchMode>(radioBatchMode->GetSelection());
}

bool AutomationDialog::IsBatchMode(AutomationBatchMode mode) const {
	return GetSelectedBatchMode() == mode;
}

void AutomationDialog::ApplyBatchModeDefaults(AutomationStep& step) const {
	if (step.type == AutomationStepType::SaveProject) {
		if (IsBatchMode(AutomationBatchMode::SliderSets)) {
			step.saveUseOriginal = true;
			step.saveCopyRefFromProject = true;
		}
		else if (IsBatchMode(AutomationBatchMode::FolderScan)) {
			step.saveUseOriginal = false;
		}
	}
	else if (step.type == AutomationStepType::ExportFile && IsBatchMode(AutomationBatchMode::FolderScan)) {
		step.exportUseOriginalPath = true;
	}
}

void AutomationDialog::UpdateSaveProjectBatchModeUI(const AutomationStep& step) {
	auto* chkUseOrig = XRCCTRL(*this, "chkSaveUseOriginal", wxCheckBox);
	if (chkUseOrig) {
		if (IsBatchMode(AutomationBatchMode::SliderSets)) {
			chkUseOrig->SetValue(true);
			chkUseOrig->Enable(false);
		}
		else if (IsBatchMode(AutomationBatchMode::FolderScan)) {
			chkUseOrig->SetValue(false);
			chkUseOrig->Enable(false);
		}
		else {
			chkUseOrig->Enable(true);
		}
	}

	bool fieldsEnabled = IsBatchMode(AutomationBatchMode::SliderSets) ? false :
		(IsBatchMode(AutomationBatchMode::FolderScan) ? true : !step.saveUseOriginal);
	UpdateSaveFieldsEnabled(fieldsEnabled);
}

void AutomationDialog::UpdateExportFileBatchModeUI(const AutomationStep& step) {
	auto* chkUseOrig = XRCCTRL(*this, "chkExportUseOriginalPath", wxCheckBox);
	if (chkUseOrig) {
		if (IsBatchMode(AutomationBatchMode::FolderScan)) {
			chkUseOrig->SetValue(true);
			chkUseOrig->Enable(false);
		}
		else {
			chkUseOrig->Enable(true);
		}
	}

	UpdateExportFieldsEnabled(!step.exportUseOriginalPath);
}

void AutomationDialog::SetFloatValue(const char* name, float value) {
	auto* txt = XRCCTRL(*this, name, wxTextCtrl);
	if (txt)
		txt->SetValue(wxString::Format("%.5g", value));
}

float AutomationDialog::GetFloatValue(const char* name) const {
	auto* txt = XRCCTRL(*this, name, wxTextCtrl);
	return txt ? static_cast<float>(atof(txt->GetValue().c_str())) : 0.0f;
}

int AutomationDialog::GetIntValue(const char* name) const {
	auto* txt = XRCCTRL(*this, name, wxTextCtrl);
	return txt ? atoi(txt->GetValue().c_str()) : 0;
}

void AutomationDialog::SetVectorValue(const char* name, const std::vector<std::string>& values) {
	auto* txt = XRCCTRL(*this, name, wxTextCtrl);
	if (txt)
		txt->SetValue(wxString::FromUTF8(JoinStrings(values, ", ")));
}

std::vector<std::string> AutomationDialog::GetVectorValue(const char* name) const {
	auto* txt = XRCCTRL(*this, name, wxTextCtrl);
	return txt ? SplitCommaSeparated(std::string(txt->GetValue().ToUTF8().data())) : std::vector<std::string>();
}

// Progress methods

void AutomationDialog::StartProgress(const wxString& msg) {
	if (progressBar)
		return;

	wxRect rect;
	statusBar->GetFieldRect(1, rect);
	progressBar = new wxGauge(statusBar, wxID_ANY, 10000, rect.GetPosition(), rect.GetSize());

	statusBar->SetStatusText(msg.IsEmpty() ? _("Starting...") : msg);

	// Redirect log output to the output pane
	if (paneOutput && txtOutput) {
		txtOutput->Clear();
		paneOutput->Collapse(false);
		GetSizer()->Layout();
		oldLogTarget = wxLog::SetActiveTarget(new wxLogTextCtrl(txtOutput));
	}
}

void AutomationDialog::UpdateProgress(int val, const wxString& msg) {
	if (!progressBar)
		return;

	int scaled = val * 100;
	if (scaled > 10000)
		scaled = 10000;
	progressBar->SetValue(scaled);
	statusBar->SetStatusText(msg);
	wxYield();
}

void AutomationDialog::EndProgress(const wxString& msg) {
	if (!progressBar)
		return;

	progressBar->SetValue(10000);
	delete progressBar;
	progressBar = nullptr;
	statusBar->SetStatusText(msg.IsEmpty() ? _("Ready.") : msg);

	// Restore previous log target
	if (oldLogTarget) {
		delete wxLog::SetActiveTarget(oldLogTarget);
		oldLogTarget = nullptr;
	}
}

void AutomationDialog::PopulateStepList() {
	listSteps->DeleteAllItems();
	auto& steps = script.GetSteps();

	for (size_t i = 0; i < steps.size(); i++) {
		long idx = listSteps->InsertItem(i, steps[i].active ? wxString(L"\u2713") : wxString(""));
		listSteps->SetItem(idx, 1, wxString::FromUTF8(AutomationStepTypeToString(steps[i].type)));

		std::string targetStr = JoinStrings(steps[i].targetMeshes, ", ");
		if (targetStr.empty())
			targetStr = "(all)";
		listSteps->SetItem(idx, 2, wxString::FromUTF8(targetStr));

		wxString noteExcerpt = wxString::FromUTF8(steps[i].note);
		if (noteExcerpt.length() > 60)
			noteExcerpt = noteExcerpt.Left(57) + "...";
		listSteps->SetItem(idx, 3, noteExcerpt);
	}
}

void AutomationDialog::RefreshStepRow(int index) {
	if (index < 0 || index >= static_cast<int>(script.GetSteps().size()))
		return;

	auto& step = script.GetSteps()[index];
	listSteps->SetItem(index, 0, step.active ? wxString(L"\u2713") : wxString(""));
	listSteps->SetItem(index, 1, wxString::FromUTF8(AutomationStepTypeToString(step.type)));

	std::string targetStr = JoinStrings(step.targetMeshes, ", ");
	if (targetStr.empty())
		targetStr = "(all)";
	listSteps->SetItem(index, 2, wxString::FromUTF8(targetStr));

	wxString noteExcerpt = wxString::FromUTF8(step.note);
	if (noteExcerpt.length() > 60)
		noteExcerpt = noteExcerpt.Left(57) + "...";
	listSteps->SetItem(index, 3, noteExcerpt);
}

void AutomationDialog::ShowStepSettings(bool show) {
	if (!panelStepSettings)
		return;

	panelStepSettings->Show(show);
	GetSizer()->Layout();

	if (show) {
		wxSize minSize = GetSizer()->GetMinSize();
		wxSize curSize = GetSize();
		if (minSize.GetWidth() > curSize.GetWidth() || minSize.GetHeight() > curSize.GetHeight()) {
			SetSize(wxSize(std::max(curSize.GetWidth(), minSize.GetWidth()), std::max(curSize.GetHeight(), minSize.GetHeight())));
		}
	}
}

void AutomationDialog::OnStepListContextMenu(wxContextMenuEvent& WXUNUSED(event)) {
	enum {
		ID_CTX_ADD_STEP = wxID_HIGHEST + 100,
		ID_CTX_REMOVE_STEP,
		ID_CTX_MOVE_UP,
		ID_CTX_MOVE_DOWN,
		ID_CTX_EXECUTE_SELECTED
	};

	wxMenu menu;
	menu.Append(ID_CTX_ADD_STEP, _("Add Step"));

	bool hasSelection = (selectedStep >= 0);
	menu.Append(ID_CTX_REMOVE_STEP, _("Remove Step"))->Enable(hasSelection);
	menu.AppendSeparator();
	menu.Append(ID_CTX_MOVE_UP, _("Move Up"))->Enable(hasSelection && selectedStep > 0);
	menu.Append(ID_CTX_MOVE_DOWN, _("Move Down"))->Enable(hasSelection && selectedStep < static_cast<int>(script.GetSteps().size()) - 1);
	menu.AppendSeparator();
	menu.Append(ID_CTX_EXECUTE_SELECTED, _("Execute Selected"))->Enable(hasSelection);

	int result = GetPopupMenuSelectionFromUser(menu);
	wxCommandEvent evt;
	switch (result) {
		case ID_CTX_ADD_STEP: OnAddStep(evt); break;
		case ID_CTX_REMOVE_STEP: OnRemoveStep(evt); break;
		case ID_CTX_MOVE_UP: OnMoveUp(evt); break;
		case ID_CTX_MOVE_DOWN: OnMoveDown(evt); break;
		case ID_CTX_EXECUTE_SELECTED: OnExecuteSelected(evt); break;
	}
}

void AutomationDialog::SelectStep(int index) {
	if (selectedStep >= 0 && selectedStep < static_cast<int>(script.GetSteps().size()))
		UpdateStepFromUI();

	selectedStep = index;

	if (index < 0 || index >= static_cast<int>(script.GetSteps().size())) {
		selectedStep = -1;
		return;
	}

	ShowStepSettings(true);
	UpdateUIFromStep(script.GetSteps()[index]);
}

void AutomationDialog::UpdateUIFromStep(const AutomationStep& step) {
	chkActive->SetValue(step.active);
	txtNote->SetValue(wxString::FromUTF8(step.note));

	txtTargetMeshes->SetValue(wxString::FromUTF8(JoinStrings(step.targetMeshes, ", ")));

	if (chkTargetRegex)
		chkTargetRegex->SetValue(step.targetRegex);

	int typeIndex = static_cast<int>(step.type);
	choiceStepType->SetSelection(typeIndex);
	bookStepPages->SetSelection(typeIndex);

	switch (step.type) {
		case AutomationStepType::ClearProject:
			// No parameters to set
			break;

		case AutomationStepType::LoadReference: {
			auto* fp = XRCCTRL(*this, "fpRefSourceFile", wxFilePickerCtrl);
			if (fp) {
				// If path is relative, resolve it for the file picker display
				wxString path = wxString::FromUTF8(step.refSourceFile);
				if (!path.IsEmpty()) {
					wxFileName fn(path);
					if (fn.IsRelative()) {
						std::string projPath = GetProjectPath();
						fn.MakeAbsolute(wxString::FromUTF8(projPath));
					}
					fp->SetPath(fn.GetFullPath());
				}
				else {
					fp->SetPath(wxEmptyString);
				}

				// Populate set/shape dropdowns from the file
				PopulateSetsFromFile(fp->GetPath(), "choiceRefSet", "choiceRefShape");
			}

			auto* choiceSet = XRCCTRL(*this, "choiceRefSet", wxChoice);
			if (choiceSet) {
				wxString setName = wxString::FromUTF8(step.refSet);
				int idx = choiceSet->FindString(setName);
				if (idx != wxNOT_FOUND) {
					choiceSet->SetSelection(idx);
				}
				else if (!setName.IsEmpty()) {
					choiceSet->Append(setName);
					choiceSet->SetSelection(choiceSet->GetCount() - 1);
				}

				// Populate shapes for the selected set
				if (fp)
					PopulateRefShapesForSet(fp->GetPath(), choiceSet->GetStringSelection());
			}

			auto* choiceShape = XRCCTRL(*this, "choiceRefShape", wxChoice);
			if (choiceShape) {
				wxString shapeName = wxString::FromUTF8(step.refShape);
				int idx = choiceShape->FindString(shapeName);
				if (idx != wxNOT_FOUND)
					choiceShape->SetSelection(idx);
				else if (!shapeName.IsEmpty()) {
					choiceShape->Append(shapeName);
					choiceShape->SetSelection(choiceShape->GetCount() - 1);
				}
			}

			// Select the matching template if any
			auto* choiceTemplate = XRCCTRL(*this, "choiceRefTemplate", wxChoice);
			if (choiceTemplate)
				choiceTemplate->SetSelection(0); // "(None)" by default

			SetCheckboxValue("chkRefLoadAll", step.refLoadAll);
			SetCheckboxValue("chkRefMergeSliders", step.refMergeSliders);
			SetCheckboxValue("chkRefMergeZaps", step.refMergeZaps);
			SetCheckboxValue("chkRefAppendNewSliders", step.refAppendNewSliders);
			break;
		}
		case AutomationStepType::AddProject: {
			auto* fp = XRCCTRL(*this, "fpAddProjSourceFile", wxFilePickerCtrl);
			if (fp) {
				wxString path = wxString::FromUTF8(step.refSourceFile);
				if (!path.IsEmpty()) {
					wxFileName fn(path);
					if (fn.IsRelative()) {
						std::string projPath = GetProjectPath();
						fn.MakeAbsolute(wxString::FromUTF8(projPath));
					}
					fp->SetPath(fn.GetFullPath());
				}
				else {
					fp->SetPath(wxEmptyString);
				}
				PopulateSetsFromFile(fp->GetPath(), "choiceAddProjSet");
			}

			auto* choiceSet = XRCCTRL(*this, "choiceAddProjSet", wxChoice);
			if (choiceSet) {
				wxString setName = wxString::FromUTF8(step.refSet);
				int idx = choiceSet->FindString(setName);
				if (idx != wxNOT_FOUND)
					choiceSet->SetSelection(idx);
				else if (!setName.IsEmpty()) {
					choiceSet->Append(setName);
					choiceSet->SetSelection(choiceSet->GetCount() - 1);
				}
			}

			SetCheckboxValue("chkAddProjAppendSliders", step.refAppendNewSliders);
			break;
		}
		case AutomationStepType::ConformSliders: {
			auto* txt = XRCCTRL(*this, "txtConformRadius", wxTextCtrl);
			if (txt)
				txt->SetValue(wxString::Format("%.1f", step.conformProximityRadius));
			txt = XRCCTRL(*this, "txtConformMaxResults", wxTextCtrl);
			if (txt)
				txt->SetValue(wxString::Format("%d", step.conformMaxResults));

			SetCheckboxValue("chkConformNoSqueeze", step.conformNoSqueeze);
			SetCheckboxValue("chkConformSolidMode", step.conformSolidMode);
			SetCheckboxValue("chkConformAxisX", step.conformAxisX);
			SetCheckboxValue("chkConformAxisY", step.conformAxisY);
			SetCheckboxValue("chkConformAxisZ", step.conformAxisZ);
			SetVectorValue("txtConformSliderNames", step.conformSliderNames);
			break;
		}
		case AutomationStepType::CopyBoneWeights: {
			auto* txt = XRCCTRL(*this, "txtWeightRadius", wxTextCtrl);
			if (txt)
				txt->SetValue(wxString::Format("%.1f", step.weightProximityRadius));
			txt = XRCCTRL(*this, "txtWeightMaxResults", wxTextCtrl);
			if (txt)
				txt->SetValue(wxString::Format("%d", step.weightMaxResults));
			SetVectorValue("txtWeightBoneList", step.weightBoneList);
			break;
		}
		case AutomationStepType::ImportSliderData: {
			auto* fp = XRCCTRL(*this, "fpSliderDataFile", wxFilePickerCtrl);
			if (fp)
				fp->SetPath(wxString::FromUTF8(step.sliderDataFile));
			SetCheckboxValue("chkSliderDataFromFolder", step.sliderDataFromFolder);
			auto* dp = XRCCTRL(*this, "dpSliderDataFolder", wxDirPickerCtrl);
			if (dp)
				dp->SetPath(wxString::FromUTF8(step.sliderDataFile));
			UpdateSliderDataFolderVisibility(step.sliderDataFromFolder);
			SetCheckboxValue("chkSliderMerge", step.sliderMerge);
			SetVectorValue("txtSliderNames", step.sliderNames);
			break;
		}
		case AutomationStepType::SetSliderValues: {
			SetVectorValue("txtSetSliderNames", step.setSliderNames);
			auto* txt = XRCCTRL(*this, "txtSetSliderValue", wxTextCtrl);
			if (txt)
				txt->SetValue(wxString::Format("%d", static_cast<int>(step.setSliderValue * 100)));
			break;
		}
		case AutomationStepType::SetSliderProperties: {
			SetVectorValue("txtSliderPropNames", step.sliderPropNames);
			auto* choiceZap = XRCCTRL(*this, "choiceSliderPropZap", wxChoice);
			if (choiceZap)
				choiceZap->SetSelection(step.sliderPropZap < 0 ? 0 : step.sliderPropZap + 1);
			auto* choiceHidden = XRCCTRL(*this, "choiceSliderPropHidden", wxChoice);
			if (choiceHidden)
				choiceHidden->SetSelection(step.sliderPropHidden < 0 ? 0 : step.sliderPropHidden + 1);

			bool isZap = step.sliderPropZap == 1;
			auto* choiceZapped = XRCCTRL(*this, "choiceSliderPropZapped", wxChoice);
			if (choiceZapped) {
				if (isZap) {
					// Map lo/hi to zapped state: -1 = no change, both 0 = not zapped, any > 0 = zapped
					if (step.sliderPropDefaultLo < 0 && step.sliderPropDefaultHi < 0)
						choiceZapped->SetSelection(0);
					else if (step.sliderPropDefaultLo > 0 || step.sliderPropDefaultHi > 0)
						choiceZapped->SetSelection(2);
					else
						choiceZapped->SetSelection(1);
				}
				else {
					choiceZapped->SetSelection(0);
				}
			}

			auto* txtLo = XRCCTRL(*this, "txtSliderPropDefaultLo", wxTextCtrl);
			if (txtLo)
				txtLo->SetValue(step.sliderPropDefaultLo >= 0 ? wxString::Format("%d", step.sliderPropDefaultLo) : "");
			auto* txtHi = XRCCTRL(*this, "txtSliderPropDefaultHi", wxTextCtrl);
			if (txtHi)
				txtHi->SetValue(step.sliderPropDefaultHi >= 0 ? wxString::Format("%d", step.sliderPropDefaultHi) : "");

			UpdateSliderPropDefaultVisibility();
			break;
		}
		case AutomationStepType::ImportFile: {
			auto* fp = XRCCTRL(*this, "fpImportFile", wxFilePickerCtrl);
			if (fp)
				fp->SetPath(wxString::FromUTF8(step.importFilePath));
			SetCheckboxValue("chkImportFromFolder", step.importFromFolder);
			auto* dp = XRCCTRL(*this, "dpImportFolder", wxDirPickerCtrl);
			if (dp)
				dp->SetPath(wxString::FromUTF8(step.importFilePath));
			UpdateImportFolderVisibility(step.importFromFolder);
			break;
		}
		case AutomationStepType::DeleteShape:
			// No parameters — uses Target Meshes
			break;
		case AutomationStepType::RenameShape: {
			SetTextValue("txtRenameOldName", step.renameOldName);
			SetTextValue("txtRenameNewName", step.renameNewName);
			break;
		}
		case AutomationStepType::DeleteSlider: {
			SetTextValue("txtDeleteSliderName", step.deleteSliderName);
			SetCheckboxValue("chkDeleteSliderRegex", step.deleteSliderRegex);
			break;
		}
		case AutomationStepType::SetReferenceShape: {
			SetTextValue("txtSetRefShapeName", step.setRefShapeName);
			break;
		}
		case AutomationStepType::RefineMesh:
		case AutomationStepType::SetBaseShape:
		case AutomationStepType::ClearReference:
		case AutomationStepType::RemoveSkinning:
			// No parameters to set
			break;
		case AutomationStepType::TransformShape: {
			SetFloatValue("txtMoveX", step.moveX);
			SetFloatValue("txtMoveY", step.moveY);
			SetFloatValue("txtMoveZ", step.moveZ);
			SetFloatValue("txtRotateX", step.rotateX);
			SetFloatValue("txtRotateY", step.rotateY);
			SetFloatValue("txtRotateZ", step.rotateZ);
			SetFloatValue("txtScaleX", step.scaleX);
			SetFloatValue("txtScaleY", step.scaleY);
			SetFloatValue("txtScaleZ", step.scaleZ);
			SetFloatValue("txtInflateX", step.inflateX);
			SetFloatValue("txtInflateY", step.inflateY);
			SetFloatValue("txtInflateZ", step.inflateZ);
			break;
		}
		case AutomationStepType::InvertUVs: {
			SetCheckboxValue("chkInvertU", step.invertU);
			SetCheckboxValue("chkInvertV", step.invertV);
			break;
		}
		case AutomationStepType::DeleteBones: {
			SetVectorValue("txtDeleteBoneNames", step.deleteBoneNames);
			SetCheckboxValue("chkDeleteBoneFromProject", step.deleteBoneFromProject);
			break;
		}
		case AutomationStepType::AddCustomBone: {
			SetTextValue("txtAddBoneName", step.addBoneName);
			SetTextValue("txtAddBoneParent", step.addBoneParent);
			SetFloatValue("txtAddBoneTransX", step.addBoneTransX);
			SetFloatValue("txtAddBoneTransY", step.addBoneTransY);
			SetFloatValue("txtAddBoneTransZ", step.addBoneTransZ);
			SetFloatValue("txtAddBoneRotX", step.addBoneRotX);
			SetFloatValue("txtAddBoneRotY", step.addBoneRotY);
			SetFloatValue("txtAddBoneRotZ", step.addBoneRotZ);
			break;
		}
		case AutomationStepType::EditBone: {
			SetTextValue("txtEditBoneName", step.editBoneName);
			SetTextValue("txtEditBoneParent", step.editBoneParent);
			SetFloatValue("txtEditBoneTransX", step.editBoneTransX);
			SetFloatValue("txtEditBoneTransY", step.editBoneTransY);
			SetFloatValue("txtEditBoneTransZ", step.editBoneTransZ);
			SetFloatValue("txtEditBoneRotX", step.editBoneRotX);
			SetFloatValue("txtEditBoneRotY", step.editBoneRotY);
			SetFloatValue("txtEditBoneRotZ", step.editBoneRotZ);
			break;
		}
		case AutomationStepType::ApplyPose: {
			SetTextValue("txtPoseName", step.poseName);
			break;
		}

		case AutomationStepType::SaveProject: {
			SetTextValue("txtSaveDisplayName", step.saveName);
			SetTextValue("txtSaveOutputFileName", step.saveOutputFileName);
			SetTextValue("txtSaveOutputDataPath", step.saveOutputDataPath);
			SetTextValue("txtSaveSliderSetFile", step.saveSliderSetFile);
			SetTextValue("txtSaveShapeDataFolder", step.saveShapeDataFolder);
			SetTextValue("txtSaveShapeDataFile", step.saveShapeDataFile);

			SetCheckboxValue("chkSaveGenWeights", step.saveGenWeights);
			SetCheckboxValue("chkSaveAutoCopyRef", step.saveAutoCopyRef);
			SetCheckboxValue("chkSaveCopyRefFromProject", step.saveCopyRefFromProject);
			SetTextValue("txtSaveCopyRefShapeName", step.saveCopyRefShapeName);
			SetCheckboxValue("chkSaveUseOriginal", step.saveUseOriginal);
			SetTextValue("txtSaveReplaceFrom", step.saveReplaceFrom);
			SetTextValue("txtSaveReplaceTo", step.saveReplaceTo);
			SetTextValue("txtSaveSuffix", step.saveSuffix);
			UpdateSaveProjectBatchModeUI(step);
			break;
		}
		case AutomationStepType::ExportFile: {
			bool isBatch = GetSelectedBatchMode() != AutomationBatchMode::None;
			if (isBatch) {
				auto* dp = XRCCTRL(*this, "dpExportFolder", wxDirPickerCtrl);
				if (dp)
					dp->SetPath(wxString::FromUTF8(step.exportFilePath));
			}
			else {
				auto* fp = XRCCTRL(*this, "fpExportFile", wxFilePickerCtrl);
				if (fp)
					fp->SetPath(wxString::FromUTF8(step.exportFilePath));
			}
			SetCheckboxValue("chkExportWithRef", step.exportWithRef);
			SetCheckboxValue("chkExportUseOriginalPath", step.exportUseOriginalPath);
			SetTextValue("txtExportPrefix", step.exportPrefix);
			SetTextValue("txtExportSuffix", step.exportSuffix);
			UpdateExportForBatchMode();
			UpdateExportFileBatchModeUI(step);
			break;
		}

		case AutomationStepType::ResetTransforms:
			// No parameters to set
			break;

		case AutomationStepType::DuplicateShape: {
			SetTextValue("txtDupNewName", step.dupNewName);
			break;
		}

		case AutomationStepType::MirrorShape: {
			SetCheckboxValue("chkMirrorX", step.mirrorX);
			SetCheckboxValue("chkMirrorY", step.mirrorY);
			SetCheckboxValue("chkMirrorZ", step.mirrorZ);
			SetCheckboxValue("chkMirrorSwapBonesX", step.mirrorSwapBonesX);
			break;
		}

		case AutomationStepType::LoadMask: {
			auto* fp = XRCCTRL(*this, "fpLoadMaskFile", wxFilePickerCtrl);
			if (fp) {
				// If path is relative, resolve it for the file picker display
				wxString path = wxString::FromUTF8(step.loadMaskFile);
				if (!path.IsEmpty()) {
					wxFileName fn(path);
					if (fn.IsRelative()) {
						std::string projPath = GetProjectPath();
						fn.MakeAbsolute(wxString::FromUTF8(projPath));
					}
					fp->SetPath(fn.GetFullPath());
				}
				else {
					fp->SetPath(wxEmptyString);
				}

				// Populate dropdown from the mask file
				PopulateMaskNamesFromFile(fp->GetPath());
			}

			auto* choice = XRCCTRL(*this, "choiceLoadMaskName", wxChoice);
			if (choice) {
				wxString maskName = wxString::FromUTF8(step.loadMaskName);
				int idx = choice->FindString(maskName);
				if (idx != wxNOT_FOUND)
					choice->SetSelection(idx);
				else if (!maskName.IsEmpty()) {
					choice->Append(maskName);
					choice->SetSelection(choice->GetCount() - 1);
				}
			}
			break;
		}

		case AutomationStepType::RemoveUnusedNodes:
			// No parameters to set
			break;
	}
}

void AutomationDialog::UpdateStepFromUI() {
	if (selectedStep < 0 || selectedStep >= static_cast<int>(script.GetSteps().size()))
		return;

	auto& step = script.GetSteps()[selectedStep];
	step.active = chkActive->GetValue();
	step.note = txtNote->GetValue().ToUTF8().data();
	step.targetMeshes = SplitCommaSeparated(txtTargetMeshes->GetValue().ToUTF8().data());
	step.targetRegex = chkTargetRegex ? chkTargetRegex->GetValue() : false;
	step.type = static_cast<AutomationStepType>(choiceStepType->GetSelection());

	switch (step.type) {
		case AutomationStepType::ClearProject:
			// No parameters to read
			break;

		case AutomationStepType::LoadReference: {
			auto* fp = XRCCTRL(*this, "fpRefSourceFile", wxFilePickerCtrl);
			if (fp)
				step.refSourceFile = MakeRelativeToProject(fp->GetPath()).ToUTF8().data();
			auto* choiceSet = XRCCTRL(*this, "choiceRefSet", wxChoice);
			if (choiceSet && choiceSet->GetSelection() != wxNOT_FOUND)
				step.refSet = choiceSet->GetStringSelection().ToUTF8().data();
			auto* choiceShape = XRCCTRL(*this, "choiceRefShape", wxChoice);
			if (choiceShape && choiceShape->GetSelection() != wxNOT_FOUND)
				step.refShape = choiceShape->GetStringSelection().ToUTF8().data();

			step.refLoadAll = GetCheckboxValue("chkRefLoadAll");
			step.refMergeSliders = GetCheckboxValue("chkRefMergeSliders");
			step.refMergeZaps = GetCheckboxValue("chkRefMergeZaps");
			step.refAppendNewSliders = GetCheckboxValue("chkRefAppendNewSliders");
			break;
		}
		case AutomationStepType::AddProject: {
			auto* fp = XRCCTRL(*this, "fpAddProjSourceFile", wxFilePickerCtrl);
			if (fp)
				step.refSourceFile = MakeRelativeToProject(fp->GetPath()).ToUTF8().data();
			auto* choiceSet = XRCCTRL(*this, "choiceAddProjSet", wxChoice);
			if (choiceSet && choiceSet->GetSelection() != wxNOT_FOUND)
				step.refSet = choiceSet->GetStringSelection().ToUTF8().data();
			step.refAppendNewSliders = GetCheckboxValue("chkAddProjAppendSliders");
			break;
		}
		case AutomationStepType::ConformSliders: {
			step.conformProximityRadius = GetFloatValue("txtConformRadius");
			step.conformMaxResults = GetIntValue("txtConformMaxResults");
			step.conformNoSqueeze = GetCheckboxValue("chkConformNoSqueeze");
			step.conformSolidMode = GetCheckboxValue("chkConformSolidMode");
			step.conformAxisX = GetCheckboxValue("chkConformAxisX");
			step.conformAxisY = GetCheckboxValue("chkConformAxisY");
			step.conformAxisZ = GetCheckboxValue("chkConformAxisZ");
			step.conformSliderNames = GetVectorValue("txtConformSliderNames");
			break;
		}
		case AutomationStepType::CopyBoneWeights: {
			step.weightProximityRadius = GetFloatValue("txtWeightRadius");
			step.weightMaxResults = GetIntValue("txtWeightMaxResults");
			step.weightBoneList = GetVectorValue("txtWeightBoneList");
			break;
		}
		case AutomationStepType::ImportSliderData: {
			auto* chkFolder = XRCCTRL(*this, "chkSliderDataFromFolder", wxCheckBox);
			step.sliderDataFromFolder = chkFolder && chkFolder->GetValue();
			if (step.sliderDataFromFolder) {
				auto* dp = XRCCTRL(*this, "dpSliderDataFolder", wxDirPickerCtrl);
				if (dp)
					step.sliderDataFile = dp->GetPath().ToUTF8().data();
			}
			else {
				auto* fp = XRCCTRL(*this, "fpSliderDataFile", wxFilePickerCtrl);
				if (fp)
					step.sliderDataFile = fp->GetPath().ToUTF8().data();
			}
			step.sliderMerge = GetCheckboxValue("chkSliderMerge");
			step.sliderNames = GetVectorValue("txtSliderNames");
			break;
		}
		case AutomationStepType::ImportFile: {
			auto* chkFolder = XRCCTRL(*this, "chkImportFromFolder", wxCheckBox);
			step.importFromFolder = chkFolder && chkFolder->GetValue();
			if (step.importFromFolder) {
				auto* dp = XRCCTRL(*this, "dpImportFolder", wxDirPickerCtrl);
				if (dp)
					step.importFilePath = dp->GetPath().ToUTF8().data();
			}
			else {
				auto* fp = XRCCTRL(*this, "fpImportFile", wxFilePickerCtrl);
				if (fp)
					step.importFilePath = fp->GetPath().ToUTF8().data();
			}
			break;
		}
		case AutomationStepType::SetSliderValues: {
			step.setSliderNames = GetVectorValue("txtSetSliderNames");
			step.setSliderValue = GetFloatValue("txtSetSliderValue") / 100.0f;
			break;
		}
		case AutomationStepType::SetSliderProperties: {
			step.sliderPropNames = GetVectorValue("txtSliderPropNames");
			auto* choiceZap = XRCCTRL(*this, "choiceSliderPropZap", wxChoice);
			if (choiceZap) {
				int sel = choiceZap->GetSelection();
				step.sliderPropZap = sel <= 0 ? -1 : sel - 1;
			}
			auto* choiceHidden = XRCCTRL(*this, "choiceSliderPropHidden", wxChoice);
			if (choiceHidden) {
				int sel = choiceHidden->GetSelection();
				step.sliderPropHidden = sel <= 0 ? -1 : sel - 1;
			}

			bool isZap = step.sliderPropZap == 1;
			if (isZap) {
				// Read from zapped choice instead of lo/hi text fields
				auto* choiceZapped = XRCCTRL(*this, "choiceSliderPropZapped", wxChoice);
				if (choiceZapped) {
					int sel = choiceZapped->GetSelection();
					if (sel == 2) {
						step.sliderPropDefaultLo = 100;
						step.sliderPropDefaultHi = 100;
					}
					else if (sel == 1) {
						step.sliderPropDefaultLo = 0;
						step.sliderPropDefaultHi = 0;
					}
					else {
						step.sliderPropDefaultLo = -1;
						step.sliderPropDefaultHi = -1;
					}
				}
			}
			else {
				auto* txtLo = XRCCTRL(*this, "txtSliderPropDefaultLo", wxTextCtrl);
				if (txtLo) {
					wxString val = txtLo->GetValue().Trim();
					step.sliderPropDefaultLo = val.IsEmpty() ? -1 : wxAtoi(val);
				}
				auto* txtHi = XRCCTRL(*this, "txtSliderPropDefaultHi", wxTextCtrl);
				if (txtHi) {
					wxString val = txtHi->GetValue().Trim();
					step.sliderPropDefaultHi = val.IsEmpty() ? -1 : wxAtoi(val);
				}
			}
			break;
		}
		case AutomationStepType::DeleteShape:
			// No parameters — uses Target Meshes
			break;
		case AutomationStepType::RenameShape: {
			step.renameOldName = GetTextValue("txtRenameOldName");
			step.renameNewName = GetTextValue("txtRenameNewName");
			break;
		}
		case AutomationStepType::DeleteSlider: {
			step.deleteSliderName = GetTextValue("txtDeleteSliderName");
			step.deleteSliderRegex = GetCheckboxValue("chkDeleteSliderRegex");
			break;
		}
		case AutomationStepType::SetReferenceShape: {
			step.setRefShapeName = GetTextValue("txtSetRefShapeName");
			break;
		}
		case AutomationStepType::RefineMesh:
		case AutomationStepType::SetBaseShape:
		case AutomationStepType::ClearReference:
		case AutomationStepType::RemoveSkinning:
			// No parameters to read
			break;
		case AutomationStepType::TransformShape: {
			step.moveX = GetFloatValue("txtMoveX");
			step.moveY = GetFloatValue("txtMoveY");
			step.moveZ = GetFloatValue("txtMoveZ");
			step.rotateX = GetFloatValue("txtRotateX");
			step.rotateY = GetFloatValue("txtRotateY");
			step.rotateZ = GetFloatValue("txtRotateZ");
			step.scaleX = GetFloatValue("txtScaleX");
			step.scaleY = GetFloatValue("txtScaleY");
			step.scaleZ = GetFloatValue("txtScaleZ");
			step.inflateX = GetFloatValue("txtInflateX");
			step.inflateY = GetFloatValue("txtInflateY");
			step.inflateZ = GetFloatValue("txtInflateZ");
			break;
		}
		case AutomationStepType::InvertUVs: {
			step.invertU = GetCheckboxValue("chkInvertU");
			step.invertV = GetCheckboxValue("chkInvertV");
			break;
		}
		case AutomationStepType::DeleteBones: {
			step.deleteBoneNames = GetVectorValue("txtDeleteBoneNames");
			step.deleteBoneFromProject = GetCheckboxValue("chkDeleteBoneFromProject");
			break;
		}
		case AutomationStepType::AddCustomBone: {
			step.addBoneName = GetTextValue("txtAddBoneName");
			step.addBoneParent = GetTextValue("txtAddBoneParent");
			step.addBoneTransX = GetFloatValue("txtAddBoneTransX");
			step.addBoneTransY = GetFloatValue("txtAddBoneTransY");
			step.addBoneTransZ = GetFloatValue("txtAddBoneTransZ");
			step.addBoneRotX = GetFloatValue("txtAddBoneRotX");
			step.addBoneRotY = GetFloatValue("txtAddBoneRotY");
			step.addBoneRotZ = GetFloatValue("txtAddBoneRotZ");
			break;
		}
		case AutomationStepType::EditBone: {
			step.editBoneName = GetTextValue("txtEditBoneName");
			step.editBoneParent = GetTextValue("txtEditBoneParent");
			step.editBoneTransX = GetFloatValue("txtEditBoneTransX");
			step.editBoneTransY = GetFloatValue("txtEditBoneTransY");
			step.editBoneTransZ = GetFloatValue("txtEditBoneTransZ");
			step.editBoneRotX = GetFloatValue("txtEditBoneRotX");
			step.editBoneRotY = GetFloatValue("txtEditBoneRotY");
			step.editBoneRotZ = GetFloatValue("txtEditBoneRotZ");
			break;
		}
		case AutomationStepType::ApplyPose: {
			step.poseName = GetTextValue("txtPoseName");
			break;
		}

		case AutomationStepType::SaveProject: {
			step.saveName = GetTextValue("txtSaveDisplayName");
			step.saveOutputFileName = GetTextValue("txtSaveOutputFileName");
			step.saveOutputDataPath = GetTextValue("txtSaveOutputDataPath");
			step.saveSliderSetFile = GetTextValue("txtSaveSliderSetFile");
			step.saveShapeDataFolder = GetTextValue("txtSaveShapeDataFolder");
			step.saveShapeDataFile = GetTextValue("txtSaveShapeDataFile");

			step.saveGenWeights = GetCheckboxValue("chkSaveGenWeights");
			step.saveAutoCopyRef = GetCheckboxValue("chkSaveAutoCopyRef");
			step.saveCopyRefFromProject = GetCheckboxValue("chkSaveCopyRefFromProject");
			step.saveCopyRefShapeName = GetTextValue("txtSaveCopyRefShapeName");
			step.saveUseOriginal = GetCheckboxValue("chkSaveUseOriginal");
			step.saveReplaceFrom = GetTextValue("txtSaveReplaceFrom");
			step.saveReplaceTo = GetTextValue("txtSaveReplaceTo");
			step.saveSuffix = GetTextValue("txtSaveSuffix");
			break;
		}
		case AutomationStepType::ExportFile: {
			bool isBatch = radioBatchMode && radioBatchMode->GetSelection() != 0;
			if (isBatch) {
				auto* dp = XRCCTRL(*this, "dpExportFolder", wxDirPickerCtrl);
				if (dp)
					step.exportFilePath = dp->GetPath().ToUTF8().data();
			}
			else {
				auto* fp = XRCCTRL(*this, "fpExportFile", wxFilePickerCtrl);
				if (fp)
					step.exportFilePath = fp->GetPath().ToUTF8().data();
			}
			step.exportWithRef = GetCheckboxValue("chkExportWithRef");
			step.exportUseOriginalPath = GetCheckboxValue("chkExportUseOriginalPath");
			step.exportPrefix = GetTextValue("txtExportPrefix");
			step.exportSuffix = GetTextValue("txtExportSuffix");
			break;
		}

		case AutomationStepType::ResetTransforms:
			// No parameters to read
			break;

		case AutomationStepType::DuplicateShape: {
			step.dupNewName = GetTextValue("txtDupNewName");
			break;
		}

		case AutomationStepType::MirrorShape: {
			step.mirrorX = GetCheckboxValue("chkMirrorX");
			step.mirrorY = GetCheckboxValue("chkMirrorY");
			step.mirrorZ = GetCheckboxValue("chkMirrorZ");
			step.mirrorSwapBonesX = GetCheckboxValue("chkMirrorSwapBonesX");
			break;
		}

		case AutomationStepType::LoadMask: {
			auto* fp = XRCCTRL(*this, "fpLoadMaskFile", wxFilePickerCtrl);
			if (fp)
				step.loadMaskFile = MakeRelativeToProject(fp->GetPath()).ToUTF8().data();
			auto* choice = XRCCTRL(*this, "choiceLoadMaskName", wxChoice);
			if (choice && choice->GetSelection() != wxNOT_FOUND)
				step.loadMaskName = choice->GetStringSelection().ToUTF8().data();
			break;
		}

		case AutomationStepType::RemoveUnusedNodes:
			// No parameters to read
			break;
	}

	RefreshStepRow(selectedStep);
}

std::map<std::string, std::string> AutomationDialog::CollectVariables() {
	std::map<std::string, std::string> vars;

	// Collect from the variables panel
	for (int i = 1; i <= 10; i++) {
		wxString keyName = wxString::Format("txtVarKey%d", i);
		wxString valName = wxString::Format("txtVarVal%d", i);
		auto* keyCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(keyName));
		auto* valCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(valName));
		if (keyCtrl && valCtrl) {
			std::string key = keyCtrl->GetValue().ToUTF8().data();
			std::string val = valCtrl->GetValue().ToUTF8().data();
			if (!key.empty())
				vars[key] = val;
		}
	}

	return vars;
}

NiShape* AutomationDialog::FindShapeByName(const std::string& name) {
	auto shapes = project->GetWorkNif()->GetShapes();
	for (auto* shape : shapes) {
		if (shape->name.get() == name)
			return shape;
	}
	return nullptr;
}

std::vector<NiShape*> AutomationDialog::ResolveTargetShapes(const AutomationStep& step) {
	if (step.targetMeshes.empty()) {
		auto allShapes = project->GetWorkNif()->GetShapes();
		std::vector<NiShape*> result;
		auto* baseShape = project->GetBaseShape();
		for (auto* shape : allShapes) {
			if (shape != baseShape)
				result.push_back(shape);
		}
		return result;
	}

	std::vector<NiShape*> result;

	if (step.targetRegex) {
		auto allShapes = project->GetWorkNif()->GetShapes();
		for (const auto& pattern : step.targetMeshes) {
			try {
				std::regex re(pattern, std::regex::icase);
				for (auto* shape : allShapes) {
					if (std::regex_search(shape->name.get(), re)) {
						// Avoid duplicates
						if (std::find(result.begin(), result.end(), shape) == result.end())
							result.push_back(shape);
					}
				}
			}
			catch (const std::regex_error&) {
				wxLogWarning("Automation: Invalid regex pattern '%s', treating as literal.", pattern);
				NiShape* shape = FindShapeByName(pattern);
				if (shape && std::find(result.begin(), result.end(), shape) == result.end())
					result.push_back(shape);
			}
		}
	}
	else {
		for (const auto& name : step.targetMeshes) {
			NiShape* shape = FindShapeByName(name);
			if (shape)
				result.push_back(shape);
		}
	}

	return result;
}

std::string AutomationDialog::GetAutomationsFolder() {
	return GetProjectPath() + "/Automations";
}

void AutomationDialog::PopulateAutomationList() {
	if (!cmbAutomation)
		return;

	wxString currentText = cmbAutomation->GetValue();
	cmbAutomation->Clear();
	cmbAutomation->Append(_("<New>"));

	wxString folder = wxString::FromUTF8(GetAutomationsFolder());
	if (wxDir::Exists(folder)) {
		wxDir dir(folder);
		if (dir.IsOpened()) {
			wxString filename;
			if (dir.GetFirst(&filename, "*.xml", wxDIR_FILES)) {
				do {
					wxFileName fn(filename);
					cmbAutomation->Append(fn.GetName());
				} while (dir.GetNext(&filename));
			}
		}
	}

	if (!currentText.IsEmpty())
		cmbAutomation->SetValue(currentText);
}

wxString AutomationDialog::SanitizeFileName(const wxString& name) {
	wxString result;
	for (auto ch : name) {
		if (ch == '<' || ch == '>' || ch == ':' || ch == '"' || ch == '/' || ch == '\\' || ch == '|' || ch == '?' || ch == '*')
			result += '_';
		else
			result += ch;
	}
	return result;
}

void AutomationDialog::LoadAutomation(const wxString& name) {
	if (name.IsEmpty())
		return;

	wxString sanitized = SanitizeFileName(name);
	wxString filePath = wxString::FromUTF8(GetAutomationsFolder()) + "/" + sanitized + ".xml";

	if (!wxFileExists(filePath))
		return;

	int err = script.Load(filePath.ToUTF8().data());
	if (err) {
		wxMessageBox(wxString::Format(_("Failed to load automation script (error %d)."), err), _("Error"), wxICON_ERROR);
		return;
	}

	selectedStep = -1;
	ShowStepSettings(false);
	PopulateStepList();
	PopulateVariablesUI();
	SyncBatchUIFromScript();

	// Expand panes based on loaded script content
	auto* paneVariables = XRCCTRL(*this, "paneVariables", wxCollapsiblePane);
	auto* paneBatch = XRCCTRL(*this, "paneBatch", wxCollapsiblePane);
	if (paneVariables)
		paneVariables->Collapse(script.GetVariables().empty());
	if (paneBatch)
		paneBatch->Collapse(script.GetBatchMode() == AutomationBatchMode::None);

	UpdateButtonState();
	GetSizer()->Layout();
}

void AutomationDialog::UpdateButtonState() {
	bool hasSteps = !script.GetSteps().empty();
	if (btnSaveScript)
		btnSaveScript->Enable(hasSteps);
	if (btnExecuteAll)
		btnExecuteAll->Enable(hasSteps);
}

void AutomationDialog::OnAutomationSelected(wxCommandEvent& WXUNUSED(event)) {
	wxString name = cmbAutomation->GetValue();
	if (name == _("<New>")) {
		script = AutomationScript();
		selectedStep = -1;
		ShowStepSettings(false);
		PopulateStepList();
		PopulateVariablesUI();
		SyncBatchUIFromScript();

		auto* paneVariables = XRCCTRL(*this, "paneVariables", wxCollapsiblePane);
		auto* paneBatch = XRCCTRL(*this, "paneBatch", wxCollapsiblePane);
		if (paneVariables)
			paneVariables->Collapse(true);
		if (paneBatch)
			paneBatch->Collapse(true);

		cmbAutomation->SetValue(wxEmptyString);
		UpdateButtonState();
		GetSizer()->Layout();
		return;
	}
	LoadAutomation(name);
}

void AutomationDialog::OnSaveScript(wxCommandEvent& WXUNUSED(event)) {
	if (selectedStep >= 0)
		UpdateStepFromUI();

	// Sync variables and batch settings from UI to script before saving
	script.GetVariables() = CollectVariables();
	SyncBatchScriptFromUI();

	wxString name = cmbAutomation ? cmbAutomation->GetValue().Trim().Trim(false) : wxString();
	if (name.IsEmpty()) {
		wxMessageBox(_("Please enter an automation name."), _("Automation"), wxICON_INFORMATION);
		return;
	}

	wxString sanitized = SanitizeFileName(name);
	wxString folder = wxString::FromUTF8(GetAutomationsFolder());

	// Create folder if missing
	if (!wxDir::Exists(folder))
		wxFileName::Mkdir(folder, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	wxString filePath = folder + "/" + sanitized + ".xml";
	int err = script.Save(filePath.ToUTF8().data());
	if (err) {
		wxMessageBox(wxString::Format(_("Failed to save automation script (error %d)."), err), _("Error"), wxICON_ERROR);
		return;
	}

	PopulateAutomationList();
	cmbAutomation->SetValue(sanitized);
}

void AutomationDialog::OnDeleteScript(wxCommandEvent& WXUNUSED(event)) {
	wxString name = cmbAutomation ? cmbAutomation->GetValue().Trim().Trim(false) : wxString();
	if (name.IsEmpty()) {
		wxMessageBox(_("No automation selected to delete."), _("Automation"), wxICON_INFORMATION);
		return;
	}

	wxString sanitized = SanitizeFileName(name);
	wxString filePath = wxString::FromUTF8(GetAutomationsFolder()) + "/" + sanitized + ".xml";

	if (!wxFileExists(filePath)) {
		wxMessageBox(_("Automation file not found."), _("Automation"), wxICON_INFORMATION);
		return;
	}

	int answer = wxMessageBox(
		wxString::Format(_("Delete automation '%s'?\nThis cannot be undone."), name),
		_("Confirm Delete"), wxYES_NO | wxICON_QUESTION);
	if (answer != wxYES)
		return;

	wxRemoveFile(filePath);

	// Clear current script
	script = AutomationScript();
	selectedStep = -1;
	ShowStepSettings(false);
	PopulateStepList();
	PopulateVariablesUI();
	SyncBatchUIFromScript();
	PopulateAutomationList();
	cmbAutomation->SetValue(wxEmptyString);
	UpdateButtonState();
	GetSizer()->Layout();
}

void AutomationDialog::OnOpenFolder(wxCommandEvent& WXUNUSED(event)) {
	wxString folder = MakeAbsoluteToProject(wxString::FromUTF8(GetAutomationsFolder()));
	if (!wxDir::Exists(folder))
		wxFileName::Mkdir(folder, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	wxLaunchDefaultApplication(folder);
}

void AutomationDialog::OnAddStep(wxCommandEvent& WXUNUSED(event)) {
	if (selectedStep >= 0)
		UpdateStepFromUI();

	AutomationStep step;
	step.type = AutomationStepType::ClearProject;

	// Prefill SaveProject fields from current project state
	step.saveName = project->mOutfitName;
	step.saveOutputFileName = project->mGameFile;
	step.saveOutputDataPath = project->mGamePath;
	step.saveSliderSetFile = project->mFileName;
	step.saveShapeDataFolder = project->mDataDir;
	step.saveShapeDataFile = project->mBaseFile;
	ApplyBatchModeDefaults(step);

	int newIndex;
	if (selectedStep >= 0) {
		newIndex = selectedStep + 1;
		script.InsertStep(newIndex, step);
	}
	else {
		script.AddStep(step);
		newIndex = static_cast<int>(script.GetSteps().size()) - 1;
	}

	PopulateStepList();

	listSteps->SetItemState(newIndex, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	listSteps->EnsureVisible(newIndex);
	SelectStep(newIndex);
	UpdateButtonState();
}

void AutomationDialog::OnRemoveStep(wxCommandEvent& WXUNUSED(event)) {
	if (selectedStep < 0)
		return;

	script.RemoveStep(selectedStep);
	int prevSelected = selectedStep;
	selectedStep = -1;
	PopulateStepList();

	if (!script.GetSteps().empty()) {
		int newSel = std::min(prevSelected, static_cast<int>(script.GetSteps().size()) - 1);
		listSteps->SetItemState(newSel, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
		SelectStep(newSel);
	}
	else {
		ShowStepSettings(false);
	}
	UpdateButtonState();
}

void AutomationDialog::OnMoveUp(wxCommandEvent& WXUNUSED(event)) {
	if (selectedStep <= 0)
		return;

	UpdateStepFromUI();
	script.MoveStepUp(selectedStep);
	int newSel = selectedStep - 1;
	selectedStep = -1;
	PopulateStepList();
	listSteps->SetItemState(newSel, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	SelectStep(newSel);
}

void AutomationDialog::OnMoveDown(wxCommandEvent& WXUNUSED(event)) {
	if (selectedStep < 0 || selectedStep + 1 >= static_cast<int>(script.GetSteps().size()))
		return;

	UpdateStepFromUI();
	script.MoveStepDown(selectedStep);
	int newSel = selectedStep + 1;
	selectedStep = -1;
	PopulateStepList();
	listSteps->SetItemState(newSel, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	SelectStep(newSel);
}

void AutomationDialog::OnStepListKeyDown(wxKeyEvent& event) {
	if (event.ControlDown()) {
		wxCommandEvent evt;
		if (event.GetKeyCode() == WXK_UP) {
			OnMoveUp(evt);
			return;
		}
		else if (event.GetKeyCode() == WXK_DOWN) {
			OnMoveDown(evt);
			return;
		}
	}
	event.Skip();
}

void AutomationDialog::OnStepSelected(wxListEvent& event) {
	SelectStep(event.GetIndex());

	// Restore focus to the list so arrow keys continue to work
	if (listSteps)
		listSteps->SetFocus();
}

void AutomationDialog::OnStepTypeChanged(wxCommandEvent& WXUNUSED(event)) {
	int sel = choiceStepType->GetSelection();
	if (sel < 0)
		return;

	if (selectedStep < 0 || selectedStep >= static_cast<int>(script.GetSteps().size())) {
		bookStepPages->SetSelection(sel);
		return;
	}

	auto& step = script.GetSteps()[selectedStep];
	step.active = chkActive->GetValue();
	step.note = txtNote->GetValue().ToUTF8().data();
	step.targetMeshes = SplitCommaSeparated(std::string(txtTargetMeshes->GetValue().ToUTF8().data()));
	step.targetRegex = chkTargetRegex ? chkTargetRegex->GetValue() : false;
	step.type = static_cast<AutomationStepType>(sel);
	ApplyBatchModeDefaults(step);

	RefreshStepRow(selectedStep);
	UpdateUIFromStep(step);
}

bool AutomationDialog::ShowCheckableListDialog(const wxString& title, const wxString& labelText, const wxArrayString& items, std::vector<size_t>& checkedIndices) {
	wxDialog dlg(this, wxID_ANY, title, wxDefaultPosition, wxSize(800, 500), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
	auto* sizer = new wxBoxSizer(wxVERTICAL);

	auto* label = new wxStaticText(&dlg, wxID_ANY, labelText);
	sizer->Add(label, 0, wxALL, 10);

	auto* checkList = new wxCheckListBox(&dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize, items, wxLB_HSCROLL | wxLB_NEEDED_SB);
	for (unsigned int i = 0; i < checkList->GetCount(); i++)
		checkList->Check(i);

	sizer->Add(checkList, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
	auto* btnAll = new wxButton(&dlg, wxID_ANY, _("Select All"));
	auto* btnNone = new wxButton(&dlg, wxID_ANY, _("Select None"));
	auto* btnInvert = new wxButton(&dlg, wxID_ANY, _("Invert Selection"));
	btnSizer->Add(btnAll, 0, wxRIGHT, 5);
	btnSizer->Add(btnNone, 0, wxRIGHT, 5);
	btnSizer->Add(btnInvert, 0, wxRIGHT, 5);
	btnSizer->AddStretchSpacer();
	auto* btnOK = new wxButton(&dlg, wxID_OK, _("Execute"));
	auto* btnCancel = new wxButton(&dlg, wxID_CANCEL, _("Cancel"));
	btnSizer->Add(btnOK, 0, wxRIGHT, 5);
	btnSizer->Add(btnCancel, 0);
	sizer->Add(btnSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	btnAll->Bind(wxEVT_BUTTON, [checkList](wxCommandEvent&) {
		for (unsigned int i = 0; i < checkList->GetCount(); i++)
			checkList->Check(i);
	});
	btnNone->Bind(wxEVT_BUTTON, [checkList](wxCommandEvent&) {
		for (unsigned int i = 0; i < checkList->GetCount(); i++)
			checkList->Check(i, false);
	});
	btnInvert->Bind(wxEVT_BUTTON, [checkList](wxCommandEvent&) {
		for (unsigned int i = 0; i < checkList->GetCount(); i++)
			checkList->Check(i, !checkList->IsChecked(i));
	});

	dlg.SetSizer(sizer);
	dlg.CenterOnParent();

	if (dlg.ShowModal() != wxID_OK)
		return false;

	checkedIndices.clear();
	for (unsigned int i = 0; i < checkList->GetCount(); i++) {
		if (checkList->IsChecked(i))
			checkedIndices.push_back(i);
	}

	return true;
}

void AutomationDialog::OnExecuteAll(wxCommandEvent& WXUNUSED(event)) {
	if (selectedStep >= 0)
		UpdateStepFromUI();

	SyncBatchScriptFromUI();

	std::vector<size_t> indices;
	for (size_t i = 0; i < script.GetSteps().size(); i++) {
		if (script.GetSteps()[i].active)
			indices.push_back(i);
	}

	if (indices.empty()) {
		wxMessageBox(_("No active steps to execute."), _("Automation"), wxICON_INFORMATION);
		return;
	}

	if (script.GetBatchMode() != AutomationBatchMode::None) {
		// Show confirmation with item count and batch details
		int itemCount = 0;
		wxString batchDetails;

		if (script.GetBatchMode() == AutomationBatchMode::FolderScan) {
			auto files = GatherBatchFiles();
			itemCount = static_cast<int>(files.size());

			if (itemCount == 0) {
				wxMessageBox(_("No files found matching the batch folder scan criteria."), _("Automation"), wxICON_INFORMATION);
				return;
			}

			if (itemCount <= 1000) {
				wxArrayString displayItems;
				for (const auto& filePath : files)
					displayItems.Add(wxString::FromUTF8(filePath));

				std::vector<size_t> checkedIndices;
				if (!ShowCheckableListDialog(_("Batch Files"), wxString::Format(_("Select files to process (%d found):"), itemCount), displayItems, checkedIndices))
					return;

				if (checkedIndices.empty()) {
					wxMessageBox(_("No files selected."), _("Automation"), wxICON_INFORMATION);
					return;
				}

				std::vector<std::string> selectedFiles;
				for (size_t idx : checkedIndices)
					selectedFiles.push_back(files[idx]);

				ExecuteBatch(indices, selectedFiles);
				return;
			}

			// More than 1000 files: skip list, show confirmation
			batchDetails = wxString::Format(_("Folder: %s\nExtension: %s"), wxString::FromUTF8(script.GetBatchFolder()), wxString::FromUTF8(script.GetBatchExtension()));

			std::string filter = script.GetBatchFileFilter();
			if (!filter.empty())
				batchDetails += wxString::Format(_("\nFile filter: %s%s"), wxString::FromUTF8(filter), script.GetBatchFileFilterRegex() ? " (regex)" : "");
		}
		else if (script.GetBatchMode() == AutomationBatchMode::SliderSets) {
			auto sets = GatherBatchSliderSets();
			itemCount = static_cast<int>(sets.size());

			if (itemCount == 0) {
				wxMessageBox(_("No slider sets found matching the filter criteria."), _("Automation"), wxICON_INFORMATION);
				return;
			}

			wxArrayString displayItems;
			for (const auto& [filePath, setName] : sets)
				displayItems.Add(wxString::FromUTF8(setName));

			std::vector<size_t> checkedIndices;
			if (!ShowCheckableListDialog(_("Batch Slider Sets"), wxString::Format(_("Select slider sets to process (%d found):"), itemCount), displayItems, checkedIndices))
				return;

			if (checkedIndices.empty()) {
				wxMessageBox(_("No slider sets selected."), _("Automation"), wxICON_INFORMATION);
				return;
			}

			std::vector<std::pair<std::string, std::string>> selectedSets;
			for (size_t idx : checkedIndices)
				selectedSets.push_back(sets[idx]);

			ExecuteBatch(indices, {}, selectedSets);
			return;
		}

		if (itemCount == 0) {
			wxMessageBox(_("No items found matching the batch criteria."), _("Automation"), wxICON_INFORMATION);
			return;
		}

		wxString confirmMsg = wxString::Format(_("Execute %zu active step(s) on %d item(s)?\n\n%s\n\nThis may take a while."), indices.size(), itemCount, batchDetails);

		int answer = wxMessageBox(confirmMsg, _("Batch Confirmation"), wxYES_NO | wxICON_QUESTION);
		if (answer != wxYES)
			return;

		ExecuteBatch(indices);
	}
	else {
		int answer = wxMessageBox(wxString::Format(_("Execute %zu active step(s) on the current project?"), indices.size()), _("Confirm Execution"), wxYES_NO | wxICON_QUESTION);
		if (answer != wxYES)
			return;

		ExecuteSteps(indices);
	}
}

void AutomationDialog::OnExecuteSelected(wxCommandEvent& WXUNUSED(event)) {
	if (selectedStep < 0) {
		wxMessageBox(_("No step selected."), _("Automation"), wxICON_INFORMATION);
		return;
	}

	UpdateStepFromUI();
	ExecuteSteps({static_cast<size_t>(selectedStep)});
}

void AutomationDialog::OnClose(wxCommandEvent& WXUNUSED(event)) {
	if (selectedStep >= 0)
		UpdateStepFromUI();
	EndModal(wxID_CLOSE);
}

static bool StepChangesSliderSet(AutomationStepType type) {
	switch (type) {
		case AutomationStepType::ClearProject:
		case AutomationStepType::LoadReference:
		case AutomationStepType::AddProject:
		case AutomationStepType::DeleteSlider:
		case AutomationStepType::ClearReference:
		case AutomationStepType::SetBaseShape:
		case AutomationStepType::ImportSliderData:
			return true;
		default:
			return false;
	}
}

void AutomationDialog::ExecuteSteps(const std::vector<size_t>& stepIndices) {
	// Make a copy so placeholder substitution doesn't modify the UI version
	AutomationScript execScript;
	for (size_t idx : stepIndices)
		execScript.AddStep(script.GetSteps()[idx]);

	// Apply placeholder substitution
	auto vars = CollectVariables();
	if (!vars.empty())
		execScript.SubstitutePlaceholders(vars);

	StartProgress(_("Running automation script..."));

	int totalSteps = static_cast<int>(execScript.GetSteps().size());
	for (int i = 0; i < totalSteps; i++) {
		const auto& step = execScript.GetSteps()[i];
		wxString stepDesc = wxString::Format(_("Step %d/%d: %s"),
			i + 1, totalSteps,
			wxString::FromUTF8(AutomationStepTypeToString(step.type)));

		UpdateProgress(i * 100 / totalSteps, stepDesc);
		wxLogMessage("Automation: %s", stepDesc);

		int err = ExecuteStep(step);
		if (err != 0) {
			wxString errMsg = wxString::Format(
				_("Step %d (%s) failed with error %d.\n\n%s\n\nContinue with remaining steps?"),
				i + 1,
				wxString::FromUTF8(AutomationStepTypeToString(step.type)),
				err,
				wxString::FromUTF8(step.note));

			int result = wxMessageBox(errMsg, _("Automation Error"), wxYES_NO | wxICON_ERROR);
			if (result != wxYES) {
				EndProgress(_("Automation aborted."));
				return;
			}
		}

		outfitStudio->RefreshGUIFromProj();

		if (StepChangesSliderSet(step.type))
			outfitStudio->CreateSetSliders();

		outfitStudio->ApplySliders();
	}

	EndProgress(_("Automation complete."));

	wxMessageBox(wxString::Format(_("Automation completed: %d step(s) executed."), totalSteps),
				 _("Automation"), wxICON_INFORMATION);
}

void AutomationDialog::ResetAndClearProject() {
	project->GetWorkAnim()->Clear();
	project->GetWorkNif()->Clear();
	outfitStudio->ResetProject();
}

int AutomationDialog::ExecuteStepClearProject(const AutomationStep&) {
	wxLogMessage("Automation: Clearing project...");
	ResetAndClearProject();
	return 0;
}

int AutomationDialog::ExecuteStepLoadReference(const AutomationStep& step) {
	if (step.refSourceFile.empty()) {
		wxLogError("Automation: LoadReference - no source file specified.");
		return 1;
	}

	wxString refSourceFile = MakeAbsoluteToProject(wxString::FromUTF8(step.refSourceFile));
	wxFileName fn(refSourceFile);
	wxString ext = fn.GetExt().Lower();
	std::string refSourceFileStd = refSourceFile.ToUTF8().data();

	int err = 0;
	if (ext == "nif") {
		err = project->LoadReferenceNif(refSourceFileStd, step.refShape, step.refMergeSliders, step.refMergeZaps);
	}
	else {
		if (!step.refSet.empty()) {
			err = project->LoadReference(refSourceFileStd, step.refSet, step.refShape, step.refMergeSliders, step.refMergeZaps, step.refAppendNewSliders);
		}
		else {
			err = project->LoadReferenceTemplate(refSourceFileStd, step.refSet, step.refShape, step.refLoadAll, step.refMergeSliders, step.refMergeZaps, step.refAppendNewSliders);
		}
	}

	if (err) {
		wxLogError("Automation: LoadReference failed with error %d.", err);
		return err;
	}
	return 0;
}

int AutomationDialog::ExecuteStepAddProject(const AutomationStep& step) {
	if (step.refSourceFile.empty()) {
		wxLogError("Automation: AddProject - no source file specified.");
		return 1;
	}

	wxString refSourceFile = MakeAbsoluteToProject(wxString::FromUTF8(step.refSourceFile));
	std::string refSourceFileStd = refSourceFile.ToUTF8().data();
	wxLogMessage("Automation: Adding project from '%s' (set: '%s')...", refSourceFile, step.refSet);
	int err = project->AddFromSliderSet(refSourceFileStd, step.refSet, true, step.refAppendNewSliders);
	if (err) {
		wxLogError("Automation: AddProject failed with error %d.", err);
		return err;
	}

	project->SetTextures();
	return 0;
}

int AutomationDialog::ExecuteStepSetSliderValues(const AutomationStep& step) {
	int intVal = static_cast<int>(step.setSliderValue * 100);
	if (step.setSliderNames.empty()) {
		wxLogMessage("Automation: Setting all slider values to %d%%...", intVal);
		for (size_t i = 0; i < project->SliderCount(); i++)
			outfitStudio->SetSliderValue(i, intVal);
	}
	else {
		for (const auto& name : step.setSliderNames) {
			if (!project->ValidSlider(name)) {
				wxLogError("Automation: SetSliderValues - slider '%s' not found.", name);
				return 1;
			}
			wxLogMessage("Automation: Setting slider '%s' to %d%%...", name, intVal);
			outfitStudio->SetSliderValue(name, intVal);
		}
	}
	outfitStudio->ApplySliders();
	return 0;
}

int AutomationDialog::ExecuteStepConformSliders(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: ConformSliders - no target shapes found.");
		return 0;
	}

	ConformOptions options;
	options.proximityRadius = step.conformProximityRadius;
	options.maxResults = step.conformMaxResults;
	options.noSqueeze = step.conformNoSqueeze;
	options.solidMode = step.conformSolidMode;
	options.axisX = step.conformAxisX;
	options.axisY = step.conformAxisY;
	options.axisZ = step.conformAxisZ;
	options.sliderNames = step.conformSliderNames;

	project->InitConform();
	for (auto* shape : shapes) {
		wxLogMessage("Automation: Conforming '%s'...", shape->name.get());
		Mesh* m = outfitStudio->glView->GetMesh(shape->name.get());
		if (m)
			project->morpher.CopyMeshMask(m, shape->name.get());
		project->ConformShape(shape, options);
	}
	project->morpher.ClearProximityCache();
	project->morpher.UnlinkRefDiffData();
	return 0;
}

int AutomationDialog::ExecuteStepCopyBoneWeights(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: CopyBoneWeights - no target shapes found.");
		return 0;
	}

	if (!project->GetBaseShape()) {
		wxLogError("Automation: CopyBoneWeights - no reference shape loaded.");
		return 1;
	}

	AnimInfo& workAnim = *project->GetWorkAnim();
	std::vector<std::string> baseBones = workAnim.shapeBones[project->GetBaseShape()->name.get()];
	std::sort(baseBones.begin(), baseBones.end());

	int shapeIdx = 0;
	for (auto* shape : shapes) {
		wxLogMessage("Automation: Copying bone weights to '%s'...", shape->name.get());

		std::unordered_map<uint16_t, float> mask;
		outfitStudio->glView->GetShapeMask(mask, shape->name.get());
		UndoStateShape uss;
		uss.shapeName = shape->name.get();

		std::vector<std::string> boneList;
		if (!step.weightBoneList.empty()) {
			boneList = step.weightBoneList;
		}
		else {
			// Use all bones from base + shape
			boneList = baseBones;
			auto& shapeBones = workAnim.shapeBones[shape->name.get()];
			for (const auto& b : shapeBones) {
				if (!std::binary_search(baseBones.begin(), baseBones.end(), b))
					boneList.push_back(b);
			}
		}

		int nCopyBones = step.weightBoneList.empty() ? static_cast<int>(baseBones.size()) : static_cast<int>(step.weightBoneList.size());
		std::vector<std::string> lockedBones;

		project->CopyBoneWeights(shape, step.weightProximityRadius, step.weightMaxResults,
								 mask, boneList, nCopyBones, lockedBones, uss, false);
		shapeIdx++;
	}
	project->morpher.ClearProximityCache();
	return 0;
}

int AutomationDialog::ExecuteStepSetBaseShape(const AutomationStep&) {
	wxLogMessage("Automation: Setting base shape (baking slider values)...");
	outfitStudio->SetBaseShape();
	return 0;
}

int AutomationDialog::ExecuteStepClearReference(const AutomationStep&) {
	wxLogMessage("Automation: Clearing reference...");
	project->DeleteShape(project->GetBaseShape());
	return 0;
}

int AutomationDialog::ExecuteStepTransformShape(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: TransformShape - no target shapes found.");
		return 0;
	}

	nifly::Vector3 move(step.moveX, step.moveY, step.moveZ);
	nifly::Vector3 rotate(step.rotateX, step.rotateY, step.rotateZ);
	nifly::Vector3 scale(step.scaleX, step.scaleY, step.scaleZ);

	for (auto* shape : shapes) {
		wxLogMessage("Automation: Transforming shape '%s'...", shape->name.get());
		if (move.x != 0.0f || move.y != 0.0f || move.z != 0.0f)
			project->OffsetShape(shape, move);
		if (rotate.x != 0.0f || rotate.y != 0.0f || rotate.z != 0.0f)
			project->RotateShape(shape, rotate);
		if (scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f)
			project->ScaleShape(shape, scale);
	}

	// Inflate: move vertices along their normals
	nifly::Vector3 inflate(step.inflateX, step.inflateY, step.inflateZ);
	if (inflate.x != 0.0f || inflate.y != 0.0f || inflate.z != 0.0f) {
		for (auto* shape : shapes) {
			wxLogMessage("Automation: Inflating shape '%s'...", shape->name.get());
			const std::vector<nifly::Vector3>* verts = project->GetWorkNif()->GetVertsForShape(shape);
			const std::vector<nifly::Vector3>* norms = project->GetWorkNif()->GetNormalsForShape(shape);
			if (!verts || !norms || verts->size() != norms->size())
				continue;

			std::vector<nifly::Vector3> newVerts = *verts;
			for (size_t i = 0; i < newVerts.size(); i++) {
				nifly::Vector3 diff = (*norms)[i].ComponentMultiply(inflate);
				newVerts[i] += diff;
			}
			project->GetWorkNif()->SetVertsForShape(shape, newVerts);
		}
	}

	outfitStudio->ApplySliders();
	return 0;
}

int AutomationDialog::ExecuteStepInvertUVs(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: InvertUVs - no target shapes found.");
		return 0;
	}

	if (!step.invertU && !step.invertV) {
		wxLogWarning("Automation: InvertUVs - neither U nor V inversion selected.");
		return 0;
	}

	for (auto* shape : shapes) {
		wxLogMessage("Automation: Inverting UVs for '%s' (U=%d, V=%d)...", shape->name.get(), step.invertU, step.invertV);
		project->GetWorkNif()->InvertUVsForShape(shape, step.invertU, step.invertV);
	}
	return 0;
}

int AutomationDialog::ExecuteStepDeleteBones(const AutomationStep& step) {
	if (step.deleteBoneNames.empty()) {
		wxLogError("Automation: DeleteBones - no bone names specified.");
		return 1;
	}

	if (step.deleteBoneFromProject) {
		for (const auto& boneName : step.deleteBoneNames) {
			wxLogMessage("Automation: Deleting bone '%s' from project...", boneName);
			project->DeleteBone(boneName);
		}
	}
	else {
		auto shapes = ResolveTargetShapes(step);
		if (shapes.empty()) {
			wxLogWarning("Automation: DeleteBones - no target shapes found for weight removal.");
			return 0;
		}
		for (const auto& boneName : step.deleteBoneNames) {
			for (auto* shape : shapes) {
				wxLogMessage("Automation: Removing bone '%s' weights from '%s'...", boneName, shape->name.get());
				project->GetWorkAnim()->RemoveShapeBone(shape->name.get(), boneName);
			}
		}
	}
	return 0;
}

int AutomationDialog::ExecuteStepAddCustomBone(const AutomationStep& step) {
	if (step.addBoneName.empty()) {
		wxLogError("Automation: AddCustomBone - no bone name specified.");
		return 1;
	}

	nifly::MatTransform xform;
	xform.translation = nifly::Vector3(step.addBoneTransX, step.addBoneTransY, step.addBoneTransZ);
	nifly::Vector3 rotVec(step.addBoneRotX, step.addBoneRotY, step.addBoneRotZ);
	xform.rotation = nifly::RotVecToMat(rotVec);

	wxLogMessage("Automation: Adding custom bone '%s' (parent: '%s')...", step.addBoneName, step.addBoneParent);
	project->AddCustomBoneRef(step.addBoneName, step.addBoneParent, xform);
	return 0;
}

int AutomationDialog::ExecuteStepEditBone(const AutomationStep& step) {
	if (step.editBoneName.empty()) {
		wxLogError("Automation: EditBone - no bone name specified.");
		return 1;
	}

	AnimBone* bPtr = AnimSkeleton::getInstance().GetBonePtr(step.editBoneName);
	if (!bPtr) {
		wxLogError("Automation: EditBone - bone '%s' not found.", step.editBoneName);
		return 1;
	}
	if (bPtr->isStandardBone) {
		wxLogError("Automation: EditBone - bone '%s' is a standard bone, cannot edit.", step.editBoneName);
		return 1;
	}

	nifly::MatTransform xform;
	xform.translation = nifly::Vector3(step.editBoneTransX, step.editBoneTransY, step.editBoneTransZ);
	nifly::Vector3 rotVec(step.editBoneRotX, step.editBoneRotY, step.editBoneRotZ);
	xform.rotation = nifly::RotVecToMat(rotVec);

	wxLogMessage("Automation: Editing custom bone '%s' (parent: '%s')...", step.editBoneName, step.editBoneParent);
	project->ModifyCustomBone(bPtr, step.editBoneParent, xform);
	return 0;
}

int AutomationDialog::ExecuteStepRemoveSkinning(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogMessage("Automation: Removing skinning from all shapes...");
		project->RemoveSkinning();
	}
	else {
		for (auto* shape : shapes) {
			wxLogMessage("Automation: Removing skinning from '%s'...", shape->name.get());
			project->RemoveSkinning(shape);
		}
		project->GetWorkNif()->DeleteUnreferencedNodes();
	}
	return 0;
}

int AutomationDialog::ExecuteStepApplyPose(const AutomationStep& step) {
	if (step.poseName.empty()) {
		wxLogError("Automation: ApplyPose - no pose name specified.");
		return 1;
	}

	// Find the pose by name
	PoseData* posePtr = nullptr;
	for (auto& pd : outfitStudio->poseDataCollection.poseData) {
		if (pd.name == step.poseName) {
			posePtr = &pd;
			break;
		}
	}
	if (!posePtr) {
		wxLogError("Automation: ApplyPose - pose '%s' not found.", step.poseName);
		return 1;
	}

	wxLogMessage("Automation: Applying pose '%s' to meshes...", step.poseName);

	// Set bone poses from PoseData
	std::vector<std::string> boneNames;
	AnimSkeleton::getInstance().GetActiveBoneNames(boneNames);
	for (const auto& boneName : boneNames) {
		AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
		if (!bone)
			continue;

		auto it = std::find_if(posePtr->boneData.begin(), posePtr->boneData.end(),
			[&](const PoseBoneData& pbd) { return pbd.name == boneName; });

		if (it != posePtr->boneData.end()) {
			bone->poseRotVec = it->rotation;
			bone->poseTranVec = it->translation;
			bone->poseScale = it->scale;
		}
		else {
			bone->poseRotVec = nifly::Vector3(0.0f, 0.0f, 0.0f);
			bone->poseTranVec = nifly::Vector3(0.0f, 0.0f, 0.0f);
			bone->poseScale = 1.0f;
		}
		bone->UpdatePoseTransform();
	}

	// Enable pose and apply to mesh geometry
	project->bPose = true;

	UndoStateProject usp;
	project->ApplyPoseTransformsToAllShapeGeometry(usp);

	// Reset bone poses after applying
	for (const auto& boneName : boneNames) {
		AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
		if (!bone)
			continue;
		bone->poseRotVec = nifly::Vector3(0.0f, 0.0f, 0.0f);
		bone->poseTranVec = nifly::Vector3(0.0f, 0.0f, 0.0f);
		bone->poseScale = 1.0f;
		bone->UpdatePoseTransform();
	}

	project->bPose = false;
	return 0;
}

int AutomationDialog::ExecuteStepImportSliderData(const AutomationStep& step) {
	if (step.sliderDataFile.empty()) {
		wxLogError("Automation: ImportSliderData - no file/folder specified.");
		return 1;
	}

	wxString sliderDataPath = MakeAbsoluteToProject(wxString::FromUTF8(step.sliderDataFile));
	std::string sliderDataPathStd = sliderDataPath.ToUTF8().data();

	if (step.sliderDataFromFolder) {
		wxLogMessage("Automation: Importing slider data from folder '%s'...", sliderDataPath);
		wxDir dir(sliderDataPath);
		if (!dir.IsOpened()) {
			wxLogError("Automation: ImportSliderData - cannot open folder '%s'.", sliderDataPath);
			return 1;
		}

		int importCount = 0;
		const auto& shapes = project->GetWorkNif()->GetShapes();
		wxString filename;
		bool cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
		while (cont) {
			std::string fname = filename.ToUTF8().data();
			wxFileName wxFn(filename);
			std::string extLower = wxFn.GetExt().Lower().ToUTF8().data();
			std::string fullPath = sliderDataPathStd + "/" + fname;

			if (extLower == "osd") {
				// OSD: import all sliders, auto-mapping shapes by target name
				OSDataFile osd;
				if (osd.Read(fullPath)) {
					auto& diffs = osd.GetDataDiffs();
					for (auto& [diffName, diffData] : diffs) {
						std::string bestTargetName;
						NiShape* bestShape = nullptr;

						for (auto* shape : shapes) {
							std::string targetName = project->ShapeToTarget(shape->name.get());
							if (diffName.substr(0, targetName.size()) == targetName) {
								if (targetName.length() > bestTargetName.length()) {
									bestTargetName = targetName;
									bestShape = shape;
								}
							}
						}

						if (!bestShape || bestTargetName.empty())
							continue;

						auto sliderName = project->activeSet.SliderFromDataName(bestTargetName, diffName);
						if (sliderName.empty())
							sliderName = diffName.substr(bestTargetName.length());

						if (!step.sliderNames.empty()) {
							bool found = false;
							for (const auto& name : step.sliderNames) {
								if (name == sliderName) {
									found = true;
									break;
								}
							}
							if (!found)
								continue;
						}

						if (!project->ValidSlider(sliderName)) {
							if (step.sliderMerge)
								continue;
							project->AddEmptySlider(sliderName);
						}

						if (diffData)
							project->SetSliderFromDiff(sliderName, bestShape, *diffData);

						importCount++;
						wxLogMessage("Automation: Imported slider '%s' for shape '%s'.", sliderName, bestShape->name.get());
					}
				}
				else {
					wxLogWarning("Automation: Failed to read OSD file '%s'.", fname);
				}
			}
			else if (extLower == "tri") {
				// TRI: import all morphs, auto-mapping shapes by name
				TriFile tri;
				if (tri.Read(fullPath)) {
					auto morphs = tri.GetMorphs();
					for (auto& [shapeName, morphList] : morphs) {
						auto* shape = project->GetWorkNif()->FindBlockByName<NiShape>(shapeName);
						if (!shape)
							continue;

						for (auto& morphData : morphList) {
							if (!step.sliderNames.empty()) {
								bool found = false;
								for (const auto& name : step.sliderNames) {
									if (name == morphData->name) {
										found = true;
										break;
									}
								}
								if (!found)
									continue;
							}

							if (!project->ValidSlider(morphData->name)) {
								if (step.sliderMerge)
									continue;
								project->AddEmptySlider(morphData->name);
							}

							std::unordered_map<uint16_t, Vector3> diff(morphData->offsets.begin(), morphData->offsets.end());
							project->SetSliderFromDiff(morphData->name, shape, diff);

							if (morphData->type == MORPHTYPE_UV) {
								size_t sliderIndex = 0;
								if (project->SliderIndexFromName(morphData->name, sliderIndex))
									project->SetSliderUV(sliderIndex, true);
							}

							importCount++;
							wxLogMessage("Automation: Imported morph '%s' for shape '%s'.", morphData->name, shapeName);
						}
					}
				}
				else {
					wxLogWarning("Automation: Failed to read TRI file '%s'.", fname);
				}
			}
			else if (extLower == "bsd" || extLower == "nif" || extLower == "obj" || extLower == "fbx") {
				// NIF/OBJ/FBX/BSD: use "ShapeName#SliderName.ext" naming pattern
				auto hashPos = fname.find('#');
				if (hashPos != std::string::npos) {
					std::string shapeName = fname.substr(0, hashPos);
					std::string rest = fname.substr(hashPos + 1);
					auto dotPos = rest.rfind('.');
					std::string sliderName = (dotPos != std::string::npos) ? rest.substr(0, dotPos) : rest;

					NiShape* shape = FindShapeByName(shapeName);
					if (shape) {
						if (step.sliderMerge && !project->ValidSlider(sliderName)) {
							cont = dir.GetNext(&filename);
							continue;
						}

						if (!project->ValidSlider(sliderName))
							project->AddEmptySlider(sliderName);

						bool ok = false;
						if (extLower == "bsd") {
							project->SetSliderFromBSD(sliderName, shape, fullPath);
							ok = true;
						}
						else if (extLower == "nif") {
							ok = project->SetSliderFromNIF(sliderName, shape, fullPath);
						}
						else if (extLower == "obj") {
							ok = project->SetSliderFromOBJ(sliderName, shape, fullPath);
						}
#ifdef USE_FBXSDK
						else if (extLower == "fbx") {
							ok = project->SetSliderFromFBX(sliderName, shape, fullPath);
						}
#endif

						if (ok) {
							importCount++;
							wxLogMessage("Automation: Imported slider '%s' for shape '%s'.", sliderName, shapeName);
						}
						else {
							wxLogWarning("Automation: Failed to import slider '%s' for shape '%s' from '%s'.", sliderName, shapeName, fname);
						}
					}
					else {
						wxLogWarning("Automation: Shape '%s' not found for file '%s'.", shapeName, fname);
					}
				}
			}

			cont = dir.GetNext(&filename);
		}

		wxLogMessage("Automation: Imported %d slider data file(s) from folder.", importCount);
	}
	else {
		// Single file mode
		wxLogMessage("Automation: Importing slider data from '%s'...", sliderDataPath);
		wxFileName fn(sliderDataPath);
		wxString ext = fn.GetExt().Lower();

		if (ext == "osd") {
			// OSD multi-diff import
			OSDataFile osd;
			if (!osd.Read(sliderDataPathStd)) {
				wxLogError("Automation: Failed to read OSD file '%s'.", sliderDataPath);
				return 1;
			}

			auto& diffs = osd.GetDataDiffs();
			const auto& shapes = project->GetWorkNif()->GetShapes();

			for (auto& [diffName, diffData] : diffs) {
				std::string bestTargetName;
				NiShape* bestShape = nullptr;

				for (auto* shape : shapes) {
					std::string shapeName = shape->name.get();
					std::string targetName = project->ShapeToTarget(shapeName);
					if (diffName.substr(0, targetName.size()) == targetName) {
						if (targetName.length() > bestTargetName.length()) {
							bestTargetName = targetName;
							bestShape = shape;
						}
					}
				}

				if (!bestShape || bestTargetName.empty())
					continue;

				auto sliderName = project->activeSet.SliderFromDataName(bestTargetName, diffName);
				if (sliderName.empty())
					sliderName = diffName.substr(bestTargetName.length());

				if (!step.sliderNames.empty()) {
					bool found = false;
					for (const auto& name : step.sliderNames) {
						if (name == sliderName) {
							found = true;
							break;
						}
					}
					if (!found)
						continue;
				}

				if (!project->ValidSlider(sliderName)) {
					if (step.sliderMerge)
						continue;
					project->AddEmptySlider(sliderName);
				}

				if (diffData)
					project->SetSliderFromDiff(sliderName, bestShape, *diffData);
				wxLogMessage("Automation: Imported slider '%s' for shape '%s'.", sliderName, bestShape->name.get());
			}
		}
		else if (ext == "tri") {
			// TRI multi-morph import
			TriFile tri;
			if (!tri.Read(sliderDataPathStd)) {
				wxLogError("Automation: Failed to read TRI file '%s'.", sliderDataPath);
				return 1;
			}

			auto morphs = tri.GetMorphs();
			for (auto& [shapeName, morphList] : morphs) {
				auto* shape = project->GetWorkNif()->FindBlockByName<NiShape>(shapeName);
				if (!shape)
					continue;

				for (auto& morphData : morphList) {
					if (!step.sliderNames.empty()) {
						bool found = false;
						for (const auto& name : step.sliderNames) {
							if (name == morphData->name) {
								found = true;
								break;
							}
						}
						if (!found)
							continue;
					}

					if (!project->ValidSlider(morphData->name)) {
						if (step.sliderMerge)
							continue;
						project->AddEmptySlider(morphData->name);
					}

					std::unordered_map<uint16_t, Vector3> diff(morphData->offsets.begin(), morphData->offsets.end());
					project->SetSliderFromDiff(morphData->name, shape, diff);

					if (morphData->type == MORPHTYPE_UV) {
						size_t sliderIndex = 0;
						if (project->SliderIndexFromName(morphData->name, sliderIndex))
							project->SetSliderUV(sliderIndex, true);
					}

					wxLogMessage("Automation: Imported morph '%s' for shape '%s'.", morphData->name, shapeName);
				}
			}
		}
		else if (ext == "nif" || ext == "obj" || ext == "bsd" || ext == "fbx") {
			// Per-shape slider import: compute diff from file mesh vs current shape
			std::string sliderName;
			if (!step.sliderNames.empty())
				sliderName = step.sliderNames[0];
			else
				sliderName = fn.GetName().ToUTF8().data();

			if (!project->ValidSlider(sliderName)) {
				if (step.sliderMerge) {
					wxLogWarning("Automation: Slider '%s' does not exist and merge-only is enabled.", sliderName);
					return 0;
				}
				project->AddEmptySlider(sliderName);
			}

			const auto& shapes = project->GetWorkNif()->GetShapes();
			for (auto* shape : shapes) {
				bool ok = false;
				if (ext == "nif")
					ok = project->SetSliderFromNIF(sliderName, shape, sliderDataPathStd);
				else if (ext == "obj")
					ok = project->SetSliderFromOBJ(sliderName, shape, sliderDataPathStd);
				else if (ext == "bsd") {
					project->SetSliderFromBSD(sliderName, shape, sliderDataPathStd);
					ok = true;
				}
#ifdef USE_FBXSDK
				else if (ext == "fbx")
					ok = project->SetSliderFromFBX(sliderName, shape, sliderDataPathStd);
#endif

				if (ok)
					wxLogMessage("Automation: Imported slider '%s' for shape '%s'.", sliderName, shape->name.get());
			}
		}
		else {
			wxLogError("Automation: ImportSliderData - unsupported file format '.%s'.", ext);
			return 1;
		}
	}
	return 0;
}

int AutomationDialog::ExecuteStepImportFile(const AutomationStep& step) {
	if (step.importFilePath.empty()) {
		wxLogError("Automation: ImportFile - no file/folder specified.");
		return 1;
	}

	wxString importFilePath = MakeAbsoluteToProject(wxString::FromUTF8(step.importFilePath));
	std::string importFilePathStd = importFilePath.ToUTF8().data();

	if (step.importFromFolder) {
		// Folder mode: import all NIF/OBJ/FBX files from the folder
		wxLogMessage("Automation: Importing all files from folder '%s'...", importFilePath);
		wxDir dir(importFilePath);
		if (!dir.IsOpened()) {
			wxLogError("Automation: ImportFile - cannot open folder '%s'.", importFilePath);
			return 1;
		}

		int importCount = 0;
		wxString filename;
		bool cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
		while (cont) {
			wxFileName fn(filename);
			wxString ext = fn.GetExt().Lower();
			if (ext == "nif" || ext == "obj" || ext == "fbx") {
				std::string fullPath = importFilePathStd + "/" + filename.ToUTF8().data();
				int err = 0;
				if (ext == "nif")
					err = project->ImportNIF(fullPath, false);
				else if (ext == "obj")
					err = project->ImportOBJ(fullPath);
				else if (ext == "fbx")
#ifdef USE_FBXSDK
					err = project->ImportFBX(fullPath);
#else
					wxLogError("Automation: FBX import is not available (FBX SDK not compiled in).");
#endif

				if (err)
					wxLogWarning("Automation: Failed to import '%s' (error %d).", fullPath, err);
				else
					importCount++;
			}
			cont = dir.GetNext(&filename);
		}

		wxLogMessage("Automation: Imported %d file(s) from folder.", importCount);
	}
	else {
		// Single file mode
		wxFileName fn(importFilePath);
		wxString ext = fn.GetExt().Lower();
		int err = 0;

		if (ext == "nif")
			err = project->ImportNIF(importFilePathStd, false);
		else if (ext == "obj")
			err = project->ImportOBJ(importFilePathStd);
		else if (ext == "fbx")
#ifdef USE_FBXSDK
			err = project->ImportFBX(importFilePathStd);
#else
			wxLogError("Automation: FBX import is not available (FBX SDK not compiled in).");
#endif
		else {
			wxLogError("Automation: ImportFile - unsupported file extension '%s'.", ext);
			return 1;
		}

		if (err) {
			wxLogError("Automation: ImportFile '%s' failed with error %d.", importFilePath, err);
			return err;
		}
	}
	return 0;
}

int AutomationDialog::ExecuteStepDeleteShape(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: DeleteShape - no target shapes found.");
		return 0;
	}

	for (auto* shape : shapes) {
		wxLogMessage("Automation: Deleting shape '%s'...", shape->name.get());
		project->DeleteShape(shape);
	}
	return 0;
}

int AutomationDialog::ExecuteStepRenameShape(const AutomationStep& step) {
	if (step.renameOldName.empty() || step.renameNewName.empty()) {
		wxLogError("Automation: RenameShape - old or new name not specified.");
		return 1;
	}

	NiShape* shape = FindShapeByName(step.renameOldName);
	if (!shape) {
		wxLogError("Automation: RenameShape - shape '%s' not found.", step.renameOldName);
		return 1;
	}

	wxLogMessage("Automation: Renaming shape '%s' to '%s'...", step.renameOldName, step.renameNewName);
	project->RenameShape(shape, step.renameNewName);
	return 0;
}

int AutomationDialog::ExecuteStepSaveProject(const AutomationStep& step) {
	if (step.saveSliderSetFile.empty() || step.saveOutputFileName.empty()) {
		wxLogError("Automation: SaveProject - required fields not filled.");
		return 1;
	}

	wxLogMessage("Automation: Saving project '%s'...", step.saveName);

	wxFileName sliderSetFile(wxString::FromUTF8(step.saveSliderSetFile));
	wxString strOutfitName = wxString::FromUTF8(step.saveName);
	wxString strDataDir = wxString::FromUTF8(step.saveShapeDataFolder);
	wxString strBaseFile = wxString::FromUTF8(step.saveShapeDataFile);
	wxString strGamePath = wxString::FromUTF8(step.saveOutputDataPath);
	wxString strGameFile = wxString::FromUTF8(step.saveOutputFileName);

	std::string result = project->Save(sliderSetFile, strOutfitName, strDataDir,
									   strBaseFile, strGamePath, strGameFile,
									   step.saveGenWeights, step.saveAutoCopyRef,
									   false, false);

	if (!result.empty()) {
		wxLogError("Automation: SaveProject error: %s", result);
		return 1;
	}
	return 0;
}

int AutomationDialog::ExecuteStepExportFile(const AutomationStep& step) {
	if (step.exportFilePath.empty()) {
		wxLogError("Automation: ExportFile - no file path specified.");
		return 1;
	}

	// Apply prefix/suffix to filename
	std::string exportPath = step.exportFilePath;
	if (!step.exportPrefix.empty() || !step.exportSuffix.empty()) {
		wxFileName fn(wxString::FromUTF8(exportPath));
		wxString name = fn.GetName();
		if (!step.exportPrefix.empty())
			name = wxString::FromUTF8(step.exportPrefix) + name;
		if (!step.exportSuffix.empty())
			name = name + wxString::FromUTF8(step.exportSuffix);
		fn.SetName(name);
		exportPath = fn.GetFullPath().ToUTF8().data();
	}

	wxFileName fn(wxString::FromUTF8(exportPath));
	wxString ext = fn.GetExt().Lower();

	wxLogMessage("Automation: Exporting to '%s'...", exportPath);

	if (ext == "nif") {
		std::vector<Mesh*> shapeMeshes;
		for (auto* s : project->GetWorkNif()->GetShapes()) {
			if (step.exportWithRef || !project->IsBaseShape(s)) {
				Mesh* m = outfitStudio->glView->GetMesh(s->name.get());
				if (m)
					shapeMeshes.push_back(m);
			}
		}
		int err = project->ExportNIF(exportPath, shapeMeshes, step.exportWithRef);
		if (err) {
			wxLogError("Automation: ExportNIF failed with error %d.", err);
			return err;
		}
	}
	else if (ext == "obj") {
		auto shapes = project->GetWorkNif()->GetShapes();
		int err = project->ExportOBJ(exportPath, shapes, false);
		if (err) {
			wxLogError("Automation: ExportOBJ failed with error %d.", err);
			return err;
		}
	}
	else if (ext == "fbx") {
#ifdef USE_FBXSDK
		auto shapes = project->GetWorkNif()->GetShapes();
		int err = project->ExportFBX(exportPath, shapes, false);
		if (err) {
			wxLogError("Automation: ExportFBX failed with error %d.", err);
			return err;
		}
#else
		wxLogError("Automation: FBX export is not available (FBX SDK not compiled in).");
		return 1;
#endif
	}
	else if (ext == "osd") {
		bool ok = project->SaveSliderData(exportPath);
		if (!ok) {
			wxLogError("Automation: SaveSliderData failed.");
			return 1;
		}
	}
	else if (ext == "tri") {
		bool ok = project->WriteMorphTRI(exportPath);
		if (!ok) {
			wxLogError("Automation: WriteMorphTRI failed.");
			return 1;
		}
	}
	else {
		wxLogError("Automation: ExportFile - unsupported file extension '%s'.", ext);
		return 1;
	}
	return 0;
}

int AutomationDialog::ExecuteStepRefineMesh(const AutomationStep&) {
	wxLogMessage("Automation: Refining meshes...");

	auto workNif = project->GetWorkNif();
	if (!workNif) {
		wxLogError("Automation: RefineMesh - no work NIF loaded.");
		return 1;
	}

	constexpr size_t maxVertIndex = std::numeric_limits<uint16_t>().max();
	size_t maxTriIndex = std::numeric_limits<uint16_t>().max();
	if (workNif->GetHeader().GetVersion().IsFO4() || workNif->GetHeader().GetVersion().IsFO76())
		maxTriIndex = std::numeric_limits<uint32_t>().max();

	for (auto* shape : workNif->GetShapes()) {
		size_t nverts = shape->GetNumVertices();

		// Determine unmasked vertices
		std::unordered_map<uint16_t, float> mask;
		outfitStudio->glView->GetShapeUnmasked(mask, shape->name.get());
		std::vector<bool> pincs(nverts, false);
		for (auto& m : mask)
			pincs[m.first] = true;

		std::vector<Triangle> tris;
		shape->GetTriangles(tris);
		size_t nedges = 0;
		for (Triangle& tri : tris) {
			int ntripts = pincs[tri.p1] + pincs[tri.p2] + pincs[tri.p3];
			if (ntripts == 3)
				nedges += 3;
			else if (ntripts == 2)
				nedges += 1;
		}

		if (nverts + nedges > maxVertIndex || shape->GetNumTriangles() + nedges > maxTriIndex) {
			wxLogWarning("Automation: RefineMesh - shape '%s' would exceed vertex/triangle limits, skipping.", shape->name.get());
			continue;
		}

		Mesh* m = outfitStudio->glView->GetMesh(shape->name.get());
		if (!m)
			continue;

		UndoStateShape uss;
		uss.shapeName = shape->name.get();
		if (!project->PrepareRefineMesh(shape, uss, pincs, m->weldVerts, false)) {
			wxLogWarning("Automation: RefineMesh - shape '%s' has orientation issues, skipping.", shape->name.get());
			continue;
		}

		std::vector<float> emptyMask;
		project->ApplyShapeMeshUndo(shape, emptyMask, uss, false);
	}

	outfitStudio->ApplySliders();
	return 0;
}

int AutomationDialog::ExecuteStepDeleteSlider(const AutomationStep& step) {
	if (step.deleteSliderName.empty()) {
		wxLogError("Automation: DeleteSlider - no slider name specified.");
		return 1;
	}

	if (step.deleteSliderRegex) {
		try {
			std::regex re(step.deleteSliderName, std::regex::icase);
			std::vector<std::string> sliderList;
			project->GetSliderList(sliderList);
			int deleted = 0;
			for (const auto& name : sliderList) {
				if (std::regex_search(name, re)) {
					wxLogMessage("Automation: Deleting slider '%s'...", name);
					project->DeleteSlider(name);
					deleted++;
				}
			}
			wxLogMessage("Automation: Deleted %d slider(s) matching '%s'.", deleted, step.deleteSliderName);
		}
		catch (const std::regex_error&) {
			wxLogError("Automation: DeleteSlider - invalid regex '%s'.", step.deleteSliderName);
			return 1;
		}
	}
	else {
		wxLogMessage("Automation: Deleting slider '%s'...", step.deleteSliderName);
		project->DeleteSlider(step.deleteSliderName);
	}
	return 0;
}

int AutomationDialog::ExecuteStepSetReferenceShape(const AutomationStep& step) {
	if (step.setRefShapeName.empty()) {
		wxLogError("Automation: SetReferenceShape - no shape name specified.");
		return 1;
	}

	NiShape* shape = FindShapeByName(step.setRefShapeName);
	if (!shape) {
		wxLogError("Automation: SetReferenceShape - shape '%s' not found.", step.setRefShapeName);
		return 1;
	}

	wxLogMessage("Automation: Setting reference shape to '%s'...", step.setRefShapeName);
	project->SetBaseShape(shape);
	return 0;
}

int AutomationDialog::ExecuteStepResetTransforms(const AutomationStep&) {
	wxLogMessage("Automation: Resetting transforms...");
	project->ResetTransforms();
	return 0;
}

int AutomationDialog::ExecuteStepDuplicateShape(const AutomationStep& step) {
	if (step.dupNewName.empty()) {
		wxLogError("Automation: DuplicateShape - no new name specified.");
		return 1;
	}

	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: DuplicateShape - no target shapes found.");
		return 0;
	}

	for (auto* shape : shapes) {
		std::string newName = step.dupNewName;
		// If duplicating multiple shapes, append original name to avoid duplicates
		if (shapes.size() > 1)
			newName = step.dupNewName + "_" + shape->name.get();

		if (project->IsValidShape(newName)) {
			wxLogWarning("Automation: DuplicateShape - shape '%s' already exists, skipping.", newName);
			continue;
		}

		wxLogMessage("Automation: Duplicating shape '%s' as '%s'...", shape->name.get(), newName);
		project->DuplicateShape(shape, newName);
	}
	return 0;
}

int AutomationDialog::ExecuteStepMirrorShape(const AutomationStep& step) {
	if (!step.mirrorX && !step.mirrorY && !step.mirrorZ) {
		wxLogWarning("Automation: MirrorShape - no mirror axis selected.");
		return 0;
	}

	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: MirrorShape - no target shapes found.");
		return 0;
	}

	for (auto* shape : shapes) {
		wxLogMessage("Automation: Mirroring shape '%s' (X=%d, Y=%d, Z=%d, SwapBones=%d)...",
			shape->name.get(), step.mirrorX, step.mirrorY, step.mirrorZ, step.mirrorSwapBonesX);
		project->GetWorkNif()->MirrorShape(shape, step.mirrorX, step.mirrorY, step.mirrorZ);
		if (step.mirrorSwapBonesX)
			project->GetWorkAnim()->SwapBonesLR(shape->name.get());
	}
	return 0;
}

int AutomationDialog::ExecuteStepLoadMask(const AutomationStep& step) {
	if (step.loadMaskFile.empty()) {
		wxLogError("Automation: LoadMask - no mask file specified.");
		return 1;
	}

	if (step.loadMaskName.empty()) {
		wxLogError("Automation: LoadMask - no mask name specified.");
		return 1;
	}

	wxString loadMaskFile = MakeAbsoluteToProject(wxString::FromUTF8(step.loadMaskFile));
	std::string loadMaskFileStd = loadMaskFile.ToUTF8().data();
	MaskFile maskFile;
	int maskErr = maskFile.Load(loadMaskFileStd);
	if (maskErr) {
		wxLogError("Automation: LoadMask - failed to load file '%s' (error %d).", loadMaskFile, maskErr);
		return 1;
	}

	const MaskEntry* entry = maskFile.FindEntry(step.loadMaskName);
	if (!entry) {
		wxLogError("Automation: LoadMask - mask name '%s' not found in file '%s'.",
			step.loadMaskName, loadMaskFile);
		return 1;
	}

	auto targetShapes = ResolveTargetShapes(step);
	if (targetShapes.empty()) {
		wxLogWarning("Automation: LoadMask - no target shapes found.");
		return 0;
	}

	for (auto* shape : targetShapes) {
		std::string shapeName = shape->name.get();
		Mesh* mesh = outfitStudio->glView->GetMesh(shapeName);
		if (!mesh)
			continue;

		const MaskShapeData* matched = entry->FindMatchingMask(shapeName, mesh->nVerts);
		if (!matched) {
			wxLogWarning("Automation: LoadMask - no matching mask found for shape '%s' (vertex count: %d).",
				shapeName, mesh->nVerts);
			continue;
		}

		wxLogMessage("Automation: Loading mask '%s' onto shape '%s' (matched from '%s')...",
			step.loadMaskName, shapeName, matched->name);
		auto maskCopy = matched->mask;
		outfitStudio->glView->SetShapeMask(maskCopy, shapeName);
	}

	outfitStudio->glView->Render();
	return 0;
}

int AutomationDialog::ExecuteStepSetSliderProperties(const AutomationStep& step) {
	wxLogMessage("Automation: Setting slider properties...");

	for (size_t i = 0; i < project->SliderCount(); i++) {
		std::string name = project->GetSliderName(i);

		// If specific slider names given, check if this one matches
		if (!step.sliderPropNames.empty()) {
			bool found = false;
			for (const auto& n : step.sliderPropNames) {
				if (n == name) {
					found = true;
					break;
				}
			}
			if (!found)
				continue;
		}

		if (step.sliderPropZap >= 0) {
			project->SetSliderZap(i, step.sliderPropZap != 0);
			wxLogMessage("Automation: Slider '%s' zap = %s.", name, step.sliderPropZap ? "true" : "false");
		}
		if (step.sliderPropHidden >= 0) {
			project->SetSliderHidden(i, step.sliderPropHidden != 0);
			wxLogMessage("Automation: Slider '%s' hidden = %s.", name, step.sliderPropHidden ? "true" : "false");
		}
		if (step.sliderPropDefaultLo >= 0) {
			project->SetSliderDefault(i, step.sliderPropDefaultLo, false);
			wxLogMessage("Automation: Slider '%s' default (small) = %d.", name, step.sliderPropDefaultLo);
		}
		if (step.sliderPropDefaultHi >= 0) {
			project->SetSliderDefault(i, step.sliderPropDefaultHi, true);
			wxLogMessage("Automation: Slider '%s' default (big) = %d.", name, step.sliderPropDefaultHi);
		}
	}

	return 0;
}

int AutomationDialog::ExecuteStepRemoveUnusedNodes(const AutomationStep&) {
	wxLogMessage("Automation: Removing unused nodes...");
	int deletionCount = 0;
	auto workNif = project->GetWorkNif();
	if (workNif)
		workNif->DeleteUnreferencedNodes(&deletionCount);
	wxLogMessage("Automation: %d unreferenced nodes removed.", deletionCount);
	return 0;
}

int AutomationDialog::ExecuteStep(const AutomationStep& step) {
	switch (step.type) {
		case AutomationStepType::ClearProject: return ExecuteStepClearProject(step);
		case AutomationStepType::LoadReference: return ExecuteStepLoadReference(step);
		case AutomationStepType::AddProject: return ExecuteStepAddProject(step);
		case AutomationStepType::SetSliderValues: return ExecuteStepSetSliderValues(step);
		case AutomationStepType::ConformSliders: return ExecuteStepConformSliders(step);
		case AutomationStepType::CopyBoneWeights: return ExecuteStepCopyBoneWeights(step);
		case AutomationStepType::SetBaseShape: return ExecuteStepSetBaseShape(step);
		case AutomationStepType::ClearReference: return ExecuteStepClearReference(step);
		case AutomationStepType::TransformShape: return ExecuteStepTransformShape(step);
		case AutomationStepType::InvertUVs: return ExecuteStepInvertUVs(step);
		case AutomationStepType::DeleteBones: return ExecuteStepDeleteBones(step);
		case AutomationStepType::AddCustomBone: return ExecuteStepAddCustomBone(step);
		case AutomationStepType::EditBone: return ExecuteStepEditBone(step);
		case AutomationStepType::RemoveSkinning: return ExecuteStepRemoveSkinning(step);
		case AutomationStepType::ApplyPose: return ExecuteStepApplyPose(step);
		case AutomationStepType::ImportSliderData: return ExecuteStepImportSliderData(step);
		case AutomationStepType::ImportFile: return ExecuteStepImportFile(step);
		case AutomationStepType::DeleteShape: return ExecuteStepDeleteShape(step);
		case AutomationStepType::RenameShape: return ExecuteStepRenameShape(step);
		case AutomationStepType::SaveProject: return ExecuteStepSaveProject(step);
		case AutomationStepType::ExportFile: return ExecuteStepExportFile(step);
		case AutomationStepType::RefineMesh: return ExecuteStepRefineMesh(step);
		case AutomationStepType::DeleteSlider: return ExecuteStepDeleteSlider(step);
		case AutomationStepType::SetReferenceShape: return ExecuteStepSetReferenceShape(step);
		case AutomationStepType::ResetTransforms: return ExecuteStepResetTransforms(step);
		case AutomationStepType::DuplicateShape: return ExecuteStepDuplicateShape(step);
		case AutomationStepType::MirrorShape: return ExecuteStepMirrorShape(step);
		case AutomationStepType::LoadMask: return ExecuteStepLoadMask(step);
		case AutomationStepType::SetSliderProperties: return ExecuteStepSetSliderProperties(step);
		case AutomationStepType::RemoveUnusedNodes: return ExecuteStepRemoveUnusedNodes(step);
	}

	return 0;
}

void AutomationDialog::OnAddVariable(wxCommandEvent& WXUNUSED(event)) {
	if (varRowCount >= 10)
		return;

	varRowCount++;

	auto* pane = XRCCTRL(*this, "paneVariables", wxCollapsiblePane);
	if (!pane)
		return;

	wxWindow* paneWin = pane->GetPane();
	if (!paneWin)
		return;

	// The sizer hierarchy: panewindow -> wxBoxSizer -> item[1] -> wxFlexGridSizer
	wxSizer* boxSizer = paneWin->GetSizer();
	if (!boxSizer || boxSizer->GetItemCount() < 2)
		return;

	auto* gridSizer = dynamic_cast<wxFlexGridSizer*>(boxSizer->GetItem(static_cast<size_t>(1))->GetSizer());
	if (!gridSizer)
		return;

	// The grid's containing window is the pane window
	wxWindow* parent = paneWin;

	wxString keyName = wxString::Format("txtVarKey%d", varRowCount);
	wxString valName = wxString::Format("txtVarVal%d", varRowCount);

	auto* keyCtrl = new wxTextCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, keyName);
	auto* valCtrl = new wxTextCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, valName);

	keyCtrl->SetName(keyName);
	valCtrl->SetName(valName);

	gridSizer->Add(keyCtrl, 0, wxEXPAND);
	gridSizer->Add(valCtrl, 0, wxEXPAND);

	paneWin->Layout();
	pane->InvalidateBestSize();
	Layout();
}

void AutomationDialog::OnRemoveVariable(wxCommandEvent& WXUNUSED(event)) {
	if (varRowCount <= 1)
		return;

	auto* pane = XRCCTRL(*this, "paneVariables", wxCollapsiblePane);
	if (!pane)
		return;

	wxWindow* paneWin = pane->GetPane();
	if (!paneWin)
		return;

	wxSizer* boxSizer = paneWin->GetSizer();
	if (!boxSizer || boxSizer->GetItemCount() < 2)
		return;

	auto* gridSizer = dynamic_cast<wxFlexGridSizer*>(boxSizer->GetItem(static_cast<size_t>(1))->GetSizer());
	if (!gridSizer)
		return;

	wxString keyName = wxString::Format("txtVarKey%d", varRowCount);
	wxString valName = wxString::Format("txtVarVal%d", varRowCount);
	auto* keyCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(keyName));
	auto* valCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(valName));

	if (keyCtrl) {
		gridSizer->Detach(keyCtrl);
		keyCtrl->Destroy();
	}
	if (valCtrl) {
		gridSizer->Detach(valCtrl);
		valCtrl->Destroy();
	}

	varRowCount--;

	paneWin->Layout();
	pane->InvalidateBestSize();
	Layout();
}

void AutomationDialog::PopulateRefTemplates() {
	auto* choice = XRCCTRL(*this, "choiceRefTemplate", wxChoice);
	if (!choice)
		return;

	choice->Clear();
	choice->Append("(None - use file below)");

	for (const auto& tmpl : outfitStudio->GetRefTemplates())
		choice->Append(wxString::FromUTF8(tmpl.GetName()));

	choice->SetSelection(0);
}

void AutomationDialog::OnRefTemplateChanged(wxCommandEvent& WXUNUSED(event)) {
	auto* choice = XRCCTRL(*this, "choiceRefTemplate", wxChoice);
	if (!choice)
		return;

	int sel = choice->GetSelection();
	if (sel <= 0)
		return; // "(None)" selected

	size_t tmplIdx = static_cast<size_t>(sel - 1);
	if (tmplIdx >= outfitStudio->GetRefTemplates().size())
		return;

	const auto& tmpl = outfitStudio->GetRefTemplates()[tmplIdx];

	// Fill source file
	auto* fp = XRCCTRL(*this, "fpRefSourceFile", wxFilePickerCtrl);
	if (fp) {
		wxString source = wxString::FromUTF8(tmpl.GetSource());
		if (!source.IsEmpty()) {
			wxFileName fn(source);
			if (fn.IsRelative()) {
				std::string projPath = GetProjectPath();
				fn.MakeAbsolute(wxString::FromUTF8(projPath));
			}
			fp->SetPath(fn.GetFullPath());
		}

		// Populate sets from the source file
		PopulateSetsFromFile(fp->GetPath(), "choiceRefSet", "choiceRefShape");
	}

	// Select slider set
	auto* choiceSet = XRCCTRL(*this, "choiceRefSet", wxChoice);
	if (choiceSet) {
		wxString setName = wxString::FromUTF8(tmpl.GetSetName());
		int idx = choiceSet->FindString(setName);
		if (idx != wxNOT_FOUND)
			choiceSet->SetSelection(idx);

		// Populate shapes
		if (fp)
			PopulateRefShapesForSet(fp->GetPath(), setName);
	}

	// Select shape
	auto* choiceShape = XRCCTRL(*this, "choiceRefShape", wxChoice);
	if (choiceShape) {
		wxString shapeName = wxString::FromUTF8(tmpl.GetShape());
		int idx = choiceShape->FindString(shapeName);
		if (idx != wxNOT_FOUND)
			choiceShape->SetSelection(idx);
	}

	// Set load all checkbox
	auto* chk = XRCCTRL(*this, "chkRefLoadAll", wxCheckBox);
	if (chk)
		chk->SetValue(tmpl.GetLoadAll());
}

void AutomationDialog::OnRefSourceFileChanged(wxFileDirPickerEvent& event) {
	wxString filePath = event.GetPath();
	PopulateSetsFromFile(filePath, "choiceRefSet", "choiceRefShape");

	// Reset template dropdown to "(None)" since user chose a file manually
	auto* choiceTemplate = XRCCTRL(*this, "choiceRefTemplate", wxChoice);
	if (choiceTemplate)
		choiceTemplate->SetSelection(0);
}

void AutomationDialog::OnRefSetChanged(wxCommandEvent& WXUNUSED(event)) {
	auto* choiceSet = XRCCTRL(*this, "choiceRefSet", wxChoice);
	auto* fp = XRCCTRL(*this, "fpRefSourceFile", wxFilePickerCtrl);
	if (!choiceSet || !fp)
		return;

	wxString setName = choiceSet->GetStringSelection();
	PopulateRefShapesForSet(fp->GetPath(), setName);
}

void AutomationDialog::PopulateSetsFromFile(const wxString& filePath, const char* choiceName, const char* shapesChoiceName) {
	auto* choiceSet = XRCCTRL(*this, choiceName, wxChoice);
	if (!choiceSet)
		return;

	choiceSet->Clear();

	wxChoice* choiceShape = nullptr;
	if (shapesChoiceName) {
		choiceShape = XRCCTRL(*this, shapesChoiceName, wxChoice);
		if (choiceShape)
			choiceShape->Clear();
	}

	if (filePath.IsEmpty())
		return;

	wxFileName fn(filePath);
	wxString ext = fn.GetExt().Lower();
	if (ext == "nif")
		return; // NIF files don't have slider sets

	SliderSetFile ssf(filePath.ToUTF8().data());
	if (ssf.fail())
		return;

	std::vector<std::string> setNames;
	ssf.GetSetNamesUnsorted(setNames);

	for (const auto& name : setNames)
		choiceSet->Append(wxString::FromUTF8(name));

	if (!setNames.empty()) {
		choiceSet->SetSelection(0);
		if (shapesChoiceName)
			PopulateRefShapesForSet(filePath, choiceSet->GetStringSelection());
	}
}

void AutomationDialog::PopulateRefShapesForSet(const wxString& filePath, const wxString& setName) {
	auto* choiceShape = XRCCTRL(*this, "choiceRefShape", wxChoice);
	if (!choiceShape)
		return;

	choiceShape->Clear();

	if (filePath.IsEmpty() || setName.IsEmpty())
		return;

	SliderSetFile ssf(filePath.ToUTF8().data());
	if (ssf.fail())
		return;

	std::vector<std::string> shapes;
	ssf.SetShapes(setName.ToUTF8().data(), shapes);

	for (const auto& shape : shapes)
		choiceShape->Append(wxString::FromUTF8(shape));

	if (!shapes.empty())
		choiceShape->SetSelection(0);
}

void AutomationDialog::OnAddProjSourceFileChanged(wxFileDirPickerEvent& event) {
	wxString filePath = event.GetPath();
	PopulateSetsFromFile(filePath, "choiceAddProjSet");
}

void AutomationDialog::OnAddProjSetChanged(wxCommandEvent& WXUNUSED(event)) {
	// Nothing to do — AddProject has no shape picker
}

void AutomationDialog::OnImportFolderChanged(wxCommandEvent& WXUNUSED(event)) {
	auto* chk = XRCCTRL(*this, "chkImportFromFolder", wxCheckBox);
	if (chk)
		UpdateImportFolderVisibility(chk->GetValue());
}

void AutomationDialog::OnSliderDataFolderChanged(wxCommandEvent& WXUNUSED(event)) {
	auto* chk = XRCCTRL(*this, "chkSliderDataFromFolder", wxCheckBox);
	if (chk)
		UpdateSliderDataFolderVisibility(chk->GetValue());
}

void AutomationDialog::OnSaveUseOriginalChanged(wxCommandEvent& WXUNUSED(event)) {
	auto* chk = XRCCTRL(*this, "chkSaveUseOriginal", wxCheckBox);
	if (chk) {
		UpdateSaveFieldsEnabled(!chk->GetValue());

		// Default copy-ref-from-project to on when use-original is checked
		if (chk->GetValue())
			SetCheckboxValue("chkSaveCopyRefFromProject", true);
	}
}

void AutomationDialog::OnExportUseOriginalChanged(wxCommandEvent& WXUNUSED(event)) {
	auto* chk = XRCCTRL(*this, "chkExportUseOriginalPath", wxCheckBox);
	if (chk)
		UpdateExportFieldsEnabled(!chk->GetValue());
}

void AutomationDialog::OnSliderPropZapChanged(wxCommandEvent& WXUNUSED(event)) {
	UpdateSliderPropDefaultVisibility();
}

void AutomationDialog::UpdateSliderPropDefaultVisibility() {
	auto* choiceZap = XRCCTRL(*this, "choiceSliderPropZap", wxChoice);
	bool isZap = choiceZap && choiceZap->GetSelection() == 2; // "Yes"

	auto showCtrl = [this](const char* name, bool show) {
		auto* win = FindWindow(name);
		if (win)
			win->Show(show);
	};

	showCtrl("lblSliderPropZapped", isZap);
	showCtrl("choiceSliderPropZapped", isZap);
	showCtrl("lblSliderPropDefaultLo", !isZap);
	showCtrl("txtSliderPropDefaultLo", !isZap);
	showCtrl("lblSliderPropDefaultHi", !isZap);
	showCtrl("txtSliderPropDefaultHi", !isZap);

	auto* panel = XRCCTRL(*this, "pageSetSliderProperties", wxPanel);
	if (panel)
		panel->Layout();
}

void AutomationDialog::OnLoadMaskFileChanged(wxFileDirPickerEvent& event) {
	wxString filePath = event.GetPath();
	PopulateMaskNamesFromFile(filePath);
}

void AutomationDialog::PopulateMaskNamesFromFile(const wxString& filePath) {
	auto* choice = XRCCTRL(*this, "choiceLoadMaskName", wxChoice);
	if (!choice)
		return;

	choice->Clear();

	if (filePath.IsEmpty())
		return;

	MaskFile maskFile;
	if (maskFile.Load(filePath.ToUTF8().data()) == 0) {
		for (const auto& entry : maskFile.GetEntries())
			choice->Append(wxString::FromUTF8(entry.name));

		if (choice->GetCount() > 0)
			choice->SetSelection(0);
	}
}

void AutomationDialog::UpdateImportFolderVisibility(bool fromFolder) {
	auto setEnabled = [this](const char* name, bool enabled) {
		auto* win = FindWindow(name);
		if (win)
			win->Enable(enabled);
	};

	setEnabled("fpImportFile", !fromFolder);
	setEnabled("dpImportFolder", fromFolder);
}

void AutomationDialog::UpdateSliderDataFolderVisibility(bool fromFolder) {
	auto setEnabled = [this](const char* name, bool enabled) {
		auto* win = FindWindow(name);
		if (win)
			win->Enable(enabled);
	};

	setEnabled("fpSliderDataFile", !fromFolder);
	setEnabled("dpSliderDataFolder", fromFolder);
}

void AutomationDialog::UpdateSaveFieldsEnabled(bool enabled) {
	bool useOriginal = !enabled;

	auto* originalFields = FindWindow("panelSaveOriginalFields");
	if (originalFields)
		originalFields->Show(!useOriginal);

	auto* batchFields = FindWindow("panelSaveBatchFields");
	if (batchFields)
		batchFields->Show(useOriginal);

	auto* page = FindWindow("pageSaveProject");
	if (page)
		page->Layout();
}

void AutomationDialog::UpdateExportFieldsEnabled(bool enabled) {
	auto* fp = XRCCTRL(*this, "fpExportFile", wxFilePickerCtrl);
	if (fp)
		fp->Enable(enabled);

	auto* dp = XRCCTRL(*this, "dpExportFolder", wxDirPickerCtrl);
	if (dp)
		dp->Enable(enabled);
}

void AutomationDialog::UpdateExportForBatchMode() {
	bool isBatch = radioBatchMode && radioBatchMode->GetSelection() != 0;

	auto* lbl = XRCCTRL(*this, "lblExportPath", wxStaticText);
	if (lbl)
		lbl->SetLabel(isBatch ? _("Export Folder:") : _("Export File Path:"));

	auto* fp = XRCCTRL(*this, "fpExportFile", wxFilePickerCtrl);
	if (fp)
		fp->Show(!isBatch);

	auto* dp = XRCCTRL(*this, "dpExportFolder", wxDirPickerCtrl);
	if (dp)
		dp->Show(isBatch);

	auto* chk = XRCCTRL(*this, "chkExportUseOriginalPath", wxCheckBox);
	if (chk) {
		chk->Show(isBatch);
		if (!isBatch)
			chk->SetValue(false);
	}

	auto* page = XRCCTRL(*this, "pageExportFile", wxPanel);
	if (page)
		page->Layout();
}

wxString AutomationDialog::MakeRelativeToProject(const wxString& absolutePath) const {
	if (absolutePath.empty())
		return absolutePath;

	wxFileName fn(absolutePath);

	if (fn.IsRelative())
		return absolutePath; // Already relative

	if (fn.MakeRelativeTo(wxString::FromUTF8(GetProjectPath())))
		return fn.GetFullPath();

	return absolutePath; // Couldn't make relative, return as-is
}

wxString AutomationDialog::MakeAbsoluteToProject(const wxString& path) const {
	if (path.empty())
		return path;

	wxFileName fn(path);
	if (!fn.IsRelative())
		return path;

	fn.MakeAbsolute(wxString::FromUTF8(GetProjectPath()));
	return fn.GetFullPath();
}

void AutomationDialog::PopulateVariablesUI() {
	const auto& vars = script.GetVariables();

	// Clear existing rows (set them empty)
	for (int i = 1; i <= 10; i++) {
		wxString keyName = wxString::Format("txtVarKey%d", i);
		wxString valName = wxString::Format("txtVarVal%d", i);
		auto* keyCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(keyName));
		auto* valCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(valName));
		if (keyCtrl)
			keyCtrl->SetValue(wxEmptyString);
		if (valCtrl)
			valCtrl->SetValue(wxEmptyString);
	}

	// Populate from script variables
	int row = 1;
	for (const auto& [key, val] : vars) {
		if (row > 10)
			break;

		wxString keyName = wxString::Format("txtVarKey%d", row);
		wxString valName = wxString::Format("txtVarVal%d", row);
		auto* keyCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(keyName));
		auto* valCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(valName));

		if (!keyCtrl || !valCtrl) {
			// Need to add a row by simulating Add Variable click
			wxCommandEvent evt;
			OnAddVariable(evt);
			keyCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(keyName));
			valCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(valName));
		}

		if (keyCtrl)
			keyCtrl->SetValue(wxString::FromUTF8(key));
		if (valCtrl)
			valCtrl->SetValue(wxString::FromUTF8(val));

		row++;
	}

	varRowCount = std::max(1, row - 1);
}

void AutomationDialog::SyncBatchUIFromScript() {
	if (radioBatchMode)
		radioBatchMode->SetSelection(static_cast<int>(script.GetBatchMode()));

	auto* dp = XRCCTRL(*this, "dpBatchFolder", wxDirPickerCtrl);
	if (dp)
		dp->SetPath(wxString::FromUTF8(script.GetBatchFolder()));

	auto* txt = XRCCTRL(*this, "txtBatchExtension", wxTextCtrl);
	if (txt)
		txt->SetValue(wxString::FromUTF8(script.GetBatchExtension()));

	auto* chk = XRCCTRL(*this, "chkBatchSubdirs", wxCheckBox);
	if (chk)
		chk->SetValue(script.GetBatchSubdirectories());

	txt = XRCCTRL(*this, "txtBatchFileFilter", wxTextCtrl);
	if (txt)
		txt->SetValue(wxString::FromUTF8(script.GetBatchFileFilter()));

	chk = XRCCTRL(*this, "chkBatchFileFilterRegex", wxCheckBox);
	if (chk)
		chk->SetValue(script.GetBatchFileFilterRegex());

	txt = XRCCTRL(*this, "txtBatchSliderSetFilter", wxTextCtrl);
	if (txt)
		txt->SetValue(wxString::FromUTF8(script.GetBatchSliderSetFilter()));

	chk = XRCCTRL(*this, "chkBatchSliderSetFilterRegex", wxCheckBox);
	if (chk)
		chk->SetValue(script.GetBatchSliderSetFilterRegex());

	UpdateBatchPanelVisibility();
}

void AutomationDialog::SyncBatchScriptFromUI() {
	if (radioBatchMode)
		script.SetBatchMode(static_cast<AutomationBatchMode>(radioBatchMode->GetSelection()));

	auto* dp = XRCCTRL(*this, "dpBatchFolder", wxDirPickerCtrl);
	if (dp)
		script.SetBatchFolder(dp->GetPath().ToUTF8().data());

	auto* txt = XRCCTRL(*this, "txtBatchExtension", wxTextCtrl);
	if (txt)
		script.SetBatchExtension(txt->GetValue().ToUTF8().data());

	auto* chk = XRCCTRL(*this, "chkBatchSubdirs", wxCheckBox);
	if (chk)
		script.SetBatchSubdirectories(chk->GetValue());

	txt = XRCCTRL(*this, "txtBatchFileFilter", wxTextCtrl);
	if (txt)
		script.SetBatchFileFilter(txt->GetValue().ToUTF8().data());

	chk = XRCCTRL(*this, "chkBatchFileFilterRegex", wxCheckBox);
	if (chk)
		script.SetBatchFileFilterRegex(chk->GetValue());

	txt = XRCCTRL(*this, "txtBatchSliderSetFilter", wxTextCtrl);
	if (txt)
		script.SetBatchSliderSetFilter(txt->GetValue().ToUTF8().data());

	chk = XRCCTRL(*this, "chkBatchSliderSetFilterRegex", wxCheckBox);
	if (chk)
		script.SetBatchSliderSetFilterRegex(chk->GetValue());
}

void AutomationDialog::UpdateBatchPanelVisibility() {
	int mode = radioBatchMode ? radioBatchMode->GetSelection() : 0;

	bool folderMode = (mode == 1);
	bool sliderSetMode = (mode == 2);

	auto* panelFolder = XRCCTRL(*this, "panelBatchFolder", wxPanel);
	auto* panelSliderSets = XRCCTRL(*this, "panelBatchSliderSets", wxPanel);

	if (panelFolder)
		panelFolder->Show(folderMode);
	if (panelSliderSets)
		panelSliderSets->Show(sliderSetMode);

	auto* paneBatch = XRCCTRL(*this, "paneBatch", wxCollapsiblePane);
	if (paneBatch) {
		auto* pane = paneBatch->GetPane();
		pane->Layout();
		pane->GetSizer()->SetSizeHints(pane);
		GetSizer()->Layout();
	}
}

void AutomationDialog::OnBatchModeChanged(wxCommandEvent& WXUNUSED(event)) {
	UpdateBatchPanelVisibility();

	// Set batch-specific options on all steps
	for (auto& step : script.GetSteps())
		ApplyBatchModeDefaults(step);

	// Update currently displayed step
	if (selectedStep >= 0 && selectedStep < static_cast<int>(script.GetSteps().size())) {
		auto& step = script.GetSteps()[selectedStep];
		if (step.type == AutomationStepType::ExportFile) {
			step.exportFilePath.clear();
			auto* fp = XRCCTRL(*this, "fpExportFile", wxFilePickerCtrl);
			if (fp)
				fp->SetPath(wxEmptyString);
			auto* dp = XRCCTRL(*this, "dpExportFolder", wxDirPickerCtrl);
			if (dp)
				dp->SetPath(wxEmptyString);

			SetCheckboxValue("chkExportUseOriginalPath", step.exportUseOriginalPath);
			UpdateExportForBatchMode();
			UpdateExportFileBatchModeUI(step);
		}
		else if (step.type == AutomationStepType::SaveProject) {
			SetCheckboxValue("chkSaveUseOriginal", step.saveUseOriginal);
			SetCheckboxValue("chkSaveCopyRefFromProject", step.saveCopyRefFromProject);
			UpdateSaveProjectBatchModeUI(step);
		}
	}
}

std::vector<std::string> AutomationDialog::GatherBatchFiles() {
	std::vector<std::string> result;

	wxString folder = MakeAbsoluteToProject(wxString::FromUTF8(script.GetBatchFolder()));
	std::string ext = script.GetBatchExtension();
	if (folder.IsEmpty() || ext.empty())
		return result;

	// Ensure extension starts with *
	wxString wildcard = wxString::FromUTF8("*" + ext);

	wxArrayString wxFiles;
	if (script.GetBatchSubdirectories())
		wxDir::GetAllFiles(folder, &wxFiles, wildcard);
	else {
		wxDir dir(folder);
		if (dir.IsOpened()) {
			wxString f;
			if (dir.GetFirst(&f, wildcard, wxDIR_FILES)) {
				do {
					wxFiles.Add(folder + wxFileName::GetPathSeparator() + f);
				} while (dir.GetNext(&f));
			}
		}
	}

	// Apply file filter
	std::string filter = script.GetBatchFileFilter();
	bool useRegex = script.GetBatchFileFilterRegex();
	std::regex filterRegex;
	if (useRegex && !filter.empty()) {
		try {
			filterRegex = std::regex(filter, std::regex::icase);
		}
		catch (const std::regex_error&) {
			wxLogWarning("Automation: Invalid batch file filter regex '%s'.", filter);
			useRegex = false;
		}
	}

	for (const auto& filePath : wxFiles) {
		std::string path = filePath.ToUTF8().data();

		if (!filter.empty()) {
			wxFileName fn(filePath);
			std::string name = fn.GetFullName().ToUTF8().data();
			if (!MatchesFilter(name, filter, useRegex, filterRegex))
				continue;
		}

		result.push_back(path);
	}

	return result;
}

std::vector<std::pair<std::string, std::string>> AutomationDialog::GatherBatchSliderSets() {
	std::vector<std::pair<std::string, std::string>> result; // (fileName, setName) pairs

	std::string projPath = GetProjectPath();
	wxArrayString files;
	wxDir::GetAllFiles(wxString::FromUTF8(projPath) + "/SliderSets", &files, "*.osp");
	wxDir::GetAllFiles(wxString::FromUTF8(projPath) + "/SliderSets", &files, "*.xml");

	std::string filter = script.GetBatchSliderSetFilter();
	bool useRegex = script.GetBatchSliderSetFilterRegex();
	std::regex filterRegex;
	if (useRegex && !filter.empty()) {
		try {
			filterRegex = std::regex(filter, std::regex::icase);
		}
		catch (const std::regex_error&) {
			wxLogWarning("Automation: Invalid slider set filter regex '%s'.", filter);
			useRegex = false;
		}
	}

	for (const auto& filePath : files) {
		SliderSetFile ssf(filePath.ToUTF8().data());
		if (ssf.fail())
			continue;

		std::vector<std::string> setNames;
		ssf.GetSetNamesUnsorted(setNames);

		for (const auto& setName : setNames) {
			if (!MatchesFilter(setName, filter, useRegex, filterRegex))
				continue;

			result.push_back({filePath.ToUTF8().data(), setName});
		}
	}

	return result;
}

void AutomationDialog::ExecuteBatch(const std::vector<size_t>& stepIndices, const std::vector<std::string>& selectedFiles, const std::vector<std::pair<std::string, std::string>>& selectedSets) {
	auto batchMode = script.GetBatchMode();

	// Pre-set OptimizeForSSE if not already configured.
	// ValidateNIF would normally prompt via wxMessageBox parented to OutfitStudioFrame,
	// which appears behind this modal dialog and blocks. Default to true (optimize).
	Config.SetDefaultBoolValue("OptimizeForSSE", true);

	// Clear project before starting batch to prevent current content from leaking into first iteration
	ResetAndClearProject();

	StartProgress(_("Preparing..."));

	if (batchMode == AutomationBatchMode::FolderScan) {
		auto batchFiles = selectedFiles.empty() ? GatherBatchFiles() : selectedFiles;
		if (batchFiles.empty()) {
			wxMessageBox(_("No files found matching the batch folder scan criteria."), _("Automation"), wxICON_INFORMATION);
			return;
		}

		int totalItems = static_cast<int>(batchFiles.size());
		int processedCount = 0;
		int errorCount = 0;

		for (int itemIdx = 0; itemIdx < totalItems; itemIdx++) {
			const auto& filePath = batchFiles[itemIdx];
			wxFileName fn(wxString::FromUTF8(filePath));
			std::string baseName = fn.GetName().ToUTF8().data();

			int progress = itemIdx * 100 / totalItems;
			wxString msg = wxString::Format(_("Processing %d/%d: %s"), itemIdx + 1, totalItems, fn.GetFullName());
			UpdateProgress(progress, msg);

			// Set up batch-specific variables
			auto vars = CollectVariables();
			vars["BATCH_FILE"] = filePath;
			vars["BATCH_NAME"] = baseName;
			vars["BATCH_DIR"] = fn.GetPath().ToUTF8().data();
			vars["BATCH_FULLNAME"] = fn.GetFullName().ToUTF8().data();

			// Clear project for fresh start
			ResetAndClearProject();

			// Import the batch file
			wxString ext = fn.GetExt().Lower();
			int importErr = 0;
			if (ext == "nif")
				importErr = project->ImportNIF(filePath, false);
			else if (ext == "obj")
				importErr = project->ImportOBJ(filePath);
			else if (ext == "fbx")
#ifdef USE_FBXSDK
				importErr = project->ImportFBX(filePath);
#else
				wxLogError("Automation: FBX import is not available (FBX SDK not compiled in).");
#endif
			else {
				wxLogWarning("Automation: Batch - unsupported file type '%s', skipping.", ext);
				continue;
			}

			if (importErr) {
				wxLogError("Automation: Batch - failed to import '%s' (error %d), skipping.", filePath, importErr);
				errorCount++;
				continue;
			}

			// Build and execute script copy with substituted variables
			AutomationScript execScript;
			for (size_t idx : stepIndices)
				execScript.AddStep(script.GetSteps()[idx]);

			execScript.SubstitutePlaceholders(vars);

			// Process steps that have exportUseOriginalPath
			for (auto& step : execScript.GetSteps()) {
				if (step.type == AutomationStepType::ExportFile) {
					if (step.exportUseOriginalPath) {
						step.exportFilePath = filePath;
					}
					else if (!step.exportFilePath.empty()) {
						// In batch mode, exportFilePath is a folder - construct full path
						wxFileName exportFn;
						exportFn.SetPath(wxString::FromUTF8(step.exportFilePath));
						exportFn.SetFullName(fn.GetFullName());
						step.exportFilePath = exportFn.GetFullPath().ToUTF8().data();
					}
				}
			}

			bool stepFailed = false;
			for (size_t i = 0; i < execScript.GetSteps().size(); i++) {
				int err = ExecuteStep(execScript.GetSteps()[i]);
				if (err != 0) {
					wxLogError("Automation: Batch - step %zu failed on '%s'.", i + 1, filePath);
					stepFailed = true;
					break;
				}
			}

			if (stepFailed)
				errorCount++;

			processedCount++;

			// Clear project after processing each entry
			ResetAndClearProject();
		}

		EndProgress(_("Batch complete."));

		// Refresh UI
		outfitStudio->RefreshGUIFromProj();
		outfitStudio->CreateSetSliders();

		wxMessageBox(wxString::Format(_("Batch completed: %d/%d items processed successfully."),
									  processedCount - errorCount, processedCount),
					 _("Automation"), wxICON_INFORMATION);
	}
	else if (batchMode == AutomationBatchMode::SliderSets) {
		auto batchSets = selectedSets.empty() ? GatherBatchSliderSets() : selectedSets;
		if (batchSets.empty()) {
			wxMessageBox(_("No slider sets found matching the filter criteria."), _("Automation"), wxICON_INFORMATION);
			return;
		}

		int totalItems = static_cast<int>(batchSets.size());
		int processedCount = 0;
		int errorCount = 0;

		for (int itemIdx = 0; itemIdx < totalItems; itemIdx++) {
			const auto& [filePath, setName] = batchSets[itemIdx];
			wxFileName fn(wxString::FromUTF8(filePath));

			int progress = itemIdx * 100 / totalItems;
			wxString msg = wxString::Format(_("Processing %d/%d: %s"), itemIdx + 1, totalItems, wxString::FromUTF8(setName));
			UpdateProgress(progress, msg);

			// Set up batch-specific variables
			auto vars = CollectVariables();
			vars["BATCH_FILE"] = filePath;
			vars["BATCH_NAME"] = setName;
			vars["BATCH_SET"] = setName;
			vars["BATCH_DIR"] = fn.GetPath().ToUTF8().data();

			// Clear and load the project
			ResetAndClearProject();
			bool loaded = outfitStudio->LoadProject(filePath, setName, true);

			// LoadProject with clearProject=true recreates the project object
			project = outfitStudio->project;

			if (!loaded) {
				wxLogError("Automation: Batch - failed to load project '%s' from '%s', skipping.", setName, filePath);
				errorCount++;
				continue;
			}

			// Check if loaded project has a reference shape (before any modifications)
			auto* baseShape = project->GetBaseShape();
			bool loadedProjectHadRef = (baseShape != nullptr);

			// Build and execute script copy
			AutomationScript execScript;
			for (size_t idx : stepIndices)
				execScript.AddStep(script.GetSteps()[idx]);

			execScript.SubstitutePlaceholders(vars);

			for (auto& step : execScript.GetSteps()) {
				// Apply copy-ref-from-project: override saveAutoCopyRef based on loaded project
				if (step.type == AutomationStepType::SaveProject && step.saveCopyRefFromProject) {
					bool hadMatchingRef = loadedProjectHadRef;

					// If shape name filter(s) set, check if the reference matches any of them
					if (hadMatchingRef && !step.saveCopyRefShapeName.empty()) {
						std::string refName = baseShape->name.get();
						hadMatchingRef = false;
						for (const auto& name : SplitCommaSeparated(step.saveCopyRefShapeName)) {
							if (refName == name) {
								hadMatchingRef = true;
								break;
							}
						}
					}

					step.saveAutoCopyRef = hadMatchingRef;
				}

				if (step.type == AutomationStepType::SaveProject && step.saveUseOriginal) {
					// Use the original project's save settings
					step.saveSliderSetFile = filePath;
					step.saveName = setName;

					SliderSetFile ssf(filePath);
					if (!ssf.fail()) {
						SliderSet ss;
						if (ssf.GetSet(setName, ss) == 0) {
							step.saveOutputFileName = ss.GetOutputFile();
							step.saveOutputDataPath = ss.GetOutputPath();
							step.saveShapeDataFolder = ss.GetDefaultDataFolder();
							step.saveShapeDataFile = ss.GetInputFile();
							step.saveGenWeights = ss.GenWeights();
						}
					}

					// Apply replace from/to on all string fields
					if (!step.saveReplaceFrom.empty()) {
						auto replaceAll = [&](std::string& s) {
							size_t pos = 0;
							while ((pos = s.find(step.saveReplaceFrom, pos)) != std::string::npos) {
								s.replace(pos, step.saveReplaceFrom.size(), step.saveReplaceTo);
								pos += step.saveReplaceTo.size();
							}
						};
						replaceAll(step.saveName);
						replaceAll(step.saveOutputFileName);
						replaceAll(step.saveOutputDataPath);
						replaceAll(step.saveShapeDataFolder);
						replaceAll(step.saveShapeDataFile);

						// Replace only in the filename part of the slider set file path
						wxFileName ssfFn(wxString::FromUTF8(step.saveSliderSetFile));
						wxString ssfName = ssfFn.GetName();
						std::string ssfNameStr = ssfName.ToUTF8().data();
						replaceAll(ssfNameStr);
						ssfFn.SetName(wxString::FromUTF8(ssfNameStr));
						step.saveSliderSetFile = ssfFn.GetFullPath().ToUTF8().data();
					}

					// Apply suffix to display name, shape data folder, shape data file, and slider set file
					if (!step.saveSuffix.empty()) {
						step.saveName += step.saveSuffix;
						step.saveShapeDataFolder += step.saveSuffix;

						// Insert suffix before the file extension for slider set file
						wxFileName ssfFn(wxString::FromUTF8(step.saveSliderSetFile));
						ssfFn.SetName(ssfFn.GetName() + wxString::FromUTF8(step.saveSuffix));
						step.saveSliderSetFile = ssfFn.GetFullPath().ToUTF8().data();
					}
				}
				else if (step.type == AutomationStepType::ExportFile) {
					if (step.exportUseOriginalPath) {
						// Overwrite the original file loaded by the batch
                        step.exportFilePath = filePath;
					}
					else if (!step.exportFilePath.empty()) {
						// In batch mode, exportFilePath is a folder - construct full path
						wxFileName exportFn;
						exportFn.SetPath(wxString::FromUTF8(step.exportFilePath));
						exportFn.SetName(wxString::FromUTF8(setName));
						exportFn.SetExt("nif");
						step.exportFilePath = exportFn.GetFullPath().ToUTF8().data();
					}
				}
			}

			bool stepFailed = false;
			for (size_t i = 0; i < execScript.GetSteps().size(); i++) {
				int err = ExecuteStep(execScript.GetSteps()[i]);
				if (err != 0) {
					wxLogError("Automation: Batch - step %zu failed on '%s'.", i + 1, setName);
					stepFailed = true;
					break;
				}
			}

			if (stepFailed)
				errorCount++;

			processedCount++;

			// Clear project after processing each entry
			ResetAndClearProject();
		}

		EndProgress(_("Batch complete."));

		// Refresh UI
		outfitStudio->RefreshGUIFromProj();
		outfitStudio->CreateSetSliders();

		wxMessageBox(wxString::Format(_("Batch completed: %d/%d slider sets processed successfully."),
									  processedCount - errorCount, processedCount),
					 _("Automation"), wxICON_INFORMATION);
	}
}
