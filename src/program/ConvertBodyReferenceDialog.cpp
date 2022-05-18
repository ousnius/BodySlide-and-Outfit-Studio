/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "ConvertBodyReferenceDialog.h"

#include <NifUtil.hpp>

#include "../utils/ConfigDialogUtil.h"
#include "../utils/ConfigurationManager.h"

#include <regex>

#include "OutfitStudio.h"

using namespace std;
class RefTemplate;
extern ConfigurationManager Config;

wxBEGIN_EVENT_TABLE(ConvertBodyReferenceDialog, wxWizard)

wxEND_EVENT_TABLE()

const char* CONVERT_SLIDER_PREFIX = "Convert";
const char* SLIDER_SET_PREFIX = "Sliders";


ConvertBodyReferenceDialog::ConvertBodyReferenceDialog(OutfitStudioFrame* outfitStudio,
													   OutfitProject* project,
													   ConfigurationManager& config,
													   const std::vector<RefTemplate>& refTemplates)
	: outfitStudio(outfitStudio)
	, project(project)
	, config(config)
	, refTemplates(refTemplates)
	, pg1(nullptr)
	, pg2(nullptr) {
	wxXmlResource* xrc = wxXmlResource::Get();
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/ConvertBodyReference.xrc");
	xrc->LoadObject(this, outfitStudio, "wizConvertBodyRef", "wxWizard");

	pg1 = (wxWizardPage*)XRCCTRL(*this, "wizpgConvertBodyRef1", wxWizardPageSimple);
	pg2 = (wxWizardPage*)XRCCTRL(*this, "wizpgConvertBodyRef2", wxWizardPageSimple);

	npConvRefChoice = XRCCTRL((*this), "npConvRefChoice", wxChoice);
	npConvRefChoice->Append("None");

	chkConformSliders = XRCCTRL((*this), "chkConformSliders", wxCheckBox);
	chkSkipConformPopup = XRCCTRL((*this), "chkSkipConformPopup", wxCheckBox);
	chkCopyBoneWeights = XRCCTRL((*this), "chkCopyBoneWeights", wxCheckBox);
	chkSkipCopyBonesPopup = XRCCTRL((*this), "chkSkipCopyBonesPopup", wxCheckBox);

	std::vector<RefTemplate> converters = refTemplates;
	std::vector<RefTemplate> bodies = refTemplates;

	auto sortTemplates = [&](const RefTemplate& first, const RefTemplate& second, vector<string> prioritizeNames, bool ascendingPriority) {
		const bool firstHasPriorityName = std::any_of(prioritizeNames.begin(), prioritizeNames.end(), [first](const string& name) {
			return strstr(first.GetName().c_str(), name.c_str());
		});
		const bool secondHasPriorityName = std::any_of(prioritizeNames.begin(), prioritizeNames.end(), [second](const string& name) {
			return strstr(second.GetName().c_str(), name.c_str());
		});
		if (firstHasPriorityName == secondHasPriorityName)
			return first.GetName().compare(second.GetName()) <= 0;
		return static_cast<bool>((!firstHasPriorityName && secondHasPriorityName) ^ ascendingPriority);
	};


	std::sort(converters.begin(), converters.end(), [&](const RefTemplate& first, const RefTemplate& second) {
		return sortTemplates(first, second, {CONVERT_SLIDER_PREFIX}, true);
	});
	std::sort(bodies.begin(), bodies.end(), [&](const RefTemplate& first, const RefTemplate& second) {
		return sortTemplates(first, second, {CONVERT_SLIDER_PREFIX, SLIDER_SET_PREFIX}, false);
	});

	ConfigDialogUtil::LoadDialogChoices(config, (*this), "ConvertBodyReference", "npConvRefChoice", converters);
	ConfigDialogUtil::LoadDialogChoices(config, (*this), "ConvertBodyReference", "npNewRefChoice", bodies);
	ConfigDialogUtil::LoadDialogText(config, (*this), "ConvertBodyReference", "npRemoveText");
	ConfigDialogUtil::LoadDialogText(config, (*this), "ConvertBodyReference", "npAppendText");
	ConfigDialogUtil::LoadDialogText(config, (*this), "ConvertBodyReference", "npDeleteShapesText");
	ConfigDialogUtil::LoadDialogText(config, (*this), "ConvertBodyReference", "npAddBonesText");
	ConfigDialogUtil::LoadDialogCheckBox(config, (*this), "ConvertBodyReference", "chkConvertMergeSliders");
	ConfigDialogUtil::LoadDialogCheckBox(config, (*this), "ConvertBodyReference", "chkConvertMergeZaps");
	ConfigDialogUtil::LoadDialogCheckBox(config, (*this), "ConvertBodyReference", "chkConformSliders");
	ConfigDialogUtil::LoadDialogCheckBox(config, (*this), "ConvertBodyReference", "chkSkipConformPopup");
	ConfigDialogUtil::LoadDialogCheckBox(config, (*this), "ConvertBodyReference", "chkCopyBoneWeights");
	ConfigDialogUtil::LoadDialogCheckBox(config, (*this), "ConvertBodyReference", "chkSkipCopyBonesPopup");
	ConfigDialogUtil::LoadDialogCheckBox(config, (*this), "ConvertBodyReference", "chkDeleteReferenceOnComplete");

	if (!chkConformSliders->IsChecked())
		chkSkipConformPopup->Disable();

	if (!chkCopyBoneWeights->IsChecked())
		chkSkipCopyBonesPopup->Disable();

	chkConformSliders->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent& event) {
		bool checked = event.IsChecked();
		chkSkipConformPopup->Enable(checked);
	});

	chkCopyBoneWeights->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent& event) {
		bool checked = event.IsChecked();
		chkSkipCopyBonesPopup->Enable(checked);
	});

	SetDoubleBuffered(true);
	CenterOnParent();
}

