/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "AutomationDialog.h"

#include "OutfitProject.h"
#include "OutfitStudio.h"

#include "../files/MaskFile.h"

#include <NifFile.hpp>

#include <wx/wx.h>
#include <wx/dir.h>
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <wx/clntdata.h>
#include <wx/xrc/xmlres.h>

#include <tinyxml2.h>

#include <algorithm>
#include <regex>
#include <set>

#include "../components/SliderSet.h"

using namespace nifly;

extern ConfigurationManager Config;
extern ConfigurationManager OutfitStudioConfig;

namespace {
enum class ShaderPropertyValueKind {
	Scalar,
	Vector2,
	Color,
	Choice
};

struct ShaderPropertyChoiceDef {
	const char* label;
	const char* value;
};

struct ShaderPropertyDef {
	const char* name;
	const char* label;
	ShaderPropertyValueKind kind;
	float default1;
	float default2;
	float default3;
	float default4;
	const ShaderPropertyChoiceDef* choices;
	size_t choiceCount;
};

struct GeometryPropertyDef {
	const char* name;
	const char* label;
	bool defaultEnabled;
};

struct TexturePathDef {
	int index;
	const char* name;
	const char* label;
};

const ShaderPropertyChoiceDef ShaderTypeChoices[] = {
	{"Default (BSLighting)", "BSLighting:0"},
	{"Environment Map (BSLighting)", "BSLighting:1"},
	{"Glow Shader (BSLighting)", "BSLighting:2"},
	{"Heightmap (BSLighting)", "BSLighting:3"},
	{"Face Tint (BSLighting)", "BSLighting:4"},
	{"Skin Tint (BSLighting)", "BSLighting:5"},
	{"Hair Tint (BSLighting)", "BSLighting:6"},
	{"Parallax Occlusion Material (BSLighting)", "BSLighting:7"},
	{"World Multitexture (BSLighting)", "BSLighting:8"},
	{"World Map 1 (BSLighting)", "BSLighting:9"},
	{"Unknown 10 (BSLighting)", "BSLighting:10"},
	{"Multi Layer Parallax (BSLighting)", "BSLighting:11"},
	{"Unknown 12 (BSLighting)", "BSLighting:12"},
	{"World Map 2 (BSLighting)", "BSLighting:13"},
	{"Sparkle Snow (BSLighting)", "BSLighting:14"},
	{"World Map 3 (BSLighting)", "BSLighting:15"},
	{"Eye Environment Map (BSLighting)", "BSLighting:16"},
	{"Unknown 17 (BSLighting)", "BSLighting:17"},
	{"World Map 4 (BSLighting)", "BSLighting:18"},
	{"World LOD Multitexture (BSLighting)", "BSLighting:19"},
	{"Tall Grass (FO3/NV)", "PPLighting:0"},
	{"Default (FO3/NV)", "PPLighting:1"},
	{"Sky (FO3/NV)", "PPLighting:10"},
	{"Skin (FO3/NV)", "PPLighting:14"},
	{"Water (FO3/NV)", "PPLighting:17"},
	{"Lighting 30 (FO3/NV)", "PPLighting:29"},
	{"Tile (FO3/NV)", "PPLighting:32"},
	{"No Lighting (FO3/NV)", "PPLighting:33"}
};

const ShaderPropertyDef ShaderPropertyDefs[] = {
	{"ShaderType", "Shader Type", ShaderPropertyValueKind::Choice, 0.0f, 0.0f, 0.0f, 1.0f, ShaderTypeChoices, sizeof(ShaderTypeChoices) / sizeof(ShaderTypeChoices[0])},
	{"SpecularColor", "Specular Color", ShaderPropertyValueKind::Color, 1.0f, 1.0f, 1.0f, 1.0f, nullptr, 0},
	{"SpecularStrength", "Specular Strength", ShaderPropertyValueKind::Scalar, 1.0f, 0.0f, 0.0f, 1.0f, nullptr, 0},
	{"SpecularPower", "Specular Power", ShaderPropertyValueKind::Scalar, 1.0f, 0.0f, 0.0f, 1.0f, nullptr, 0},
	{"EmissiveColor", "Emissive Color", ShaderPropertyValueKind::Color, 0.0f, 0.0f, 0.0f, 1.0f, nullptr, 0},
	{"EmissiveMultiple", "Emissive Multiple", ShaderPropertyValueKind::Scalar, 1.0f, 0.0f, 0.0f, 1.0f, nullptr, 0},
	{"Alpha", "Alpha", ShaderPropertyValueKind::Scalar, 1.0f, 0.0f, 0.0f, 1.0f, nullptr, 0},
	{"EnvMapScale", "Env Map Scale", ShaderPropertyValueKind::Scalar, 1.0f, 0.0f, 0.0f, 1.0f, nullptr, 0},
	{"EyeCubemapScale", "Eye Cubemap Scale", ShaderPropertyValueKind::Scalar, 1.0f, 0.0f, 0.0f, 1.0f, nullptr, 0},
	{"UVOffset", "UV Offset", ShaderPropertyValueKind::Vector2, 0.0f, 0.0f, 0.0f, 1.0f, nullptr, 0},
	{"UVScale", "UV Scale", ShaderPropertyValueKind::Vector2, 1.0f, 1.0f, 0.0f, 1.0f, nullptr, 0},
	{"LightingEffect1", "Lighting Effect 1", ShaderPropertyValueKind::Scalar, 0.3f, 0.0f, 0.0f, 1.0f, nullptr, 0},
	{"LightingEffect2", "Lighting Effect 2", ShaderPropertyValueKind::Scalar, 2.0f, 0.0f, 0.0f, 1.0f, nullptr, 0},
	{"SkinTintColor", "Skin Tint Color", ShaderPropertyValueKind::Color, 1.0f, 1.0f, 1.0f, 1.0f, nullptr, 0},
	{"HairTintColor", "Hair Tint Color", ShaderPropertyValueKind::Color, 1.0f, 1.0f, 1.0f, 1.0f, nullptr, 0},
	{"RefractionStrength", "Refraction Strength", ShaderPropertyValueKind::Scalar, 0.0f, 0.0f, 0.0f, 1.0f, nullptr, 0}
};

const GeometryPropertyDef GeometryPropertyDefs[] = {
	{"Skinned", "Skinned", true},
	{"Dynamic", "Dynamic", true},
	{"FullPrecision", "Full Precision", true},
	{"SubIndex", "Sub Index", true}
};

const TexturePathDef TexturePathDefs[] = {
	{0, "Diffuse", "0: Diffuse"},
	{1, "Normal", "1: Normal"},
	{2, "Glow/Skin", "2: Glow/Skin"},
	{3, "Parallax", "3: Parallax"},
	{4, "Environment", "4: Environment"},
	{5, "Env Mask", "5: Env Mask"},
	{6, "6", "6"},
	{7, "Specular", "7: Specular"},
	{8, "8", "8"},
	{9, "9", "9"},
	{10, "10", "10"},
	{11, "11", "11"},
	{12, "12", "12"}
};

const ShaderPropertyDef* FindShaderPropertyDef(const std::string& name) {
	for (const auto& def : ShaderPropertyDefs) {
		if (name == def.name)
			return &def;
	}
	return nullptr;
}

AutomationStep::ShaderProperty MakeDefaultShaderProperty(const ShaderPropertyDef& def) {
	AutomationStep::ShaderProperty prop;
	prop.name = def.name;
	prop.value1 = def.default1;
	prop.value2 = def.default2;
	prop.value3 = def.default3;
	prop.value4 = def.default4;
	if (def.kind == ShaderPropertyValueKind::Choice && def.choiceCount > 0)
		prop.stringValue = def.choices[0].value;
	return prop;
}

const GeometryPropertyDef* FindGeometryPropertyDef(const std::string& name) {
	for (const auto& def : GeometryPropertyDefs) {
		if (name == def.name)
			return &def;
	}
	return nullptr;
}

AutomationStep::GeometryProperty MakeDefaultGeometryProperty(const GeometryPropertyDef& def) {
	AutomationStep::GeometryProperty prop;
	prop.name = def.name;
	prop.enabled = def.defaultEnabled;
	return prop;
}

const TexturePathDef* FindTexturePathDefByIndex(int index) {
	for (const auto& def : TexturePathDefs) {
		if (def.index == index)
			return &def;
	}
	return nullptr;
}

const TexturePathDef* FindTexturePathDefByName(const std::string& name) {
	std::string nameLower = ToLower(name);
	for (const auto& def : TexturePathDefs) {
		if (nameLower == ToLower(def.name) || nameLower == ToLower(def.label))
			return &def;
	}
	return nullptr;
}

AutomationStep::TexturePath MakeDefaultTexturePath(const TexturePathDef& def) {
	AutomationStep::TexturePath path;
	path.index = def.index;
	path.name = def.name;
	return path;
}

int ColorByte(float value) {
	value = std::max(0.0f, std::min(1.0f, value));
	return static_cast<int>(value * 255.0f + 0.5f);
}

std::string GetChoiceClientValue(wxChoice* choice) {
	if (!choice)
		return "";

	int sel = choice->GetSelection();
	if (sel == wxNOT_FOUND)
		return "";

	auto* data = dynamic_cast<wxStringClientData*>(choice->GetClientObject(sel));
	if (!data)
		return "";

	return data->GetData().ToUTF8().data();
}

int FindChoiceByClientValue(wxChoice* choice, const std::string& value) {
	if (!choice)
		return wxNOT_FOUND;

	for (unsigned int i = 0; i < choice->GetCount(); i++) {
		auto* data = dynamic_cast<wxStringClientData*>(choice->GetClientObject(i));
		if (data && value == data->GetData().ToUTF8().data())
			return static_cast<int>(i);
	}

	return wxNOT_FOUND;
}

}

