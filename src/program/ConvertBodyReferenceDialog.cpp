/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "ConvertBodyReferenceDialog.h"

#include <NifUtil.hpp>

#include "../utils/ConfigurationManager.h"
#include "../utils/ConfigDialogUtil.h"

#include <regex>

#include "OutfitStudio.h"

class RefTemplate;
extern ConfigurationManager Config;

wxBEGIN_EVENT_TABLE(ConvertBodyReferenceDialog, wxDialog)

wxEND_EVENT_TABLE()

ConvertBodyReferenceDialog::ConvertBodyReferenceDialog(OutfitStudioFrame* outfitStudio, OutfitProject* project, ConfigurationManager& config, const std::vector<RefTemplate>& refTemplates)
	: outfitStudio(outfitStudio), project(project), config(config), refTemplates(refTemplates) {

	wxXmlResource* xrc = wxXmlResource::Get();
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/ConvertBodyReference.xrc");
	xrc->LoadDialog(this, outfitStudio, "dlgConvertBodyRef");

	wxChoice* choice = XRCCTRL((*this), "npConvRefChoice", wxChoice);
	choice->Append("None");

	ConfigDialogUtil::LoadDialogChoices(config, (*this), "npConvRefChoice", refTemplates);
	ConfigDialogUtil::LoadDialogChoices(config, (*this), "npNewRefChoice", refTemplates);
	ConfigDialogUtil::LoadDialogText(config, (*this), "npRemoveText");
	ConfigDialogUtil::LoadDialogText(config, (*this), "npAppendText");
	ConfigDialogUtil::LoadDialogText(config, (*this), "npDeleteShapesText");
	ConfigDialogUtil::LoadDialogText(config, (*this), "npAddBonesText");
	ConfigDialogUtil::LoadDialogCheckBox(config, (*this), "chkKeepZapSliders");
	ConfigDialogUtil::LoadDialogCheckBox(config, (*this), "chkSkipConformPopup");
	ConfigDialogUtil::LoadDialogCheckBox(config, (*this), "chkSkipCopyBonesPopup");
	ConfigDialogUtil::LoadDialogCheckBox(config, (*this), "chkDeleteReferenceOnComplete");

	SetDoubleBuffered(true);
	CenterOnParent();
}
	