ConvertBodyReferenceDialog::~ConvertBodyReferenceDialog() {
	wxXmlResource::Get()->Unload(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/ConvertBodyReferenceDialog.xrc");
}

bool ConvertBodyReferenceDialog::Load() {
	FitToPage(pg1);
	return RunWizard(pg1);
}

void ConvertBodyReferenceDialog::ConvertBodyReference() const {
	outfitStudio->StartProgress(_("Starting conversion..."));

	bool mergeSliders = ConfigDialogUtil::SetBoolFromDialogCheckbox(config, (*this), "ConvertBodyReference", "chkConvertMergeSliders");
	bool mergeZaps = ConfigDialogUtil::SetBoolFromDialogCheckbox(config, (*this), "ConvertBodyReference", "chkConvertMergeZaps");
	bool conformSliders = ConfigDialogUtil::SetBoolFromDialogCheckbox(config, (*this), "ConvertBodyReference", "chkConformSliders");
	bool skipConformPopup = ConfigDialogUtil::SetBoolFromDialogCheckbox(config, (*this), "ConvertBodyReference", "chkSkipConformPopup");
	bool copyBoneWeights = ConfigDialogUtil::SetBoolFromDialogCheckbox(config, (*this), "ConvertBodyReference", "chkCopyBoneWeights");
	bool skipCopyBonesPopup = ConfigDialogUtil::SetBoolFromDialogCheckbox(config, (*this), "ConvertBodyReference", "chkSkipCopyBonesPopup");
	bool deleteReferenceOnCompleted = ConfigDialogUtil::SetBoolFromDialogCheckbox(config, (*this), "ConvertBodyReference", "chkDeleteReferenceOnComplete");
	auto conversionRefTemplate = ConfigDialogUtil::SetStringFromDialogChoice(config, (*this), "ConvertBodyReference", "npConvRefChoice");
	auto newRefTemplate = ConfigDialogUtil::SetStringFromDialogChoice(config, (*this), "ConvertBodyReference", "npNewRefChoice");
	auto removeFromProjectText = ConfigDialogUtil::SetStringFromDialogTextControl(config, (*this), "ConvertBodyReference", "npRemoveText");
	auto appendToProjectText = ConfigDialogUtil::SetStringFromDialogTextControl(config, (*this), "ConvertBodyReference", "npAppendText");
	auto deleteShapesText = ConfigDialogUtil::SetStringFromDialogTextControl(config, (*this), "ConvertBodyReference", "npDeleteShapesText");
	auto addBonesText = ConfigDialogUtil::SetStringFromDialogTextControl(config, (*this), "ConvertBodyReference", "npAddBonesText");

	Config.SaveConfig(Config["AppDir"] + "/Config.xml");

	outfitStudio->UpdateProgress(1, _("Updating Project Output Settings"));

	if (!removeFromProjectText.IsEmpty()) {
		wxStringTokenizer tkz(removeFromProjectText, wxT(","));
		bool modifiedName = false;
		while (tkz.HasMoreTokens()) {
			wxString token = tkz.GetNextToken();
			if (!modifiedName && !appendToProjectText.IsEmpty() && project->mFileName.Contains(token)) {
				project->mFileName.Replace(token, appendToProjectText);
				modifiedName = true;
			}
			else {
				project->mFileName.Replace(token, "");
			}
			project->mOutfitName.Replace(token, "");
			project->mDataDir.Replace(token, "");
			project->mBaseFile.Replace(token, "");
		}
	}
	if (!appendToProjectText.IsEmpty()) {
		if (project->mOutfitName[0] != ' ')
			project->mOutfitName.Prepend(' ');
		if (project->mDataDir[0] != ' ')
			project->mDataDir.Prepend(' ');
		if (project->mBaseFile[0] != ' ')
			project->mBaseFile.Prepend(' ');
		project->mOutfitName.Prepend(appendToProjectText);
		project->mDataDir.Prepend(appendToProjectText);
		project->mBaseFile.Prepend(appendToProjectText);

		project->outfitName = project->mOutfitName.ToStdString();
		outfitStudio->UpdateTitle();
	}

	outfitStudio->DeleteSliders(mergeSliders, mergeZaps); // we need to do this first so we can clear any broken sliders
	project->ResetTransforms();

	auto originalShapes = project->GetWorkNif()->GetShapes(); // get outfit shapes
	if (!deleteShapesText.IsEmpty()) {
		outfitStudio->UpdateProgress(5, _("Deleting Shapes..."));
		wxStringTokenizer tkz(deleteShapesText, wxT(","));

		while (tkz.HasMoreTokens()) {
			wxString token = tkz.GetNextToken();
			for (auto& shape : originalShapes) {
				if (shape == nullptr)
					continue;
				auto shapeName = wxString(shape->name.get().c_str());
				if (shapeName.Contains(token)) {
					project->DeleteShape(shape);
					shape = nullptr;
				}
			}
		}
	}

	project->DeleteShape(project->GetBaseShape());
	auto remainingOutfitShapes = project->GetWorkNif()->GetShapes(); // get outfit shapes

	if (conversionRefTemplate != "None") {
		outfitStudio->UpdateProgress(5, _("Loading conversion reference..."));
		outfitStudio->StartSubProgress(5, 10);
		if (AlertProgressError(LoadReferenceTemplate(conversionRefTemplate, mergeSliders, mergeZaps), "Load Error", "Failed to load conversion reference"))
			return;
		outfitStudio->EndProgress();

		outfitStudio->StartSubProgress(10, 20);
		outfitStudio->CreateSetSliders();
		outfitStudio->RefreshGUIFromProj();

		outfitStudio->UpdateProgress(20, _("Conforming outfit parts..."));
		outfitStudio->StartSubProgress(20, 35);

		// We shouldn't ever need to skip using default for this case as a correct conversion reference should always conform accurately
		if (AlertProgressError(outfitStudio->ConformShapes(remainingOutfitShapes, true), "Conform Error", "Failed to conform shapes"))
			return;

		outfitStudio->UpdateProgress(35, _("Updating conversion Slider..."));
		outfitStudio->SetSliderValue(project->activeSet.size() - 1, 100);
		outfitStudio->ApplySliders();

		outfitStudio->UpdateProgress(40, _("Setting the base shape and removing the conversion reference"));
		outfitStudio->SetBaseShape();
		project->DeleteShape(project->GetBaseShape());
		outfitStudio->DeleteSliders(mergeSliders, mergeZaps);
		project->GetWorkAnim()->Clear();
	}
	else {
		outfitStudio->UpdateProgress(5, _("Skipping conversion reference..."));
	}

	outfitStudio->RefreshGUIFromProj();

	outfitStudio->UpdateProgress(50, _("Loading new reference..."));
	outfitStudio->StartSubProgress(50, 55);
	if (AlertProgressError(LoadReferenceTemplate(newRefTemplate, mergeSliders, mergeZaps), "Load Error", "Failed to load new reference"))
		return;
	outfitStudio->EndProgress();

	outfitStudio->StartSubProgress(55, 65);
	outfitStudio->CreateSetSliders();
	outfitStudio->RefreshGUIFromProj();

	if (AlertProgressError(project->GetBaseShape() == nullptr, "Missing Base Shape", "The loaded reference does not contain a base shape"))
		return;

	if (copyBoneWeights) {
		outfitStudio->UpdateProgress(65, _("Copying bones..."));
		outfitStudio->StartSubProgress(65, 85);
		if (AlertProgressError(outfitStudio->CopyBoneWeightForShapes(remainingOutfitShapes, skipCopyBonesPopup), "Copy Bone Weights Error", "Failed to copy bone weights"))
			return;
	}

	if (conformSliders) {
		outfitStudio->UpdateProgress(85, _("Conforming outfit parts..."));
		outfitStudio->StartSubProgress(85, 100);
		if (AlertProgressError(outfitStudio->ConformShapes(remainingOutfitShapes, skipConformPopup), "Conform Error", "Failed to conform shapes"))
			return;
	}

	if (!addBonesText.IsEmpty()) {
		outfitStudio->UpdateProgress(100, _("Adding Bones..."));
		wxStringTokenizer tkz(addBonesText, wxT(","));
		while (tkz.HasMoreTokens()) {
			wxString token = tkz.GetNextToken();
			project->AddBoneRef(token.ToStdString());
		}
	}

	if (deleteReferenceOnCompleted) {
		auto allShapes = project->GetWorkNif()->GetShapes();
		for (auto& s : allShapes) {
			if (std::find(remainingOutfitShapes.begin(), remainingOutfitShapes.end(), s) == remainingOutfitShapes.end())
				project->DeleteShape(s);
		}
	}

	int deletionCount = 0;
	auto workNif = project->GetWorkNif();
	if (workNif)
		workNif->DeleteUnreferencedNodes(&deletionCount);

	auto allShapes = project->GetWorkNif()->GetShapes();
	for (auto& s : allShapes)
		outfitStudio->glView->RecalcNormals(s->name.get());

	outfitStudio->RefreshGUIFromProj();
	outfitStudio->ApplySliders();

	wxLogMessage("Conversion finished.");
	outfitStudio->EndProgress(_("Conversion finished."));
}

int ConvertBodyReferenceDialog::LoadReferenceTemplate(const wxString& refTemplate, bool mergeSliders, bool mergeZaps) const {
	nifly::NiShape* baseShape = project->GetBaseShape();
	if (baseShape)
		outfitStudio->glView->DeleteMesh(baseShape->name.get());

	int error;
	wxLogMessage("Loading reference template '%s'...", refTemplate);
	std::string tmplName{refTemplate.ToUTF8()};
	auto tmpl = find_if(refTemplates.begin(), refTemplates.end(), [&tmplName](const RefTemplate& rt) { return rt.GetName() == tmplName; });
	if (tmpl != refTemplates.end()) {
		if (wxFileName(wxString::FromUTF8(tmpl->GetSource())).IsRelative())
			error = project->LoadReferenceTemplate(GetProjectPath() + PathSepStr + tmpl->GetSource(),
												   tmpl->GetSetName(),
												   tmpl->GetShape(),
												   tmpl->GetLoadAll(),
												   mergeSliders,
												   mergeZaps);
		else
			error = project->LoadReferenceTemplate(tmpl->GetSource(), tmpl->GetSetName(), tmpl->GetShape(), tmpl->GetLoadAll(), mergeSliders, mergeZaps);
	}
	else
		error = 1;

	if (!error) {
		// since some reference files have multiple shapes, just update all textures
		for (auto& s : project->GetWorkNif()->GetShapes())
			project->SetTextures(s);
	}

	return error;
}

bool ConvertBodyReferenceDialog::AlertProgressError(int error, const wxString& title, const wxString& message) const {
	if (error == 0)
		return false;

	wxLogError(message);
	wxMessageBox(message, _(title), wxICON_ERROR);
	outfitStudio->EndProgress("", true);
	outfitStudio->RefreshGUIFromProj();
	return true;
}