int AutomationDialog::TexturePathIndexForName(const std::string& name) {
	const TexturePathDef* def = FindTexturePathDefByName(name);
	return def ? def->index : -1;
}

int AutomationDialog::ResolveTexturePathIndex(const AutomationStep::TexturePath& path) {
	if (path.index >= 0)
		return path.index;

	return TexturePathIndexForName(path.name);
}

std::string AutomationDialog::TexturePathNameForIndex(int index) {
	const TexturePathDef* def = FindTexturePathDefByIndex(index);
	return def ? def->name : std::to_string(index);
}

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
	EVT_CHECKBOX(XRCID("chkSetRefUnset"), AutomationDialog::OnSetRefUnsetChanged)
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
	btnClose = dynamic_cast<wxButton*>(FindWindow(wxID_CLOSE));

	listSteps->InsertColumn(0, _("Active"), wxLIST_FORMAT_CENTER, 65);
	listSteps->InsertColumn(1, _("Type"), wxLIST_FORMAT_LEFT, 165);
	listSteps->InsertColumn(2, _("Target"), wxLIST_FORMAT_LEFT, 100);
	listSteps->InsertColumn(3, _("Note"), wxLIST_FORMAT_LEFT, 250);

	listSteps->Bind(wxEVT_CONTEXT_MENU, &AutomationDialog::OnStepListContextMenu, this);
	listSteps->Bind(wxEVT_KEY_DOWN, &AutomationDialog::OnStepListKeyDown, this);

	Bind(wxEVT_CHAR_HOOK, &AutomationDialog::OnCharHook, this);
	Bind(wxEVT_CLOSE_WINDOW, &AutomationDialog::OnWindowClose, this);

	// Placeholder label shown when step list is empty
	lblStepsPlaceholder = new wxStaticText(listSteps, wxID_ANY, _("Right-click to add steps..."), wxPoint(0, 40), wxDefaultSize, wxALIGN_CENTER_HORIZONTAL | wxST_NO_AUTORESIZE);
	lblStepsPlaceholder->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
	lblStepsPlaceholder->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
	lblStepsPlaceholder->Bind(wxEVT_CONTEXT_MENU, &AutomationDialog::OnStepListContextMenu, this);
	// Keep placeholder centered when list is resized
	listSteps->Bind(wxEVT_SIZE, [this](wxSizeEvent& evt) {
		if (lblStepsPlaceholder)
			lblStepsPlaceholder->SetSize(evt.GetSize().GetWidth(), lblStepsPlaceholder->GetSize().GetHeight());
		evt.Skip();
	});

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

	// Bind "+" buttons for appending shapes/sliders to comma-separated fields
	auto* btnAddTargetMesh = XRCCTRL(*this, "btnAddTargetMesh", wxButton);
	if (btnAddTargetMesh)
		btnAddTargetMesh->Bind(wxEVT_BUTTON, &AutomationDialog::OnAddShapeToField, this);

	auto* btnAddDeleteSlider = XRCCTRL(*this, "btnAddDeleteSlider", wxButton);
	if (btnAddDeleteSlider)
		btnAddDeleteSlider->Bind(wxEVT_BUTTON, &AutomationDialog::OnAddSliderToField, this);

	auto* btnAddSetSlider = XRCCTRL(*this, "btnAddSetSlider", wxButton);
	if (btnAddSetSlider)
		btnAddSetSlider->Bind(wxEVT_BUTTON, &AutomationDialog::OnAddSliderToField, this);

	auto* btnAddConformSlider = XRCCTRL(*this, "btnAddConformSlider", wxButton);
	if (btnAddConformSlider)
		btnAddConformSlider->Bind(wxEVT_BUTTON, &AutomationDialog::OnAddSliderToField, this);

	auto* btnAddSliderProp = XRCCTRL(*this, "btnAddSliderProp", wxButton);
	if (btnAddSliderProp)
		btnAddSliderProp->Bind(wxEVT_BUTTON, &AutomationDialog::OnAddSliderToField, this);

	PopulateShaderPropertyChoice();
	auto* btnAddShaderProp = XRCCTRL(*this, "btnAddShaderProp", wxButton);
	if (btnAddShaderProp)
		btnAddShaderProp->Bind(wxEVT_BUTTON, &AutomationDialog::OnAddShaderProperty, this);

	PopulateGeometryPropertyChoice();
	auto* btnAddGeometryProp = XRCCTRL(*this, "btnAddGeometryProp", wxButton);
	if (btnAddGeometryProp)
		btnAddGeometryProp->Bind(wxEVT_BUTTON, &AutomationDialog::OnAddGeometryProperty, this);

	PopulateTexturePathChoice();
	auto* btnAddTexturePath = XRCCTRL(*this, "btnAddTexturePath", wxButton);
	if (btnAddTexturePath)
		btnAddTexturePath->Bind(wxEVT_BUTTON, &AutomationDialog::OnAddTexturePath, this);

	auto* btnAddFixClipSlider = XRCCTRL(*this, "btnAddFixClipSlider", wxButton);
	if (btnAddFixClipSlider)
		btnAddFixClipSlider->Bind(wxEVT_BUTTON, &AutomationDialog::OnAddSliderToField, this);

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
	else if (step.type == AutomationStepType::ExportFile) {
		step.exportUseOriginalPath = IsBatchMode(AutomationBatchMode::FolderScan);
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

void AutomationDialog::UpdateExportFileBatchModeUI(const AutomationStep& WXUNUSED(step)) {
	bool folderBatch = IsBatchMode(AutomationBatchMode::FolderScan);
	bool effectiveUseOriginalPath = folderBatch;

	auto* chkUseOrig = XRCCTRL(*this, "chkExportUseOriginalPath", wxCheckBox);
	if (chkUseOrig) {
		if (folderBatch) {
			chkUseOrig->SetValue(true);
			chkUseOrig->Enable(false);
		}
		else {
			chkUseOrig->SetValue(false);
			chkUseOrig->Enable(false);
		}
	}

	UpdateExportFieldsEnabled(!effectiveUseOriginalPath);
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

void AutomationDialog::PopulateShaderPropertyChoice() {
	auto* choice = XRCCTRL(*this, "choiceShaderPropAdd", wxChoice);
	if (choice) {
		choice->Clear();
		for (const auto& def : ShaderPropertyDefs)
			choice->Append(wxString::FromUTF8(def.label), new wxStringClientData(wxString::FromUTF8(def.name)));

		if (choice->GetCount() > 0)
			choice->SetSelection(0);
	}

	auto* rowsWindow = XRCCTRL(*this, "panelShaderPropRows", wxScrolledWindow);
	if (rowsWindow)
		rowsWindow->SetScrollRate(0, 8);
}

void AutomationDialog::ClearShaderPropertyRows() {
	auto* rowsWindow = XRCCTRL(*this, "panelShaderPropRows", wxScrolledWindow);
	if (rowsWindow) {
		if (auto* rowsSizer = rowsWindow->GetSizer())
			rowsSizer->Clear(true);
		rowsWindow->FitInside();
		rowsWindow->Layout();
	}

	shaderPropertyRows.clear();
}

void AutomationDialog::AddShaderPropertyRow(const AutomationStep::ShaderProperty& prop) {
	const ShaderPropertyDef* def = FindShaderPropertyDef(prop.name);
	if (!def)
		return;

	auto* rowsWindow = XRCCTRL(*this, "panelShaderPropRows", wxScrolledWindow);
	if (!rowsWindow)
		return;

	wxSizer* rowsSizer = rowsWindow->GetSizer();
	if (!rowsSizer) {
		rowsSizer = new wxBoxSizer(wxVERTICAL);
		rowsWindow->SetSizer(rowsSizer);
	}

	auto* rowPanel = new wxPanel(rowsWindow, wxID_ANY);
	auto* rowSizer = new wxBoxSizer(wxHORIZONTAL);

	ShaderPropertyRowControls row;
	row.panel = rowPanel;
	row.propertyName = prop.name;

	auto* label = new wxStaticText(rowPanel, wxID_ANY, wxString::FromUTF8(def->label), wxDefaultPosition, wxSize(145, -1));
	rowSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

	switch (def->kind) {
		case ShaderPropertyValueKind::Choice: {
			auto* choice = new wxChoice(rowPanel, wxID_ANY);
			for (size_t i = 0; i < def->choiceCount; i++)
				choice->Append(wxString::FromUTF8(def->choices[i].label), new wxStringClientData(wxString::FromUTF8(def->choices[i].value)));

			int selection = FindChoiceByClientValue(choice, prop.stringValue);
			choice->SetSelection(selection != wxNOT_FOUND ? selection : 0);
			row.choice = choice;
			rowSizer->Add(choice, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
			break;
		}
		case ShaderPropertyValueKind::Color: {
			auto* color = new wxColourPickerCtrl(rowPanel, wxID_ANY, wxColour(ColorByte(prop.value1), ColorByte(prop.value2), ColorByte(prop.value3), ColorByte(prop.value4)));
			row.color = color;
			rowSizer->Add(color, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
			break;
		}
		case ShaderPropertyValueKind::Vector2: {
			auto* value1 = new wxTextCtrl(rowPanel, wxID_ANY, wxString::Format("%.5g", prop.value1), wxDefaultPosition, wxSize(80, -1));
			auto* value2 = new wxTextCtrl(rowPanel, wxID_ANY, wxString::Format("%.5g", prop.value2), wxDefaultPosition, wxSize(80, -1));
			row.value1 = value1;
			row.value2 = value2;
			rowSizer->Add(value1, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
			rowSizer->Add(value2, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
			rowSizer->AddStretchSpacer(1);
			break;
		}
		case ShaderPropertyValueKind::Scalar:
		default: {
			auto* value = new wxTextCtrl(rowPanel, wxID_ANY, wxString::Format("%.5g", prop.value1), wxDefaultPosition, wxSize(90, -1));
			row.value1 = value;
			rowSizer->Add(value, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
			rowSizer->AddStretchSpacer(1);
			break;
		}
	}

	auto* btnRemove = new wxButton(rowPanel, wxID_ANY, "X", wxDefaultPosition, wxSize(28, -1));
	btnRemove->SetToolTip(_("Remove this shader property"));
	btnRemove->Bind(wxEVT_BUTTON, [this, rowPanel](wxCommandEvent&) { RemoveShaderPropertyRow(rowPanel); });
	rowSizer->Add(btnRemove, 0, wxALIGN_CENTER_VERTICAL);

	rowPanel->SetSizer(rowSizer);
	rowsSizer->Add(rowPanel, 0, wxEXPAND | wxBOTTOM, 4);
	shaderPropertyRows.push_back(row);

	rowsWindow->FitInside();
	rowsWindow->Layout();
	auto* page = XRCCTRL(*this, "pageSetShaderProperties", wxPanel);
	if (page)
		page->Layout();
}

void AutomationDialog::RemoveShaderPropertyRow(wxWindow* rowPanel) {
	for (auto it = shaderPropertyRows.begin(); it != shaderPropertyRows.end(); ++it) {
		if (it->panel == rowPanel) {
			auto* rowsWindow = XRCCTRL(*this, "panelShaderPropRows", wxScrolledWindow);
			if (rowsWindow && rowsWindow->GetSizer())
				rowsWindow->GetSizer()->Detach(it->panel);
			if (it->panel)
				it->panel->Destroy();
			shaderPropertyRows.erase(it);
			break;
		}
	}

	auto* rowsWindow = XRCCTRL(*this, "panelShaderPropRows", wxScrolledWindow);
	if (rowsWindow) {
		rowsWindow->FitInside();
		rowsWindow->Layout();
	}
}

void AutomationDialog::RebuildShaderPropertyRows(const std::vector<AutomationStep::ShaderProperty>& properties) {
	ClearShaderPropertyRows();
	for (const auto& prop : properties)
		AddShaderPropertyRow(prop);
}

std::vector<AutomationStep::ShaderProperty> AutomationDialog::ReadShaderPropertyRows() const {
	std::vector<AutomationStep::ShaderProperty> properties;
	for (const auto& row : shaderPropertyRows) {
		const ShaderPropertyDef* def = FindShaderPropertyDef(row.propertyName);
		if (!def)
			continue;

		AutomationStep::ShaderProperty prop = MakeDefaultShaderProperty(*def);
		prop.name = row.propertyName;

		switch (def->kind) {
			case ShaderPropertyValueKind::Choice:
				prop.stringValue = GetChoiceClientValue(row.choice);
				break;
			case ShaderPropertyValueKind::Color: {
				wxColour color = row.color ? row.color->GetColour() : wxColour(ColorByte(prop.value1), ColorByte(prop.value2), ColorByte(prop.value3), ColorByte(prop.value4));
				prop.value1 = color.Red() / 255.0f;
				prop.value2 = color.Green() / 255.0f;
				prop.value3 = color.Blue() / 255.0f;
				prop.value4 = color.Alpha() / 255.0f;
				break;
			}
			case ShaderPropertyValueKind::Vector2:
				prop.value1 = row.value1 ? static_cast<float>(atof(row.value1->GetValue().c_str())) : prop.value1;
				prop.value2 = row.value2 ? static_cast<float>(atof(row.value2->GetValue().c_str())) : prop.value2;
				break;
			case ShaderPropertyValueKind::Scalar:
			default:
				prop.value1 = row.value1 ? static_cast<float>(atof(row.value1->GetValue().c_str())) : prop.value1;
				break;
		}

		properties.push_back(prop);
	}

	return properties;
}

void AutomationDialog::PopulateGeometryPropertyChoice() {
	auto* choice = XRCCTRL(*this, "choiceGeometryPropAdd", wxChoice);
	if (choice) {
		choice->Clear();
		for (const auto& def : GeometryPropertyDefs)
			choice->Append(wxString::FromUTF8(def.label), new wxStringClientData(wxString::FromUTF8(def.name)));

		if (choice->GetCount() > 0)
			choice->SetSelection(0);
	}

	auto* rowsWindow = XRCCTRL(*this, "panelGeometryPropRows", wxScrolledWindow);
	if (rowsWindow)
		rowsWindow->SetScrollRate(0, 8);
}

void AutomationDialog::ClearGeometryPropertyRows() {
	auto* rowsWindow = XRCCTRL(*this, "panelGeometryPropRows", wxScrolledWindow);
	if (rowsWindow) {
		if (auto* rowsSizer = rowsWindow->GetSizer())
			rowsSizer->Clear(true);
		rowsWindow->FitInside();
		rowsWindow->Layout();
	}

	geometryPropertyRows.clear();
}

void AutomationDialog::AddGeometryPropertyRow(const AutomationStep::GeometryProperty& prop) {
	const GeometryPropertyDef* def = FindGeometryPropertyDef(prop.name);
	if (!def)
		return;

	auto* rowsWindow = XRCCTRL(*this, "panelGeometryPropRows", wxScrolledWindow);
	if (!rowsWindow)
		return;

	wxSizer* rowsSizer = rowsWindow->GetSizer();
	if (!rowsSizer) {
		rowsSizer = new wxBoxSizer(wxVERTICAL);
		rowsWindow->SetSizer(rowsSizer);
	}

	auto* rowPanel = new wxPanel(rowsWindow, wxID_ANY);
	auto* rowSizer = new wxBoxSizer(wxHORIZONTAL);

	GeometryPropertyRowControls row;
	row.panel = rowPanel;
	row.propertyName = prop.name;

	auto* label = new wxStaticText(rowPanel, wxID_ANY, wxString::FromUTF8(def->label), wxDefaultPosition, wxSize(145, -1));
	rowSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

	auto* choice = new wxChoice(rowPanel, wxID_ANY);
	choice->Append(_("Enabled"));
	choice->Append(_("Disabled"));
	choice->SetSelection(prop.enabled ? 0 : 1);
	row.value = choice;
	rowSizer->Add(choice, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	rowSizer->AddStretchSpacer(1);

	auto* btnRemove = new wxButton(rowPanel, wxID_ANY, "X", wxDefaultPosition, wxSize(28, -1));
	btnRemove->SetToolTip(_("Remove this geometry property"));
	btnRemove->Bind(wxEVT_BUTTON, [this, rowPanel](wxCommandEvent&) { RemoveGeometryPropertyRow(rowPanel); });
	rowSizer->Add(btnRemove, 0, wxALIGN_CENTER_VERTICAL);

	rowPanel->SetSizer(rowSizer);
	rowsSizer->Add(rowPanel, 0, wxEXPAND | wxBOTTOM, 4);
	geometryPropertyRows.push_back(row);

	rowsWindow->FitInside();
	rowsWindow->Layout();
	auto* page = XRCCTRL(*this, "pageSetGeometryProperties", wxPanel);
	if (page)
		page->Layout();
}

void AutomationDialog::RemoveGeometryPropertyRow(wxWindow* rowPanel) {
	for (auto it = geometryPropertyRows.begin(); it != geometryPropertyRows.end(); ++it) {
		if (it->panel == rowPanel) {
			auto* rowsWindow = XRCCTRL(*this, "panelGeometryPropRows", wxScrolledWindow);
			if (rowsWindow && rowsWindow->GetSizer())
				rowsWindow->GetSizer()->Detach(it->panel);
			if (it->panel)
				it->panel->Destroy();
			geometryPropertyRows.erase(it);
			break;
		}
	}

	auto* rowsWindow = XRCCTRL(*this, "panelGeometryPropRows", wxScrolledWindow);
	if (rowsWindow) {
		rowsWindow->FitInside();
		rowsWindow->Layout();
	}
}

void AutomationDialog::RebuildGeometryPropertyRows(const std::vector<AutomationStep::GeometryProperty>& properties) {
	ClearGeometryPropertyRows();
	for (const auto& prop : properties)
		AddGeometryPropertyRow(prop);
}

std::vector<AutomationStep::GeometryProperty> AutomationDialog::ReadGeometryPropertyRows() const {
	std::vector<AutomationStep::GeometryProperty> properties;
	for (const auto& row : geometryPropertyRows) {
		const GeometryPropertyDef* def = FindGeometryPropertyDef(row.propertyName);
		if (!def)
			continue;

		AutomationStep::GeometryProperty prop = MakeDefaultGeometryProperty(*def);
		prop.name = row.propertyName;
		prop.enabled = !row.value || row.value->GetSelection() != 1;
		properties.push_back(prop);
	}

	return properties;
}

void AutomationDialog::PopulateTexturePathChoice() {
	auto* choice = XRCCTRL(*this, "choiceTexturePathAdd", wxChoice);
	if (choice) {
		choice->Clear();
		for (const auto& def : TexturePathDefs)
			choice->Append(wxString::FromUTF8(def.label), new wxStringClientData(wxString::Format("%d", def.index)));

		if (choice->GetCount() > 0)
			choice->SetSelection(0);
	}

	auto* rowsWindow = XRCCTRL(*this, "panelTexturePathRows", wxScrolledWindow);
	if (rowsWindow)
		rowsWindow->SetScrollRate(0, 8);
}

void AutomationDialog::ClearTexturePathRows() {
	auto* rowsWindow = XRCCTRL(*this, "panelTexturePathRows", wxScrolledWindow);
	if (rowsWindow) {
		if (auto* rowsSizer = rowsWindow->GetSizer())
			rowsSizer->Clear(true);
		rowsWindow->FitInside();
		rowsWindow->Layout();
	}

	texturePathRows.clear();
}

void AutomationDialog::AddTexturePathRow(const AutomationStep::TexturePath& path) {
	int index = ResolveTexturePathIndex(path);
	const TexturePathDef* def = FindTexturePathDefByIndex(index);
	if (!def)
		return;

	auto* rowsWindow = XRCCTRL(*this, "panelTexturePathRows", wxScrolledWindow);
	if (!rowsWindow)
		return;

	wxSizer* rowsSizer = rowsWindow->GetSizer();
	if (!rowsSizer) {
		rowsSizer = new wxBoxSizer(wxVERTICAL);
		rowsWindow->SetSizer(rowsSizer);
	}

	auto* rowPanel = new wxPanel(rowsWindow, wxID_ANY);
	auto* rowSizer = new wxBoxSizer(wxHORIZONTAL);

	TexturePathRowControls row;
	row.panel = rowPanel;
	row.index = index;
	row.name = path.name.empty() ? def->name : path.name;

	auto* label = new wxStaticText(rowPanel, wxID_ANY, wxString::FromUTF8(def->label), wxDefaultPosition, wxSize(145, -1));
	rowSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

	auto* value = new wxTextCtrl(rowPanel, wxID_ANY, wxString::FromUTF8(path.path));
	row.path = value;
	rowSizer->Add(value, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

	auto* btnRemove = new wxButton(rowPanel, wxID_ANY, "X", wxDefaultPosition, wxSize(28, -1));
	btnRemove->SetToolTip(_("Remove this texture path"));
	btnRemove->Bind(wxEVT_BUTTON, [this, rowPanel](wxCommandEvent&) { RemoveTexturePathRow(rowPanel); });
	rowSizer->Add(btnRemove, 0, wxALIGN_CENTER_VERTICAL);

	rowPanel->SetSizer(rowSizer);
	rowsSizer->Add(rowPanel, 0, wxEXPAND | wxBOTTOM, 4);
	texturePathRows.push_back(row);

	rowsWindow->FitInside();
	rowsWindow->Layout();
	auto* page = XRCCTRL(*this, "pageSetTexturePaths", wxPanel);
	if (page)
		page->Layout();
}

void AutomationDialog::RemoveTexturePathRow(wxWindow* rowPanel) {
	for (auto it = texturePathRows.begin(); it != texturePathRows.end(); ++it) {
		if (it->panel == rowPanel) {
			auto* rowsWindow = XRCCTRL(*this, "panelTexturePathRows", wxScrolledWindow);
			if (rowsWindow && rowsWindow->GetSizer())
				rowsWindow->GetSizer()->Detach(it->panel);
			if (it->panel)
				it->panel->Destroy();
			texturePathRows.erase(it);
			break;
		}
	}

	auto* rowsWindow = XRCCTRL(*this, "panelTexturePathRows", wxScrolledWindow);
	if (rowsWindow) {
		rowsWindow->FitInside();
		rowsWindow->Layout();
	}
}

void AutomationDialog::RebuildTexturePathRows(const std::vector<AutomationStep::TexturePath>& paths) {
	ClearTexturePathRows();
	for (const auto& path : paths)
		AddTexturePathRow(path);
}

std::vector<AutomationStep::TexturePath> AutomationDialog::ReadTexturePathRows() const {
	std::vector<AutomationStep::TexturePath> paths;
	for (const auto& row : texturePathRows) {
		AutomationStep::TexturePath path;
		path.index = row.index;
		path.name = row.name.empty() ? TexturePathNameForIndex(row.index) : row.name;
		path.path = row.path ? row.path->GetValue().ToUTF8().data() : std::string();
		paths.push_back(path);
	}

	return paths;
}

// Progress methods

void AutomationDialog::StartProgress(const wxString& msg) {
	if (progressBar)
		return;

	cancelRequested = false;
	SetExecutionUIState(true);

	wxRect rect;
	statusBar->GetFieldRect(1, rect);
	progressBar = new wxGauge(statusBar, wxID_ANY, 10000, rect.GetPosition(), rect.GetSize());

	statusBar->SetStatusText(msg.IsEmpty() ? _("Starting...") : msg);

	// Redirect log output to the output pane
	if (!headlessMode && paneOutput && txtOutput) {
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

void AutomationDialog::OnCharHook(wxKeyEvent& event) {
	if (event.GetKeyCode() == WXK_ESCAPE && progressBar) {
		cancelRequested = true;
		statusBar->SetStatusText(_("Cancelling..."));
		return;
	}
	event.Skip();
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

	SetExecutionUIState(false);
}

void AutomationDialog::SetExecutionUIState(bool running) {
	isExecuting = running;

	wxWindowList children = GetChildren();
	for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
		wxWindow* child = *it;
		if (!child)
			continue;

		if (child == statusBar || child == btnClose || child == paneOutput)
			continue;

		child->Enable(!running);
	}

	if (btnClose)
		btnClose->SetLabel(running ? _("Cancel") : _("Close"));
}

void AutomationDialog::OnWindowClose(wxCloseEvent& event) {
	if (isExecuting) {
		cancelRequested = true;
		if (statusBar)
			statusBar->SetStatusText(_("Cancelling..."));
		event.Veto();
		return;
	}

	event.Skip();
}

void AutomationDialog::PopulateStepList() {
	listSteps->DeleteAllItems();
	auto& steps = script.GetSteps();

	if (lblStepsPlaceholder)
		lblStepsPlaceholder->Show(steps.empty());

	for (size_t i = 0; i < steps.size(); i++) {
		long idx = listSteps->InsertItem(i, steps[i].active ? wxString(L"\u2713") : wxString(""));
		listSteps->SetItem(idx, 1, wxString::FromUTF8(AutomationStepTypeToString(steps[i].type)));

		std::string targetStr = JoinStrings(steps[i].targetMeshes, ", ");
		if (targetStr.empty())
			targetStr = "(all)";
		listSteps->SetItem(idx, 2, wxString::FromUTF8(targetStr));

		listSteps->SetItem(idx, 3, wxString::FromUTF8(steps[i].note));
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

	listSteps->SetItem(index, 3, wxString::FromUTF8(step.note));
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
		ID_CTX_DUPLICATE_STEP,
		ID_CTX_REMOVE_STEP,
		ID_CTX_MOVE_UP,
		ID_CTX_MOVE_DOWN,
		ID_CTX_EXECUTE_SELECTED
	};

	wxMenu menu;
	menu.Append(ID_CTX_ADD_STEP, _("Add Step"));

	bool hasSelection = (selectedStep >= 0);
	menu.Append(ID_CTX_DUPLICATE_STEP, _("Duplicate Step"))->Enable(hasSelection);
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
		case ID_CTX_DUPLICATE_STEP: OnDuplicateStep(evt); break;
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
		case AutomationStepType::SetShaderProperties: {
			RebuildShaderPropertyRows(step.shaderProperties);
			break;
		}
		case AutomationStepType::SetGeometryProperties: {
			RebuildGeometryPropertyRows(step.geometryProperties);
			break;
		}
		case AutomationStepType::SetTexturePaths: {
			RebuildTexturePathRows(step.texturePaths);
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
			SetVectorValue("txtDeleteSliderName", step.deleteSliderNames);
			SetCheckboxValue("chkDeleteSliderRegex", step.deleteSliderRegex);
			break;
		}
		case AutomationStepType::SetReferenceShape: {
			SetTextValue("txtSetRefShapeName", step.setRefShapeName);
			SetCheckboxValue("chkSetRefUnset", step.setRefUnset);
			UpdateSetRefFieldsEnabled(!step.setRefUnset);
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

		case AutomationStepType::ClearMask:
			// No parameters to set
			break;

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

		case AutomationStepType::FixClipping: {
			auto* choice = XRCCTRL(*this, "choiceFixClipMode", wxChoice);
			if (choice)
				choice->SetSelection(step.fixClipMode);
			auto* txt = XRCCTRL(*this, "txtFixClipStrength", wxTextCtrl);
			if (txt)
				txt->SetValue(wxString::Format("%d", static_cast<int>(step.fixClipStrength * 100)));
			SetVectorValue("txtFixClipSliderNames", step.fixClipSliderNames);
			break;
		}

		case AutomationStepType::FixBadBones:
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
		case AutomationStepType::SetShaderProperties: {
			step.shaderProperties = ReadShaderPropertyRows();
			break;
		}
		case AutomationStepType::SetGeometryProperties: {
			step.geometryProperties = ReadGeometryPropertyRows();
			break;
		}
		case AutomationStepType::SetTexturePaths: {
			step.texturePaths = ReadTexturePathRows();
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
			step.deleteSliderNames = GetVectorValue("txtDeleteSliderName");
			step.deleteSliderRegex = GetCheckboxValue("chkDeleteSliderRegex");
			break;
		}
		case AutomationStepType::SetReferenceShape: {
			step.setRefShapeName = GetTextValue("txtSetRefShapeName");
			step.setRefUnset = GetCheckboxValue("chkSetRefUnset");
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
			step.exportUseOriginalPath = IsBatchMode(AutomationBatchMode::FolderScan) && GetCheckboxValue("chkExportUseOriginalPath");
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

		case AutomationStepType::ClearMask:
			// No parameters to read
			break;

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

		case AutomationStepType::FixClipping: {
			auto* choice = XRCCTRL(*this, "choiceFixClipMode", wxChoice);
			if (choice)
				step.fixClipMode = choice->GetSelection();
			step.fixClipStrength = GetFloatValue("txtFixClipStrength") / 100.0f;
			step.fixClipSliderNames = GetVectorValue("txtFixClipSliderNames");
			break;
		}

		case AutomationStepType::FixBadBones:
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

std::vector<NiShape*> AutomationDialog::ResolveTargetShapes(const AutomationStep& step, bool includeBaseShapeOnEmpty) {
	if (step.targetMeshes.empty()) {
		auto allShapes = project->GetWorkNif()->GetShapes();
		std::vector<NiShape*> result;
		auto* baseShape = project->GetBaseShape();
		for (auto* shape : allShapes) {
			if (includeBaseShapeOnEmpty || shape != baseShape)
				result.push_back(shape);
		}
		return result;
	}

	std::vector<NiShape*> result;
	auto addUniqueShape = [&result](NiShape* shape) {
		if (shape && std::find(result.begin(), result.end(), shape) == result.end())
			result.push_back(shape);
	};

	if (step.targetRegex) {
		auto allShapes = project->GetWorkNif()->GetShapes();
		for (const auto& pattern : step.targetMeshes) {
			try {
				std::regex re(pattern, std::regex::icase);
				bool matched = false;
				for (auto* shape : allShapes) {
					if (std::regex_search(shape->name.get(), re)) {
						addUniqueShape(shape);
						matched = true;
					}
				}
				if (!matched)
					wxLogWarning("Automation: Target shape regex '%s' matched no shapes.", pattern);
			}
			catch (const std::regex_error& e) {
				wxLogWarning("Automation: Invalid target shape regex '%s' (%s); skipping pattern.", pattern, e.what());
			}
		}
	}
	else {
		for (const auto& name : step.targetMeshes) {
			NiShape* shape = FindShapeByName(name);
			if (shape)
				addUniqueShape(shape);
			else
				wxLogWarning("Automation: Target shape '%s' not found.", name);
		}
	}

	return result;
}

std::string AutomationDialog::GetAutomationsFolder() {
	return GetProjectPath() + "/Automations";
}

void AutomationDialog::CollectScripts(const wxString& baseFolder, const wxString& currentFolder, std::vector<std::pair<wxString, wxString>>& entries) {
	wxDir dir(currentFolder);
	if (!dir.IsOpened())
		return;

	// Collect .xml files in this folder
	wxString filename;
	if (dir.GetFirst(&filename, "*.xml", wxDIR_FILES)) {
		do {
			wxString fullPath = currentFolder + "/" + filename;

			// Only include files whose root element is <AutomationScript>
			tinyxml2::XMLDocument doc;
			if (doc.LoadFile(fullPath.ToUTF8().data()) != tinyxml2::XML_SUCCESS)
				continue;
			if (!doc.FirstChildElement("AutomationScript"))
				continue;

			wxFileName fn(filename);
			wxString relativePath;
			if (currentFolder == baseFolder) {
				relativePath = fn.GetName();
			}
			else {
				wxString subPath;
				wxFileName::SplitPath(currentFolder, nullptr, nullptr, &subPath);
				// Get the relative folder path from baseFolder
				wxString relFolder = currentFolder.Mid(baseFolder.length() + 1);
				relativePath = relFolder + "/" + fn.GetName();
			}
			entries.push_back({relativePath, relativePath});
		} while (dir.GetNext(&filename));
	}

	// Recurse into subdirectories
	wxString dirName;
	if (dir.GetFirst(&dirName, wxEmptyString, wxDIR_DIRS)) {
		do {
			CollectScripts(baseFolder, currentFolder + "/" + dirName, entries);
		} while (dir.GetNext(&dirName));
	}
}

void AutomationDialog::PopulateAutomationList() {
	if (!cmbAutomation)
		return;

	wxString currentText = cmbAutomation->GetValue();
	cmbAutomation->Clear();
	cmbAutomation->Append(_("<New>"));

	wxString folder = wxString::FromUTF8(GetAutomationsFolder());
	if (!wxDir::Exists(folder)) {
		if (!currentText.IsEmpty())
			cmbAutomation->SetValue(currentText);
		return;
	}

	// Collect all scripts recursively: {relativePath, displayName}
	std::vector<std::pair<wxString, wxString>> entries;
	CollectScripts(folder, folder, entries);

	// Separate root-level scripts from subfolder scripts
	std::vector<wxString> rootScripts;
	std::map<wxString, std::vector<wxString>> folderScripts;

	for (auto& [path, display] : entries) {
		int sep = path.Find('/');
		if (sep == wxNOT_FOUND) {
			rootScripts.push_back(path);
		}
		else {
			wxString folderName = path.Left(sep);
			folderScripts[folderName].push_back(path);
		}
	}

	// Sort root scripts (case-insensitive)
	std::sort(rootScripts.begin(), rootScripts.end(),
		[](const wxString& a, const wxString& b) { return a.CmpNoCase(b) < 0; });

	// Add root-level scripts
	for (auto& name : rootScripts)
		cmbAutomation->Append(name);

	// Add folder groups with separator headers
	for (auto& [folderName, scripts] : folderScripts) {
		std::sort(scripts.begin(), scripts.end(),
			[](const wxString& a, const wxString& b) { return a.CmpNoCase(b) < 0; });

		// Separator header
		wxString separator = wxS("\u2500\u2500\u2500 ") + folderName + wxS(" \u2500\u2500\u2500");
		cmbAutomation->Append(separator);

		for (auto& path : scripts)
			cmbAutomation->Append(path);
	}

	if (!currentText.IsEmpty())
		cmbAutomation->SetValue(currentText);
}

bool AutomationDialog::IsSeparatorItem(const wxString& text) {
	return text.StartsWith(wxS("\u2500"));
}

wxString AutomationDialog::SanitizePath(const wxString& name) {
	wxString result;
	bool lastWasSep = false;
	for (auto ch : name) {
		if (ch == '/') {
			if (!result.IsEmpty() && !lastWasSep)
				result += '/';
			lastWasSep = true;
		}
		else if (ch == '<' || ch == '>' || ch == ':' || ch == '"' || ch == '\\' || ch == '|' || ch == '?' || ch == '*') {
			result += '_';
			lastWasSep = false;
		}
		else {
			result += ch;
			lastWasSep = false;
		}
	}
	// Trim trailing separator
	if (result.EndsWith("/"))
		result.RemoveLast();
	return result;
}

int AutomationDialog::RunHeadless(const wxString& scriptName, const wxArrayString& batchInputs) {
	headlessMode = true;
	lastRunErrors = 0;

	if (scriptName.IsEmpty()) {
		wxLogError("Automation: No script name provided.");
		return 1;
	}

	// Verify the script file exists before calling LoadAutomation (which silently
	// returns on missing files).
	wxString sanitized = SanitizePath(scriptName);
	wxString filePath = wxString::FromUTF8(GetAutomationsFolder()) + "/" + sanitized + ".xml";
	if (!wxFileExists(filePath)) {
		wxLogError("Automation: Script '%s' not found at '%s'.", scriptName, filePath);
		return 2;
	}

	LoadAutomation(scriptName);

	std::vector<size_t> indices;
	for (size_t i = 0; i < script.GetSteps().size(); i++) {
		if (script.GetSteps()[i].active)
			indices.push_back(i);
	}
	if (indices.empty()) {
		wxLogError("Automation: Script '%s' has no active steps.", scriptName);
		return 3;
	}

	auto mode = script.GetBatchMode();

	if (mode == AutomationBatchMode::None) {
		if (!batchInputs.IsEmpty())
			wxLogWarning("Automation: Script is not a batch script; ignoring %u positional argument(s).",
						 static_cast<unsigned>(batchInputs.GetCount()));
		ExecuteSteps(indices);
		return lastRunErrors == 0 ? 0 : 10;
	}

	if (mode == AutomationBatchMode::FolderScan) {
		std::vector<std::string> selectedFiles;

		if (batchInputs.IsEmpty()) {
			// Fall back to the script's configured batch folder/filter and let the
			// user confirm via the existing checkable list dialog.
			auto files = GatherBatchFiles();
			if (files.empty()) {
				wxLogError("Automation: No files found matching the batch folder scan criteria.");
				return 4;
			}

			wxArrayString displayItems;
			for (const auto& fp : files)
				displayItems.Add(wxString::FromUTF8(fp));

			std::vector<size_t> checkedIndices;
			if (!ShowCheckableListDialog(_("Batch Files"),
										 wxString::Format(_("Select files to process (%d found):"), static_cast<int>(files.size())),
										 displayItems, checkedIndices)) {
				wxLogMessage("Automation: Batch file selection cancelled.");
				return 5;
			}
			if (checkedIndices.empty()) {
				wxLogError("Automation: No files selected.");
				return 6;
			}

			for (size_t idx : checkedIndices)
				selectedFiles.push_back(files[idx]);
		}
		else {
			// Expand directories using the script's batch settings.
			std::string ext = script.GetBatchExtension();
			wxString wildcard = wxString::FromUTF8("*" + ext);
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

			for (const auto& input : batchInputs) {
				if (wxDirExists(input)) {
					wxArrayString found;
					if (script.GetBatchSubdirectories())
						wxDir::GetAllFiles(input, &found, wildcard);
					else {
						wxDir d(input);
						if (d.IsOpened()) {
							wxString f;
							if (d.GetFirst(&f, wildcard, wxDIR_FILES)) {
								do {
									found.Add(input + wxFileName::GetPathSeparator() + f);
								} while (d.GetNext(&f));
							}
						}
					}
					for (const auto& fp : found) {
						wxFileName fn(fp);
						std::string name = fn.GetFullName().ToUTF8().data();
						if (!MatchesFilter(name, filter, useRegex, filterRegex))
							continue;
						selectedFiles.push_back(std::string(fp.ToUTF8().data()));
					}
				}
				else if (wxFileExists(input)) {
					selectedFiles.push_back(std::string(input.ToUTF8().data()));
				}
				else {
					wxLogWarning("Automation: Input '%s' is not an existing file or directory; skipping.", input);
				}
			}

			if (selectedFiles.empty()) {
				wxLogError("Automation: No files resolved from positional arguments.");
				return 7;
			}
		}

		ExecuteBatch(indices, selectedFiles);
		return lastRunErrors == 0 ? 0 : 10;
	}

	if (mode == AutomationBatchMode::SliderSets) {
		std::vector<std::pair<std::string, std::string>> selectedSets;

		if (batchInputs.IsEmpty()) {
			auto sets = GatherBatchSliderSets();
			if (sets.empty()) {
				wxLogError("Automation: No slider sets found matching the filter criteria.");
				return 4;
			}

			wxArrayString displayItems;
			for (const auto& [fp, setName] : sets)
				displayItems.Add(wxString::FromUTF8(setName));

			std::vector<size_t> checkedIndices;
			if (!ShowCheckableListDialog(_("Batch Slider Sets"),
										 wxString::Format(_("Select slider sets to process (%d found):"), static_cast<int>(sets.size())),
										 displayItems, checkedIndices)) {
				wxLogMessage("Automation: Slider set selection cancelled.");
				return 5;
			}
			if (checkedIndices.empty()) {
				wxLogError("Automation: No slider sets selected.");
				return 6;
			}

			for (size_t idx : checkedIndices)
				selectedSets.push_back(sets[idx]);
		}
		else {
			// Resolve set project names against <ProjectPath>/SliderSets/*.{osp,xml}.
			std::string projPath = GetProjectPath();
			wxArrayString files;
			wxDir::GetAllFiles(wxString::FromUTF8(projPath) + "/SliderSets", &files, "*.osp");
			wxDir::GetAllFiles(wxString::FromUTF8(projPath) + "/SliderSets", &files, "*.xml");

			std::set<std::string> wanted;
			for (const auto& s : batchInputs)
				wanted.insert(std::string(s.ToUTF8().data()));

			std::set<std::string> resolved;
			for (const auto& fp : files) {
				SliderSetFile ssf(fp.ToUTF8().data());
				if (ssf.fail())
					continue;
				std::vector<std::string> setNames;
				ssf.GetSetNamesUnsorted(setNames);
				for (const auto& sn : setNames) {
					if (wanted.count(sn)) {
						selectedSets.push_back({std::string(fp.ToUTF8().data()), sn});
						resolved.insert(sn);
					}
				}
			}

			for (const auto& w : wanted) {
				if (!resolved.count(w))
					wxLogWarning("Automation: Slider set '%s' not found in project SliderSets.", w);
			}

			if (selectedSets.empty()) {
				wxLogError("Automation: No slider sets resolved from positional arguments.");
				return 7;
			}
		}

		ExecuteBatch(indices, {}, selectedSets);
		return lastRunErrors == 0 ? 0 : 10;
	}

	wxLogError("Automation: Unknown batch mode.");
	return 8;
}

void AutomationDialog::LoadAutomation(const wxString& name) {
	if (name.IsEmpty())
		return;

	wxString sanitized = SanitizePath(name);
	wxString filePath = wxString::FromUTF8(GetAutomationsFolder()) + "/" + sanitized + ".xml";

	if (!wxFileExists(filePath))
		return;

	int err = script.Load(filePath.ToUTF8().data());
	if (err) {
		if (headlessMode)
			wxLogError("Automation: Failed to load automation script (error %d).", err);
		else
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
		btnSaveScript->Enable(hasSteps && !isExecuting);
	if (btnExecuteAll)
		btnExecuteAll->Enable(hasSteps && !isExecuting);
}

void AutomationDialog::OnAutomationSelected(wxCommandEvent& WXUNUSED(event)) {
	wxString name = cmbAutomation->GetValue();

	// Ignore separator header items
	if (IsSeparatorItem(name)) {
		cmbAutomation->SetValue(wxEmptyString);
		return;
	}

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

	wxString sanitized = SanitizePath(name);
	wxString folder = wxString::FromUTF8(GetAutomationsFolder());

	// Create folder (including subdirectories) if missing
	wxFileName fnPath(folder + "/" + sanitized + ".xml");
	if (!wxDir::Exists(fnPath.GetPath()))
		wxFileName::Mkdir(fnPath.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	wxString filePath = fnPath.GetFullPath();
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

	wxString sanitized = SanitizePath(name);
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

void AutomationDialog::OnDuplicateStep(wxCommandEvent& WXUNUSED(event)) {
	if (selectedStep < 0 || selectedStep >= static_cast<int>(script.GetSteps().size()))
		return;

	UpdateStepFromUI();

	AutomationStep copy = script.GetSteps()[selectedStep];
	int newIndex = selectedStep + 1;
	script.InsertStep(newIndex, copy);

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
	if (isExecuting)
		return;

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
	if (isExecuting)
		return;

	if (selectedStep < 0) {
		wxMessageBox(_("No step selected."), _("Automation"), wxICON_INFORMATION);
		return;
	}

	UpdateStepFromUI();
	ExecuteSteps({static_cast<size_t>(selectedStep)});
}

void AutomationDialog::OnClose(wxCommandEvent& WXUNUSED(event)) {
	if (isExecuting) {
		cancelRequested = true;
		if (statusBar)
			statusBar->SetStatusText(_("Cancelling..."));
		return;
	}

	if (selectedStep >= 0)
		UpdateStepFromUI();
	EndModal(wxID_CLOSE);
}

void AutomationDialog::ResetAndClearProject() {
	project->SetBaseShape(nullptr, false);
	project->GetWorkAnim()->Clear();
	project->GetWorkNif()->Clear();
	outfitStudio->ResetProject();
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

void AutomationDialog::OnSetRefUnsetChanged(wxCommandEvent& WXUNUSED(event)) {
	auto* chk = XRCCTRL(*this, "chkSetRefUnset", wxCheckBox);
	if (chk)
		UpdateSetRefFieldsEnabled(!chk->GetValue());
}

void AutomationDialog::UpdateSetRefFieldsEnabled(bool enabled) {
	auto* txt = XRCCTRL(*this, "txtSetRefShapeName", wxTextCtrl);
	if (txt)
		txt->Enable(enabled);
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

void AutomationDialog::OnAddShaderProperty(wxCommandEvent& WXUNUSED(event)) {
	auto* choice = XRCCTRL(*this, "choiceShaderPropAdd", wxChoice);
	std::string propertyName = GetChoiceClientValue(choice);
	if (propertyName.empty())
		return;

	for (const auto& row : shaderPropertyRows) {
		if (row.propertyName == propertyName)
			return;
	}

	const ShaderPropertyDef* def = FindShaderPropertyDef(propertyName);
	if (!def)
		return;

	AddShaderPropertyRow(MakeDefaultShaderProperty(*def));
}

void AutomationDialog::OnAddGeometryProperty(wxCommandEvent& WXUNUSED(event)) {
	auto* choice = XRCCTRL(*this, "choiceGeometryPropAdd", wxChoice);
	std::string propertyName = GetChoiceClientValue(choice);
	if (propertyName.empty())
		return;

	for (const auto& row : geometryPropertyRows) {
		if (row.propertyName == propertyName)
			return;
	}

	const GeometryPropertyDef* def = FindGeometryPropertyDef(propertyName);
	if (!def)
		return;

	AddGeometryPropertyRow(MakeDefaultGeometryProperty(*def));
}

void AutomationDialog::OnAddTexturePath(wxCommandEvent& WXUNUSED(event)) {
	auto* choice = XRCCTRL(*this, "choiceTexturePathAdd", wxChoice);
	std::string indexText = GetChoiceClientValue(choice);
	if (indexText.empty())
		return;

	int index = -1;
	try {
		index = std::stoi(indexText);
	}
	catch (...) {
		return;
	}

	for (const auto& row : texturePathRows) {
		if (row.index == index)
			return;
	}

	const TexturePathDef* def = FindTexturePathDefByIndex(index);
	if (!def)
		return;

	AddTexturePathRow(MakeDefaultTexturePath(*def));
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
	bool folderBatch = radioBatchMode && radioBatchMode->GetSelection() == static_cast<int>(AutomationBatchMode::FolderScan);

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
		chk->Show(folderBatch);
		if (!folderBatch)
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

	auto* paneVariables = XRCCTRL(*this, "paneVariables", wxCollapsiblePane);
	wxFlexGridSizer* gridSizer = nullptr;
	if (paneVariables) {
		wxWindow* paneWin = paneVariables->GetPane();
		wxSizer* boxSizer = paneWin ? paneWin->GetSizer() : nullptr;
		if (boxSizer && boxSizer->GetItemCount() >= 2)
			gridSizer = dynamic_cast<wxFlexGridSizer*>(boxSizer->GetItem(static_cast<size_t>(1))->GetSizer());
	}

	for (int i = 10; i >= 2; i--) {
		wxString keyName = wxString::Format("txtVarKey%d", i);
		wxString valName = wxString::Format("txtVarVal%d", i);
		auto* keyCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(keyName));
		auto* valCtrl = dynamic_cast<wxTextCtrl*>(FindWindow(valName));
		if (keyCtrl) {
			if (gridSizer)
				gridSizer->Detach(keyCtrl);
			keyCtrl->Destroy();
		}
		if (valCtrl) {
			if (gridSizer)
				gridSizer->Detach(valCtrl);
			valCtrl->Destroy();
		}
	}
	varRowCount = 1;

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

void AutomationDialog::AppendFromList(const char* textCtrlName, const wxArrayString& items, const wxString& title) {
	if (items.IsEmpty())
		return;

	auto* txt = XRCCTRL(*this, textCtrlName, wxTextCtrl);
	if (!txt)
		return;

	// Parse existing entries to exclude from the list
	std::vector<std::string> existing = SplitCommaSeparated(std::string(txt->GetValue().ToUTF8().data()));
	std::set<std::string> existingSet(existing.begin(), existing.end());

	wxArrayString filtered;
	for (const auto& item : items) {
		if (existingSet.find(std::string(item.ToUTF8().data())) == existingSet.end())
			filtered.Add(item);
	}

	if (filtered.IsEmpty())
		return;

	wxMultiChoiceDialog dlg(this, _("Select items to add:"), title, filtered);
	if (dlg.ShowModal() != wxID_OK)
		return;

	wxArrayInt selections = dlg.GetSelections();
	if (selections.IsEmpty())
		return;

	wxString current = txt->GetValue().Trim().Trim(false);
	for (int sel : selections) {
		wxString item = filtered[sel];
		if (!current.IsEmpty())
			current += ", ";
		current += item;
	}
	txt->SetValue(current);
}

void AutomationDialog::OnAddShapeToField(wxCommandEvent& WXUNUSED(event)) {
	wxArrayString items;
	auto* workNif = project->GetWorkNif();
	if (workNif) {
		for (auto* shape : workNif->GetShapes())
			items.Add(wxString::FromUTF8(shape->name.get()));
	}

	AppendFromList("txtTargetMeshes", items, _("Add Shapes"));
}

void AutomationDialog::OnAddSliderToField(wxCommandEvent& event) {
	// Determine which text control to append to based on which button was clicked
	wxWindow* btn = dynamic_cast<wxWindow*>(event.GetEventObject());
	const char* textCtrlName = nullptr;

	if (btn) {
		wxString name = btn->GetName();
		if (name == "btnAddDeleteSlider")
			textCtrlName = "txtDeleteSliderName";
		else if (name == "btnAddSetSlider")
			textCtrlName = "txtSetSliderNames";
		else if (name == "btnAddConformSlider")
			textCtrlName = "txtConformSliderNames";
		else if (name == "btnAddSliderProp")
			textCtrlName = "txtSliderPropNames";
		else if (name == "btnAddFixClipSlider")
			textCtrlName = "txtFixClipSliderNames";
	}

	if (!textCtrlName)
		return;

	wxArrayString items;
	std::vector<std::string> sliderList;
	project->GetSliderList(sliderList);
	for (const auto& s : sliderList)
		items.Add(wxString::FromUTF8(s));

	AppendFromList(textCtrlName, items, _("Add Sliders"));
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
	lastRunErrors = 0;

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
			EndProgress(_("No files found."));
			if (!headlessMode)
				wxMessageBox(_("No files found matching the batch folder scan criteria."), _("Automation"), wxICON_INFORMATION);
			else
				wxLogError("Automation: No files found matching the batch folder scan criteria.");
			lastRunErrors++;
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

			if (cancelRequested) {
				wxLogMessage("Automation: Batch cancelled by user.");
				break;
			}

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
				const auto& batchStep = execScript.GetSteps()[i];
				int err = ExecuteStep(batchStep);
				if (err != 0) {
					wxLogError("Automation: Batch - step %zu failed on '%s'.", i + 1, filePath);
					stepFailed = true;
					break;
				}

				outfitStudio->RefreshGUIFromProj();

				if (StepChangesSliderSet(batchStep.type))
					outfitStudio->CreateSetSliders();

				outfitStudio->ApplySliders();
			}

			if (stepFailed)
				errorCount++;

			processedCount++;

			// Clear project after processing each entry
			ResetAndClearProject();
		}

		EndProgress(cancelRequested ? _("Batch cancelled.") : _("Batch complete."));

		// Refresh UI
		outfitStudio->RefreshGUIFromProj();
		outfitStudio->CreateSetSliders();

		lastRunErrors += errorCount;
		if (cancelRequested)
			lastRunErrors++;

		if (!headlessMode) {
			wxMessageBox(wxString::Format(cancelRequested ? _("Batch cancelled: %d/%d items processed before cancellation.")
														  : _("Batch completed: %d/%d items processed successfully."),
										  processedCount - errorCount, processedCount),
						 _("Automation"), wxICON_INFORMATION);
		}
	}
	else if (batchMode == AutomationBatchMode::SliderSets) {
		auto batchSets = selectedSets.empty() ? GatherBatchSliderSets() : selectedSets;
		if (batchSets.empty()) {
			EndProgress(_("No slider sets found."));
			if (!headlessMode)
				wxMessageBox(_("No slider sets found matching the filter criteria."), _("Automation"), wxICON_INFORMATION);
			else
				wxLogError("Automation: No slider sets found matching the filter criteria.");
			lastRunErrors++;
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

			if (cancelRequested) {
				wxLogMessage("Automation: Batch cancelled by user.");
				break;
			}

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

					// Apply suffix to display name, shape data folder, and slider set file
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
						wxLogWarning("Automation: ExportFile - original-path export is only supported for folder scan batches; using configured export folder.");
					}
					if (!step.exportFilePath.empty()) {
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
				const auto& batchStep = execScript.GetSteps()[i];
				int err = ExecuteStep(batchStep);
				if (err != 0) {
					wxLogError("Automation: Batch - step %zu failed on '%s'.", i + 1, setName);
					stepFailed = true;
					break;
				}

				outfitStudio->RefreshGUIFromProj();

				if (StepChangesSliderSet(batchStep.type))
					outfitStudio->CreateSetSliders();

				outfitStudio->ApplySliders();
			}

			if (stepFailed)
				errorCount++;

			processedCount++;

			// Clear project after processing each entry
			ResetAndClearProject();
		}

		EndProgress(cancelRequested ? _("Batch cancelled.") : _("Batch complete."));

		// Refresh UI
		outfitStudio->RefreshGUIFromProj();
		outfitStudio->CreateSetSliders();

		lastRunErrors += errorCount;
		if (cancelRequested)
			lastRunErrors++;

		if (!headlessMode) {
			wxMessageBox(wxString::Format(cancelRequested ? _("Batch cancelled: %d/%d slider sets processed before cancellation.")
														  : _("Batch completed: %d/%d slider sets processed successfully."),
										  processedCount - errorCount, processedCount),
						 _("Automation"), wxICON_INFORMATION);
		}
	}
}