ConvertBodyReferenceDialog::~ConvertBodyReferenceDialog() {
	wxXmlResource::Get()->Unload(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/ConvertBodyReferenceDialog.xrc");
}

void ConvertBodyReferenceDialog::ConvertBodyReference() const
{
	outfitStudio->StartProgress(_("Start Conversion.."));

	bool keepZapSliders = ConfigDialogUtil::SetBoolFromDialogCheckbox(config, (*this), "chkKeepZapSliders");
	bool skipConformPopup = ConfigDialogUtil::SetBoolFromDialogCheckbox(config, (*this), "chkSkipConformPopup");
	bool skipCopyBonesPopup = ConfigDialogUtil::SetBoolFromDialogCheckbox(config, (*this), "chkSkipCopyBonesPopup");
	bool deleteReferenceOnCompleted = ConfigDialogUtil::SetBoolFromDialogCheckbox(config, (*this), "chkDeleteReferenceOnComplete");
	auto conversionRefTemplate = ConfigDialogUtil::SetStringFromDialogChoice(config, (*this), "npConvRefChoice");
	auto newRefTemplate = ConfigDialogUtil::SetStringFromDialogChoice(config, (*this), "npNewRefChoice");
	auto removeFromProjectText = ConfigDialogUtil::SetStringFromDialogTextControl(config, (*this), "npRemoveText");
	auto appendToProjectText = ConfigDialogUtil::SetStringFromDialogTextControl(config, (*this), "npAppendText");
	auto deleteShapesText = ConfigDialogUtil::SetStringFromDialogTextControl(config, (*this), "npDeleteShapesText");
	auto addBonesText = ConfigDialogUtil::SetStringFromDialogTextControl(config, (*this), "npAddBonesText");

	Config.SaveConfig(Config["AppDir"] + "/Config.xml");

	outfitStudio->UpdateProgress(1, _("Updating Project Output Settings"));

	if (!removeFromProjectText.IsEmpty())
	{
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

	outfitStudio->DeleteSliders(false, true); // we need to do this first so we can clear any broken sliders
	project->ResetTransforms();

	auto originalShapes = project->GetWorkNif()->GetShapes(); // get outfit shapes
	if (!deleteShapesText.IsEmpty())
	{
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

	if (conversionRefTemplate != "None")
	{
		outfitStudio->UpdateProgress(5, _("Loading conversion reference..."));
		outfitStudio->StartSubProgress(5, 10);
		if (AlertProgressError(LoadReferenceTemplate(conversionRefTemplate, keepZapSliders), "Load Error", "Failed to load conversion reference"))
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
		outfitStudio->DeleteSliders(false, keepZapSliders);
		project->GetWorkAnim()->Clear();
	}
	else {
		outfitStudio->UpdateProgress(5, _("Skipping conversion reference..."));
	}

	outfitStudio->RefreshGUIFromProj();

	outfitStudio->UpdateProgress(50, _("Loading new reference..."));
	outfitStudio->StartSubProgress(50, 55);
	if (AlertProgressError(LoadReferenceTemplate(newRefTemplate, keepZapSliders), "Load Error", "Failed to load new reference"))
		return;
	outfitStudio->EndProgress();

	outfitStudio->StartSubProgress(55, 65);
	outfitStudio->CreateSetSliders();
	outfitStudio->RefreshGUIFromProj();

	if (AlertProgressError(project->GetBaseShape() == nullptr, "Missing Base Shape", "The loaded reference does not contain a base shape"))
		return;

	outfitStudio->UpdateProgress(65, _("Copying bones..."));
	outfitStudio->StartSubProgress(65, 85);
	if (AlertProgressError(outfitStudio->CopyBoneWeightForShapes(remainingOutfitShapes, skipCopyBonesPopup), "Copy Bone Weights Error", "Failed to copy bone weights"))
		return;

	outfitStudio->UpdateProgress(85, _("Conforming outfit parts..."));
	outfitStudio->StartSubProgress(85, 100);
	if (AlertProgressError(outfitStudio->ConformShapes(remainingOutfitShapes, skipConformPopup), "Conform Error", "Failed to conform shapes"))
		return;

	if (!addBonesText.IsEmpty())
	{
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

	wxLogMessage("Finished conversion.");
	outfitStudio->UpdateProgress(100, _("Finished"));
	outfitStudio->EndProgress();
}

int ConvertBodyReferenceDialog::LoadReferenceTemplate(const wxString& refTemplate, bool keepZapSliders) const
{
	nifly::NiShape* baseShape = project->GetBaseShape();
	if (baseShape)
		outfitStudio->glView->DeleteMesh(baseShape->name.get());

	int error;
	wxLogMessage("Loading reference template '%s'...", refTemplate);
	std::string tmplName{ refTemplate.ToUTF8() };
	auto tmpl = find_if(refTemplates.begin(), refTemplates.end(), [&tmplName](const RefTemplate& rt) { return rt.GetName() == tmplName; });
	if (tmpl != refTemplates.end()) {
		if (wxFileName(wxString::FromUTF8(tmpl->GetSource())).IsRelative())
			error = project->LoadReferenceTemplate(GetProjectPath() + PathSepStr + tmpl->GetSource(), tmpl->GetSetName(), tmpl->GetShape(), tmpl->GetLoadAll(), false, keepZapSliders);
		else
			error = project->LoadReferenceTemplate(tmpl->GetSource(), tmpl->GetSetName(), tmpl->GetShape(), tmpl->GetLoadAll(), false, keepZapSliders);
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

bool ConvertBodyReferenceDialog::AlertProgressError(int error, const wxString& title, const wxString& message) const
{
	if (error == 0)
		return false;

	wxLogError(message);
	wxMessageBox(message, _(title), wxICON_ERROR);
	outfitStudio->EndProgress("", true);
	outfitStudio->RefreshGUIFromProj();
	return true;
}
