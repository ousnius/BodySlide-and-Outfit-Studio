/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "AutomationDialog.h"

#include "OutfitProject.h"
#include "OutfitStudio.h"

#include "../components/ClippingFixer.h"
#include "../components/DiffData.h"
#include "../components/UndoState.h"
#include "../files/MaskFile.h"
#include "../files/TriFile.h"
#include "../utils/StringStuff.h"

#include <NifFile.hpp>

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace nifly;

namespace {
int ExpectedBSShaderTextureCount(const NiVersion& version) {
	if (version.User() == 12 && version.Stream() == 155)
		return 13;
	if (version.User() == 12 && version.Stream() == 130)
		return 10;
	if (version.User() == 12)
		return 9;
	return 6;
}

bool ParseShaderTypeValue(const std::string& value, std::string& domain, uint32_t& shaderType) {
	size_t sep = value.find(':');
	if (sep == std::string::npos)
		return false;

	domain = value.substr(0, sep);
	try {
		shaderType = static_cast<uint32_t>(std::stoul(value.substr(sep + 1)));
	}
	catch (...) {
		return false;
	}

	return true;
}

bool ApplyAutomationShaderProperty(NifFile* nif, NiShape* shape, const AutomationStep::ShaderProperty& prop) {
	NiShader* shader = nif->GetShader(shape);
	if (!shader)
		return false;

	NiMaterialProperty* material = nif->GetMaterialProperty(shape);
	auto* bslsp = dynamic_cast<BSLightingShaderProperty*>(shader);
	auto* bsesp = dynamic_cast<BSEffectShaderProperty*>(shader);
	auto* bspplp = dynamic_cast<BSShaderPPLightingProperty*>(shader);
	auto* bssp = dynamic_cast<BSShaderProperty*>(shader);
	auto& version = nif->GetHeader().GetVersion();

	Vector3 vectorValue(prop.value1, prop.value2, prop.value3);
	Color4 colorValue(prop.value1, prop.value2, prop.value3, prop.value4);

	if (prop.name == "ShaderType") {
		std::string domain;
		uint32_t shaderType = 0;
		if (!ParseShaderTypeValue(prop.stringValue, domain, shaderType))
			return false;

		if (domain == "BSLighting" && bslsp) {
			uint32_t oldType = bslsp->GetShaderType();
			bslsp->SetShaderType(shaderType);

			if (oldType != BSLightingShaderPropertyShaderType::BSLSP_ENVMAP && shaderType == BSLightingShaderPropertyShaderType::BSLSP_ENVMAP)
				bslsp->SetEnvironmentMapping(true);
			else if (oldType == BSLightingShaderPropertyShaderType::BSLSP_ENVMAP && shaderType != BSLightingShaderPropertyShaderType::BSLSP_ENVMAP)
				bslsp->SetEnvironmentMapping(false);

			return true;
		}

		if (domain == "PPLighting" && bspplp) {
			shader->SetShaderType(shaderType);
			return true;
		}

		return false;
	}

	if (prop.name == "SpecularColor") {
		bool applied = false;
		if (bslsp) {
			bslsp->SetSpecularColor(vectorValue);
			applied = true;
		}
		if (material) {
			material->SetSpecularColor(vectorValue);
			applied = true;
		}
		return applied;
	}

	if (prop.name == "SpecularStrength") {
		if (!bslsp)
			return false;
		bslsp->SetSpecularStrength(prop.value1);
		return true;
	}

	if (prop.name == "SpecularPower") {
		bool applied = false;
		if (bslsp) {
			bslsp->SetGlossiness(prop.value1);
			applied = true;
		}
		if (material) {
			material->SetGlossiness(prop.value1);
			applied = true;
		}
		return applied;
	}

	if (prop.name == "EmissiveColor") {
		bool applied = false;
		if (bslsp) {
			bslsp->SetEmissiveColor(colorValue);
			applied = true;
		}
		if (bsesp) {
			bsesp->SetEmissiveColor(colorValue);
			applied = true;
		}
		if (bspplp && version.User() >= 12) {
			bspplp->emissiveColor = colorValue;
			applied = true;
		}
		if (material) {
			material->SetEmissiveColor(colorValue);
			applied = true;
		}
		return applied;
	}

	if (prop.name == "EmissiveMultiple") {
		bool applied = false;
		if (bslsp) {
			bslsp->SetEmissiveMultiple(prop.value1);
			applied = true;
		}
		if (bsesp) {
			bsesp->SetEmissiveMultiple(prop.value1);
			applied = true;
		}
		if (material) {
			material->SetEmissiveMultiple(prop.value1);
			applied = true;
		}
		return applied;
	}

	if (prop.name == "Alpha") {
		bool applied = false;
		if (bslsp) {
			bslsp->SetAlpha(prop.value1);
			applied = true;
		}
		if (material) {
			material->SetAlpha(prop.value1);
			applied = true;
		}
		return applied;
	}

	if (prop.name == "EnvMapScale") {
		bool applied = false;
		if (bslsp && bslsp->GetShaderType() == BSLightingShaderPropertyShaderType::BSLSP_ENVMAP) {
			bslsp->environmentMapScale = prop.value1;
			applied = true;
		}
		else if (bsesp && version.User() == 12 && version.Stream() >= 130) {
			bsesp->envMapScale = prop.value1;
			applied = true;
		}
		else if (bssp && version.User() <= 11) {
			bssp->environmentMapScale = prop.value1;
			applied = true;
		}
		return applied;
	}

	if (prop.name == "EyeCubemapScale") {
		if (!bslsp || bslsp->GetShaderType() != BSLightingShaderPropertyShaderType::BSLSP_EYE)
			return false;
		bslsp->eyeCubemapScale = prop.value1;
		return true;
	}

	if (prop.name == "UVOffset") {
		if (!bssp || version.User() != 12)
			return false;
		bssp->uvOffset = Vector2(prop.value1, prop.value2);
		return true;
	}

	if (prop.name == "UVScale") {
		if (!bssp || version.User() != 12)
			return false;
		bssp->uvScale = Vector2(prop.value1, prop.value2);
		return true;
	}

	if (prop.name == "LightingEffect1") {
		if (!bslsp || version.Stream() >= 130)
			return false;
		bslsp->softlighting = prop.value1;
		return true;
	}

	if (prop.name == "LightingEffect2") {
		if (!bslsp || version.Stream() >= 130)
			return false;
		bslsp->rimlightPower = prop.value1;
		return true;
	}

	if (prop.name == "SkinTintColor") {
		if (!bslsp || bslsp->GetShaderType() != BSLightingShaderPropertyShaderType::BSLSP_SKINTINT)
			return false;
		bslsp->skinTintColor = vectorValue;
		return true;
	}

	if (prop.name == "HairTintColor") {
		if (!bslsp || bslsp->GetShaderType() != BSLightingShaderPropertyShaderType::BSLSP_HAIRTINT)
			return false;
		bslsp->hairTintColor = vectorValue;
		return true;
	}

	if (prop.name == "RefractionStrength") {
		bool applied = false;
		if (bslsp) {
			bslsp->refractionStrength = prop.value1;
			applied = true;
		}
		if (bspplp && version.User() == 11 && version.Stream() > 14) {
			bspplp->refractionStrength = prop.value1;
			applied = true;
		}
		if (bsesp && version.User() == 12 && version.Stream() > 139 && version.Stream() < 172) {
			bsesp->refractionPower = prop.value1;
			applied = true;
		}
		return applied;
	}

	return false;
}

bool IsSupportedAutomationExtraDataType(const std::string& type) {
	return type == "NiStringExtraData"
		|| type == "NiIntegerExtraData"
		|| type == "NiFloatExtraData"
		|| type == "NiBooleanExtraData";
}

std::unique_ptr<NiExtraData> CreateAutomationExtraData(const std::string& type, const std::string& name) {
	std::unique_ptr<NiExtraData> extraData;
	if (type == "NiStringExtraData")
		extraData = std::make_unique<NiStringExtraData>();
	else if (type == "NiIntegerExtraData")
		extraData = std::make_unique<NiIntegerExtraData>();
	else if (type == "NiFloatExtraData")
		extraData = std::make_unique<NiFloatExtraData>();
	else if (type == "NiBooleanExtraData")
		extraData = std::make_unique<NiBooleanExtraData>();

	if (extraData)
		extraData->name.get() = name;

	return extraData;
}

bool ApplyAutomationExtraDataValue(NiExtraData* extraData, const std::string& value, std::string& error) {
	if (!extraData) {
		error = "missing extra data block";
		return false;
	}

	if (auto* stringExtraData = dynamic_cast<NiStringExtraData*>(extraData)) {
		stringExtraData->stringData.get() = value;
		return true;
	}

	if (auto* intExtraData = dynamic_cast<NiIntegerExtraData*>(extraData)) {
		uint32_t parsed = 0;
		if (!ParseUInt32Value(value, parsed)) {
			error = "expected an unsigned integer";
			return false;
		}
		intExtraData->integerData = parsed;
		return true;
	}

	if (auto* floatExtraData = dynamic_cast<NiFloatExtraData*>(extraData)) {
		float parsed = 0.0f;
		if (!ParseFloatValue(value, parsed)) {
			error = "expected a floating point number";
			return false;
		}
		floatExtraData->floatData = parsed;
		return true;
	}

	if (auto* boolExtraData = dynamic_cast<NiBooleanExtraData*>(extraData)) {
		bool parsed = false;
		if (!ParseBoolValue(value, parsed)) {
			error = "expected true or false";
			return false;
		}
		boolExtraData->booleanData = parsed;
		return true;
	}

	error = "unsupported extra data block type";
	return false;
}

std::string ExtraDataTargetLabel(NiAVObject* target) {
	if (!target)
		return "<none>";

	std::string label = target->name.get();
	return label.empty() ? "<root>" : label;
}

bool ConvertToSubIndexTriShape(NifFile* nif, OutfitStudioFrame* outfitStudio, NiShape*& shape) {
	auto* bsTriShape = dynamic_cast<BSTriShape*>(shape);
	if (!bsTriShape || shape->HasType<BSSubIndexTriShape>())
		return false;

	auto bsSITS = std::make_unique<BSSubIndexTriShape>();
	*static_cast<BSTriShape*>(bsSITS.get()) = *bsTriShape;
	bsSITS->SetDefaultSegments();
	bsSITS->name.get() = bsTriShape->name.get();

	NiShape* newShape = bsSITS.get();
	outfitStudio->UpdateShapeReference(bsTriShape, newShape);
	nif->GetHeader().ReplaceBlock(nif->GetBlockID(bsTriShape), std::move(bsSITS));
	shape = newShape;
	return true;
}

bool ConvertFromSubIndexTriShape(NifFile* nif, OutfitStudioFrame* outfitStudio, NiShape*& shape) {
	auto* bsTriShape = dynamic_cast<BSTriShape*>(shape);
	if (!bsTriShape || !shape->HasType<BSSubIndexTriShape>())
		return false;

	auto bsTS = std::make_unique<BSTriShape>(*bsTriShape);
	bsTS->name.get() = bsTriShape->name.get();

	NiShape* newShape = bsTS.get();
	outfitStudio->UpdateShapeReference(bsTriShape, newShape);
	nif->GetHeader().ReplaceBlock(nif->GetBlockID(bsTriShape), std::move(bsTS));
	shape = newShape;
	return true;
}

bool ConvertToDynamicTriShape(NifFile* nif, OutfitStudioFrame* outfitStudio, NiShape*& shape) {
	auto* bsTriShape = dynamic_cast<BSTriShape*>(shape);
	if (!bsTriShape || shape->HasType<BSDynamicTriShape>())
		return false;

	auto bsDTS = std::make_unique<BSDynamicTriShape>();
	*static_cast<BSTriShape*>(bsDTS.get()) = *bsTriShape;
	bsDTS->name.get() = bsTriShape->name.get();

	bsDTS->vertexDesc.RemoveFlag(VF_VERTEX);
	bsDTS->vertexDesc.SetFlag(VF_FULLPREC);
	bsDTS->CalcDynamicData();
	bsDTS->CalcDataSizes(nif->GetHeader().GetVersion());

	NiShape* newShape = bsDTS.get();
	outfitStudio->UpdateShapeReference(bsTriShape, newShape);
	nif->GetHeader().ReplaceBlock(nif->GetBlockID(bsTriShape), std::move(bsDTS));
	shape = newShape;
	return true;
}

bool ConvertFromDynamicTriShape(NifFile* nif, OutfitStudioFrame* outfitStudio, NiShape*& shape) {
	auto* bsTriShape = dynamic_cast<BSTriShape*>(shape);
	if (!bsTriShape || !shape->HasType<BSDynamicTriShape>())
		return false;

	auto bsTS = std::make_unique<BSTriShape>(*bsTriShape);
	bsTS->name.get() = bsTriShape->name.get();

	bsTS->vertexDesc.SetFlag(VF_VERTEX);
	bsTS->vertexDesc.RemoveFlag(VF_FULLPREC);
	bsTS->CalcDataSizes(nif->GetHeader().GetVersion());

	NiShape* newShape = bsTS.get();
	outfitStudio->UpdateShapeReference(bsTriShape, newShape);
	nif->GetHeader().ReplaceBlock(nif->GetBlockID(bsTriShape), std::move(bsTS));
	shape = newShape;
	return true;
}

bool ApplyAutomationGeometryProperty(NifFile* nif,
									 OutfitStudioFrame* outfitStudio,
									 OutfitProject* project,
									 NiShape*& shape,
									 const AutomationStep::GeometryProperty& prop,
									 bool& removedSkinning) {
	if (!shape)
		return false;

	if (prop.name == "Skinned") {
		if (shape->IsSkinned() == prop.enabled)
			return false;

		if (prop.enabled) {
			project->CreateSkinning(shape);
		}
		else {
			project->RemoveSkinning(shape);
			removedSkinning = true;
		}
		return true;
	}

	auto* bsTriShape = dynamic_cast<BSTriShape*>(shape);
	if (!bsTriShape) {
		wxLogWarning("Automation: SetGeometryProperties - '%s' is not a BSTriShape; skipped %s.", shape->name.get(), prop.name.c_str());
		return false;
	}

	auto& version = nif->GetHeader().GetVersion();

	if (prop.name == "FullPrecision") {
		if (version.Stream() == 100) {
			wxLogWarning("Automation: SetGeometryProperties - Full Precision is not editable for stream 100; skipped '%s'.", shape->name.get());
			return false;
		}
		if (!bsTriShape->CanChangePrecision()) {
			wxLogWarning("Automation: SetGeometryProperties - Full Precision cannot be changed for '%s'.", shape->name.get());
			return false;
		}
		if (bsTriShape->IsFullPrecision() == prop.enabled)
			return false;

		bsTriShape->SetFullPrecision(prop.enabled);
		return true;
	}

	if (prop.name == "SubIndex") {
		if (version.Stream() < 130) {
			wxLogWarning("Automation: SetGeometryProperties - Sub Index requires stream 130 or newer; skipped '%s'.", shape->name.get());
			return false;
		}

		return prop.enabled ? ConvertToSubIndexTriShape(nif, outfitStudio, shape) : ConvertFromSubIndexTriShape(nif, outfitStudio, shape);
	}

	if (prop.name == "Dynamic") {
		if (version.Stream() != 100) {
			wxLogWarning("Automation: SetGeometryProperties - Dynamic requires stream 100; skipped '%s'.", shape->name.get());
			return false;
		}

		return prop.enabled ? ConvertToDynamicTriShape(nif, outfitStudio, shape) : ConvertFromDynamicTriShape(nif, outfitStudio, shape);
	}

	return false;
}

BSShaderTextureSet* GetOrCreateBSShaderTextureSet(NifFile* nif, NiShape* shape) {
	NiShader* shader = nif->GetShader(shape);
	if (!shader)
		return nullptr;

	auto textureSetRef = shader->TextureSetRef();
	if (!textureSetRef)
		return nullptr;

	auto textureSet = nif->GetHeader().GetBlock(textureSetRef);
	if (!textureSet) {
		auto newTextureSet = std::make_unique<BSShaderTextureSet>(nif->GetHeader().GetVersion());
		textureSet = newTextureSet.get();
		textureSetRef->index = nif->GetHeader().AddBlock(std::move(newTextureSet));
	}

	int expectedCount = ExpectedBSShaderTextureCount(nif->GetHeader().GetVersion());
	if (static_cast<int>(textureSet->textures.size()) < expectedCount)
		textureSet->textures.resize(expectedCount);

	return textureSet;
}

bool ParseAutomationPartitionID(const std::string& value, int& partitionID) {
	std::string trimmedValue = TrimString(value);
	if (trimmedValue.empty())
		return false;

	std::smatch match;
	static const std::regex partitionChoicePattern("^([0-9]+)\\s+.+$");
	if (!std::regex_match(trimmedValue, match, partitionChoicePattern))
		return false;

	try {
		long parsedValue = std::stol(match[1].str());
		if (parsedValue < std::numeric_limits<int>::min() || parsedValue > std::numeric_limits<int>::max())
			return false;

		partitionID = static_cast<int>(parsedValue);
		return true;
	}
	catch (...) {
		return false;
	}
}
}

bool AutomationDialog::StepChangesSliderSet(AutomationStepType type) {
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
	lastRunErrors = 0;

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

		if (cancelRequested) {
			wxLogMessage("Automation: Cancelled by user.");
			EndProgress(_("Automation cancelled."));
			lastRunErrors++;
			return;
		}

		wxLogMessage("Automation: %s", stepDesc);

		int err = ExecuteStep(step);
		if (err != 0) {
			lastRunErrors++;
			wxString errMsg = wxString::Format(
				_("Step %d (%s) failed with error %d.\n\n%s\n\nContinue with remaining steps?"),
				i + 1,
				wxString::FromUTF8(AutomationStepTypeToString(step.type)),
				err,
				wxString::FromUTF8(step.note));

			if (headlessMode) {
				wxLogError("Automation: %s", errMsg);
				// Auto-continue in headless mode
			}
			else {
				int result = wxMessageBox(errMsg, _("Automation Error"), wxYES_NO | wxICON_ERROR);
				if (result != wxYES) {
					EndProgress(_("Automation aborted."));
					return;
				}
			}
		}

		outfitStudio->RefreshGUIFromProj();

		if (StepChangesSliderSet(step.type))
			outfitStudio->CreateSetSliders();

		outfitStudio->ApplySliders();
	}

	EndProgress(_("Automation complete."));

	if (!headlessMode) {
		wxMessageBox(wxString::Format(_("Automation completed: %d step(s) executed."), totalSteps),
					 _("Automation"), wxICON_INFORMATION);
	}
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

	project->SetTextures();
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
	int err = project->AddFromSliderSet(refSourceFileStd, step.refSet, false, step.refAppendNewSliders);
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
	auto shapes = ResolveTargetShapes(step, false);
	if (shapes.empty()) {
		wxLogWarning("Automation: ConformSliders - no target shapes found.");
		return 0;
	}

	if (!project->GetBaseShape()) {
		wxLogError("Automation: ConformSliders - no reference shape loaded.");
		return 1;
	}

	ConformOptions options;
	options.proximityRadius = step.conformProximityRadius;
	options.maxResults = step.conformMaxResults;
	options.smoothResultDeltas = step.conformSmoothResults;
	options.smoothIterations = step.conformSmoothIterations;
	options.smoothStrength = step.conformSmoothStrength;
	options.noSqueeze = step.conformNoSqueeze;
	options.solidMode = step.conformSolidMode;
	options.axisX = step.conformAxisX;
	options.axisY = step.conformAxisY;
	options.axisZ = step.conformAxisZ;
	options.fixClipping = step.conformFixClipping;
	options.fixClippingStrength = std::max(0.0f, std::min(1.0f, step.conformFixClippingStrength));
	options.sliderNames = step.conformSliderNames;

	if (options.sliderNames.empty()) {
		wxLogMessage("Automation: ConformSliders - conforming all non-zap/non-UV sliders.");
	}
	else {
		wxString namesList;
		for (const auto& sn : options.sliderNames) {
			if (!namesList.empty())
				namesList += ", ";
			namesList += wxString::FromUTF8(sn);
		}
		wxLogMessage("Automation: ConformSliders - conforming %zu named slider(s): %s",
					 options.sliderNames.size(), namesList);
	}

	outfitStudio->ZeroSliders();

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
	auto shapes = ResolveTargetShapes(step, false);
	if (shapes.empty()) {
		wxLogWarning("Automation: CopyBoneWeights - no target shapes found.");
		return 0;
	}

	if (!project->GetBaseShape()) {
		wxLogError("Automation: CopyBoneWeights - no reference shape loaded.");
		return 1;
	}

	AnimInfo& workAnim = *project->GetWorkAnim();
	std::vector<std::string> baseBones;
	if (!step.weightBoneList.empty())
		baseBones = step.weightBoneList;
	else
		baseBones = workAnim.shapeBones[project->GetBaseShape()->name.get()];

	std::sort(baseBones.begin(), baseBones.end());

	int nCopyBones = static_cast<int>(baseBones.size());
	std::vector<std::string> lockedBones;
	bool bSpreadWeight = false;

	if (!step.weightBoneList.empty()) {
		std::unordered_set<std::string> selectedBones(baseBones.begin(), baseBones.end());
		std::vector<std::string> normalizeBones;
		std::vector<std::string> nonNormalizeBones;
		outfitStudio->GetNormalizeBones(&normalizeBones, &nonNormalizeBones);

		for (const auto& bone : normalizeBones) {
			if (!selectedBones.count(bone))
				baseBones.push_back(bone);
		}

		bSpreadWeight = static_cast<int>(baseBones.size()) > nCopyBones;

		if (bSpreadWeight) {
			for (const auto& bone : nonNormalizeBones) {
				if (!selectedBones.count(bone))
					lockedBones.push_back(bone);
			}
		}
		else {
			for (const auto& bone : nonNormalizeBones) {
				if (!selectedBones.count(bone))
					baseBones.push_back(bone);
			}
		}
	}

	UndoStateProject usp;
	usp.undoType = UndoType::Weight;

	for (auto* shape : shapes) {
		if (project->IsBaseShape(shape)) {
			wxLogWarning("Automation: CopyBoneWeights - shape '%s' is the reference shape; skipping.", shape->name.get());
			continue;
		}

		wxLogMessage("Automation: Copying bone weights to '%s'...", shape->name.get());

		std::unordered_map<uint16_t, float> mask;
		outfitStudio->glView->GetShapeMask(mask, shape->name.get());

		usp.usss.resize(usp.usss.size() + 1);
		UndoStateShape& uss = usp.usss.back();
		uss.shapeName = shape->name.get();

		std::vector<std::string> mergedBones = baseBones;

		if (step.weightBoneList.empty()) {
			auto& shapeBones = workAnim.shapeBones[shape->name.get()];
			for (const auto& b : shapeBones) {
				if (!std::binary_search(baseBones.begin(), baseBones.end(), b))
					mergedBones.push_back(b);
			}
		}

		project->CopyBoneWeights(shape,
							 step.weightProximityRadius,
							 step.weightMaxResults,
							 mask,
							 mergedBones,
							 nCopyBones,
							 lockedBones,
							 uss,
							 bSpreadWeight);
	}

	if (!usp.usss.empty())
		outfitStudio->ActiveShapesUpdated(&usp, false);

	project->morpher.ClearProximityCache();
	workAnim.CleanupBones();
	outfitStudio->UpdateAnimationGUI();
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
		wxLogWarning("Automation: RemoveSkinning - no target shapes found.");
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

	outfitStudio->PoseToGUI();
	outfitStudio->glView->UpdateBones();
	outfitStudio->glView->ApplyUndoState(&usp, false);
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
	outfitStudio->glView->RenameShape(step.renameOldName, step.renameNewName);
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

int AutomationDialog::ExecuteStepRefineMesh(const AutomationStep& step) {
	wxLogMessage("Automation: Refining meshes...");

	auto workNif = project->GetWorkNif();
	if (!workNif) {
		wxLogError("Automation: RefineMesh - no work NIF loaded.");
		return 1;
	}

	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: RefineMesh - no target shapes found.");
		return 0;
	}

	constexpr size_t maxVertIndex = std::numeric_limits<uint16_t>().max();
	size_t maxTriIndex = std::numeric_limits<uint16_t>().max();
	if (workNif->GetHeader().GetVersion().IsFO4() || workNif->GetHeader().GetVersion().IsFO76())
		maxTriIndex = std::numeric_limits<uint32_t>().max();

	for (auto* shape : shapes) {
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
	if (step.deleteSliderNames.empty()) {
		wxLogError("Automation: DeleteSlider - no slider name specified.");
		return 1;
	}

	if (step.deleteSliderRegex) {
		// In regex mode, use first entry as pattern
		std::string pattern = JoinStrings(step.deleteSliderNames, ", ");
		try {
			std::regex re(pattern, std::regex::icase);
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
			wxLogMessage("Automation: Deleted %d slider(s) matching '%s'.", deleted, pattern);
		}
		catch (const std::regex_error&) {
			wxLogError("Automation: DeleteSlider - invalid regex '%s'.", pattern);
			return 1;
		}
	}
	else {
		for (const auto& sliderName : step.deleteSliderNames) {
			wxLogMessage("Automation: Deleting slider '%s'...", sliderName);
			project->DeleteSlider(sliderName);
		}
	}
	return 0;
}

int AutomationDialog::ExecuteStepSetReferenceShape(const AutomationStep& step) {
	if (step.setRefUnset) {
		wxLogMessage("Automation: Unsetting reference shape...");
		project->SetBaseShape(nullptr);
		return 0;
	}

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

int AutomationDialog::ExecuteStepChangePartitions(const AutomationStep& step) {
	int sourcePartitionID = 0;
	int destinationPartitionID = 0;
	if (!ParseAutomationPartitionID(step.partitionSource, sourcePartitionID)) {
		wxLogError("Automation: ChangePartitions - invalid source partition '%s'.", step.partitionSource);
		return 1;
	}

	if (!ParseAutomationPartitionID(step.partitionDestination, destinationPartitionID)) {
		wxLogError("Automation: ChangePartitions - invalid destination partition '%s'.", step.partitionDestination);
		return 1;
	}

	if (sourcePartitionID == destinationPartitionID) {
		wxLogWarning("Automation: ChangePartitions - source and destination partitions are both %d; nothing to do.", sourcePartitionID);
		return 0;
	}

	NifFile* workNif = project->GetWorkNif();
	if (!workNif) {
		wxLogError("Automation: ChangePartitions - no work NIF loaded.");
		return 1;
	}

	auto targetShapes = ResolveTargetShapes(step);
	if (targetShapes.empty()) {
		wxLogWarning("Automation: ChangePartitions - no target shapes found.");
		return 0;
	}

	int updatedShapes = 0;
	int affectedTriangles = 0;
	int skippedShapes = 0;
	NiShape* activeShape = outfitStudio->activeItem ? outfitStudio->activeItem->GetShape() : nullptr;
	bool activeShapeUpdated = false;

	for (auto* shape : targetShapes) {
		NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
		std::vector<int> trianglePartitions;
		if (!workNif->GetShapePartitions(shape, partitionInfo, trianglePartitions)) {
			wxLogWarning("Automation: ChangePartitions - shape '%s' has no partitions; skipping.", shape->name.get());
			skippedShapes++;
			continue;
		}

		std::vector<int> sourcePartitionIndices;
		int destinationPartitionIndex = -1;
		for (size_t partitionIndex = 0; partitionIndex < partitionInfo.size(); partitionIndex++) {
			int bodyPartID = partitionInfo[partitionIndex].partID;
			if (bodyPartID == sourcePartitionID)
				sourcePartitionIndices.push_back(static_cast<int>(partitionIndex));
			else if (bodyPartID == destinationPartitionID && destinationPartitionIndex < 0)
				destinationPartitionIndex = static_cast<int>(partitionIndex);
		}

		if (sourcePartitionIndices.empty()) {
			wxLogWarning("Automation: ChangePartitions - shape '%s' has no partition %d; skipping.", shape->name.get(), sourcePartitionID);
			skippedShapes++;
			continue;
		}

		auto isSourcePartitionIndex = [&sourcePartitionIndices](int partitionIndex) {
			return std::find(sourcePartitionIndices.begin(), sourcePartitionIndices.end(), partitionIndex) != sourcePartitionIndices.end();
		};

		int shapeAffectedTriangles = 0;
		for (int partitionIndex : trianglePartitions) {
			if (isSourcePartitionIndex(partitionIndex))
				shapeAffectedTriangles++;
		}

		bool changed = false;
		if (destinationPartitionIndex < 0) {
			destinationPartitionIndex = sourcePartitionIndices.front();
			partitionInfo[destinationPartitionIndex].partID = destinationPartitionID;
			changed = true;
		}

		for (int sourcePartitionIndex : sourcePartitionIndices) {
			if (sourcePartitionIndex == destinationPartitionIndex)
				continue;

			changed = true;
			for (int& trianglePartition : trianglePartitions) {
				if (trianglePartition == sourcePartitionIndex)
					trianglePartition = destinationPartitionIndex;
			}
		}

		if (!changed)
			continue;

		workNif->SetShapePartitions(shape, partitionInfo, trianglePartitions);
		workNif->RemoveEmptyPartitions(shape);
		outfitStudio->MeshFromProj(shape, true);
		if (shape == activeShape)
			activeShapeUpdated = true;

		updatedShapes++;
		affectedTriangles += shapeAffectedTriangles;
		wxLogMessage("Automation: ChangePartitions - changed %d triangle(s) on '%s' from partition %d to %d.",
			shapeAffectedTriangles,
			shape->name.get(),
			sourcePartitionID,
			destinationPartitionID);
	}

	if (updatedShapes > 0) {
		if (activeShapeUpdated && outfitStudio->activeItem && outfitStudio->activeItem->GetShape() == activeShape)
			outfitStudio->RefreshActivePartitionTree();

		outfitStudio->SetPendingChanges();
		outfitStudio->glView->Render();
		wxLogMessage("Automation: ChangePartitions - updated %d shape(s), changed %d triangle assignment(s), skipped %d shape(s).",
			updatedShapes,
			affectedTriangles,
			skippedShapes);
	}
	else {
		wxLogWarning("Automation: ChangePartitions - no matching partition assignments were changed.");
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

int AutomationDialog::ExecuteStepRecalcNormals(const AutomationStep& step) {
	auto* workNif = project->GetWorkNif();
	if (!workNif) {
		wxLogError("Automation: RecalcNormals - no work NIF loaded.");
		return 1;
	}

	// Empty targets include the reference shape on purpose: recalculating the
	// reference's normals is legitimate and the project save skips it otherwise.
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: RecalcNormals - no target shapes found.");
		return 0;
	}

	int recalcCount = 0;

	for (auto* shape : shapes) {
		std::string shapeName = shape->name.get();

		// Persist the settings into the slider set before recalculating. The render
		// meshes are rebuilt from the project after every step, so setting these on
		// the mesh alone would not stick.
		if (step.normalsSeamSmooth >= 0)
			project->activeSet.SetSmoothSeamNormals(shapeName, step.normalsSeamSmooth != 0);

		if (step.normalsSeamAngle >= 0.0f)
			project->activeSet.SetSmoothSeamNormalsAngle(shapeName, step.normalsSeamAngle);

		bool smooth = project->activeSet.GetSmoothSeamNormals(shapeName);
		float angle = project->activeSet.GetSmoothSeamNormalsAngle(shapeName);

		if (project->activeSet.GetLockNormals(shapeName) && !step.normalsForce) {
			wxLogMessage("Automation: RecalcNormals - '%s' has locked normals, skipping.", shapeName);
		}
		else {
			wxLogMessage("Automation: Recalculating normals for '%s' (seams=%d, angle=%0.2f)...", shapeName, smooth, angle);

			// Honors LOCKEDNORM vertices; force also overrides the model space shader skip on SK/SSE
			workNif->CalcNormalsForShape(shape, step.normalsForce, smooth, angle);
			workNif->CalcTangentsForShape(shape);
			recalcCount++;
		}

		// Applied after the recalculation so that force + lock is a usable combination
		if (step.normalsLock >= 0)
			project->activeSet.SetLockNormals(shapeName, step.normalsLock != 0);
	}

	if (recalcCount > 0)
		outfitStudio->SetPendingChanges();

	wxLogMessage("Automation: RecalcNormals - recalculated normals on %d shape(s).", recalcCount);
	return 0;
}

int AutomationDialog::ExecuteStepClearMask(const AutomationStep& step) {
	auto targetShapes = ResolveTargetShapes(step);
	if (targetShapes.empty()) {
		wxLogWarning("Automation: ClearMask - no target shapes found.");
		return 0;
	}

	for (auto* shape : targetShapes) {
		std::string shapeName = shape->name.get();
		Mesh* mesh = outfitStudio->glView->GetMesh(shapeName);
		if (!mesh)
			continue;

		wxLogMessage("Automation: Clearing mask for shape '%s'...", shapeName);
		mesh->MaskFill(0.0f);
	}

	outfitStudio->glView->Render();
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

int AutomationDialog::ExecuteStepSetShaderProperties(const AutomationStep& step) {
	if (step.shaderProperties.empty()) {
		wxLogWarning("Automation: SetShaderProperties - no shader properties configured.");
		return 0;
	}

	NifFile* nif = project->GetWorkNif();
	if (!nif)
		return 0;

	auto targetShapes = ResolveTargetShapes(step);

	if (targetShapes.empty()) {
		wxLogWarning("Automation: SetShaderProperties - no target shapes found.");
		return 0;
	}

	int updatedShapes = 0;
	int updatedValues = 0;

	for (auto* shape : targetShapes) {
		if (!shape)
			continue;

		NiShader* shader = nif->GetShader(shape);
		if (!shader)
			continue;

		int shapeUpdates = 0;
		for (const auto& prop : step.shaderProperties) {
			if (prop.name == "ShaderType" && ApplyAutomationShaderProperty(nif, shape, prop))
				shapeUpdates++;
		}

		for (const auto& prop : step.shaderProperties) {
			if (prop.name != "ShaderType" && ApplyAutomationShaderProperty(nif, shape, prop))
				shapeUpdates++;
		}

		if (shapeUpdates > 0) {
			updatedShapes++;
			updatedValues += shapeUpdates;
			project->SetTextures(shape);
			outfitStudio->MeshFromProj(shape, true);
			wxLogMessage("Automation: SetShaderProperties - updated %d shader properties on '%s'.",
				shapeUpdates,
				shape->name.get());
		}
	}

	if (updatedShapes > 0) {
		outfitStudio->SetPendingChanges();
		outfitStudio->glView->Render();
		wxLogMessage("Automation: SetShaderProperties - updated %d shader values on %d shapes.", updatedValues, updatedShapes);
	}
	else {
		wxLogWarning("Automation: SetShaderProperties - found no matching shader properties on target shapes.");
	}

	return 0;
}

int AutomationDialog::ExecuteStepSetGeometryProperties(const AutomationStep& step) {
	if (step.geometryProperties.empty()) {
		wxLogWarning("Automation: SetGeometryProperties - no geometry properties configured.");
		return 0;
	}

	NifFile* nif = project->GetWorkNif();
	if (!nif)
		return 0;

	auto targetShapes = ResolveTargetShapes(step);
	if (targetShapes.empty()) {
		wxLogWarning("Automation: SetGeometryProperties - no target shapes found.");
		return 0;
	}

	const char* propertyOrder[] = {"FullPrecision", "SubIndex", "Dynamic", "Skinned"};
	std::vector<NiShape*> changedShapes;
	bool removedSkinning = false;
	int updatedShapes = 0;
	int updatedValues = 0;

	for (auto* targetShape : targetShapes) {
		if (!targetShape)
			continue;

		NiShape* shape = targetShape;
		int shapeUpdates = 0;
		for (const char* propertyName : propertyOrder) {
			for (const auto& prop : step.geometryProperties) {
				if (prop.name == propertyName && ApplyAutomationGeometryProperty(nif, outfitStudio, project, shape, prop, removedSkinning))
					shapeUpdates++;
			}
		}

		if (shapeUpdates > 0) {
			updatedShapes++;
			updatedValues += shapeUpdates;
			changedShapes.push_back(shape);
			wxLogMessage("Automation: SetGeometryProperties - updated %d geometry properties on '%s'.",
				shapeUpdates,
				shape->name.get());
		}
	}

	if (updatedShapes > 0) {
		if (removedSkinning) {
			nif->DeleteUnreferencedNodes();
			outfitStudio->UpdateAnimationGUI();
		}

		for (auto* shape : changedShapes) {
			if (!shape)
				continue;
			project->SetTextures(shape);
			outfitStudio->MeshFromProj(shape, true);
		}

		outfitStudio->SetPendingChanges();
		outfitStudio->glView->Render();
		wxLogMessage("Automation: SetGeometryProperties - updated %d geometry values on %d shapes.", updatedValues, updatedShapes);
	}
	else {
		wxLogWarning("Automation: SetGeometryProperties - found no applicable geometry properties on target shapes.");
	}

	return 0;
}

int AutomationDialog::ExecuteStepSetExtraData(const AutomationStep& step) {
	NifFile* nif = project->GetWorkNif();
	if (!nif)
		return 0;

	std::string type = TrimString(step.extraDataType.empty() ? "NiStringExtraData" : step.extraDataType);
	std::string name = TrimString(step.extraDataName);
	if (name.empty()) {
		wxLogError("Automation: SetExtraData - extra data name is empty.");
		return 1;
	}

	if (!IsSupportedAutomationExtraDataType(type)) {
		wxLogError("Automation: SetExtraData - unsupported extra data block type '%s'.", type.c_str());
		return 1;
	}

	auto validationBlock = CreateAutomationExtraData(type, name);
	std::string valueError;
	if (!ApplyAutomationExtraDataValue(validationBlock.get(), step.extraDataValue, valueError)) {
		wxLogError("Automation: SetExtraData - invalid value for %s '%s': %s.", type.c_str(), name.c_str(), valueError.c_str());
		return 1;
	}

	std::vector<NiAVObject*> targets;
	if (step.targetMeshes.empty()) {
		NiNode* root = nif->GetRootNode();
		if (!root) {
			wxLogError("Automation: SetExtraData - no root node found.");
			return 1;
		}
		targets.push_back(root);
	}
	else {
		auto shapes = ResolveTargetShapes(step);
		for (auto* shape : shapes)
			targets.push_back(shape);
	}

	if (targets.empty()) {
		wxLogWarning("Automation: SetExtraData - no targets found.");
		return 0;
	}

	int updatedBlocks = 0;
	int addedBlocks = 0;
	int skippedExistingBlocks = 0;
	for (auto* target : targets) {
		if (!target)
			continue;

		int targetUpdates = 0;
		bool found = false;
		int refCount = static_cast<int>(target->extraDataRefs.GetSize());
		for (int i = refCount - 1; i >= 0; i--) {
			uint32_t blockId = target->extraDataRefs.GetBlockRef(i);
			auto* extraData = nif->GetHeader().GetBlock<NiExtraData>(blockId);
			if (!extraData || extraData->name.get() != name)
				continue;

			found = true;
			if (std::string(extraData->GetBlockName()) == type) {
				std::string error;
				if (!ApplyAutomationExtraDataValue(extraData, step.extraDataValue, error)) {
					wxLogError("Automation: SetExtraData - failed to update %s '%s' on '%s': %s.",
						type.c_str(),
						name.c_str(),
						ExtraDataTargetLabel(target).c_str(),
						error.c_str());
					return 1;
				}
				updatedBlocks++;
				targetUpdates++;
			}
			else {
				skippedExistingBlocks++;
				wxLogWarning("Automation: SetExtraData - '%s' already exists on '%s' as %s; not creating a duplicate %s block.",
					name.c_str(),
					ExtraDataTargetLabel(target).c_str(),
					extraData->GetBlockName(),
					type.c_str());
			}
		}

		if (!found) {
			auto extraData = CreateAutomationExtraData(type, name);
			std::string error;
			if (!ApplyAutomationExtraDataValue(extraData.get(), step.extraDataValue, error)) {
				wxLogError("Automation: SetExtraData - failed to add %s '%s' on '%s': %s.",
					type.c_str(),
					name.c_str(),
					ExtraDataTargetLabel(target).c_str(),
					error.c_str());
				return 1;
			}

			nif->AssignExtraData(target, std::move(extraData));
			addedBlocks++;
			targetUpdates++;
		}

		if (targetUpdates > 0) {
			wxLogMessage("Automation: SetExtraData - set '%s' on '%s'.", name.c_str(), ExtraDataTargetLabel(target).c_str());
		}
	}

	if (updatedBlocks > 0 || addedBlocks > 0) {
		outfitStudio->SetPendingChanges();
		outfitStudio->glView->Render();
		wxLogMessage("Automation: SetExtraData - updated %d, added %d extra data blocks (%d existing-name blocks skipped).",
			updatedBlocks,
			addedBlocks,
			skippedExistingBlocks);
	}
	else if (skippedExistingBlocks > 0) {
		wxLogWarning("Automation: SetExtraData - no extra data blocks were changed (%d existing-name blocks skipped).", skippedExistingBlocks);
	}

	return 0;
}

int AutomationDialog::ExecuteStepDeleteExtraData(const AutomationStep& step) {
	NifFile* nif = project->GetWorkNif();
	if (!nif)
		return 0;

	std::string name = TrimString(step.extraDataName);
	if (name.empty()) {
		wxLogError("Automation: DeleteExtraData - extra data name is empty.");
		return 1;
	}

	std::vector<NiAVObject*> targets;
	if (step.targetMeshes.empty()) {
		NiNode* root = nif->GetRootNode();
		if (!root) {
			wxLogError("Automation: DeleteExtraData - no root node found.");
			return 1;
		}
		targets.push_back(root);
	}
	else {
		auto shapes = ResolveTargetShapes(step);
		for (auto* shape : shapes)
			targets.push_back(shape);
	}

	if (targets.empty()) {
		wxLogWarning("Automation: DeleteExtraData - no targets found.");
		return 0;
	}

	int deletedBlocks = 0;
	for (auto* target : targets) {
		if (!target)
			continue;

		int targetDeletes = 0;
		int refCount = static_cast<int>(target->extraDataRefs.GetSize());
		for (int i = refCount - 1; i >= 0; i--) {
			uint32_t blockId = target->extraDataRefs.GetBlockRef(i);
			auto* extraData = nif->GetHeader().GetBlock<NiExtraData>(blockId);
			if (!extraData || extraData->name.get() != name)
				continue;

			nif->GetHeader().DeleteBlock(blockId);
			deletedBlocks++;
			targetDeletes++;
		}

		if (targetDeletes > 0) {
			wxLogMessage("Automation: DeleteExtraData - removed %d '%s' blocks from '%s'.",
				targetDeletes,
				name.c_str(),
				ExtraDataTargetLabel(target).c_str());
		}
	}

	if (deletedBlocks > 0) {
		outfitStudio->SetPendingChanges();
		outfitStudio->glView->Render();
		wxLogMessage("Automation: DeleteExtraData - removed %d extra data blocks.", deletedBlocks);
	}
	else {
		wxLogWarning("Automation: DeleteExtraData - no extra data named '%s' found.", name.c_str());
	}

	return 0;
}

int AutomationDialog::ExecuteStepSetTexturePaths(const AutomationStep& step) {
	if (step.texturePaths.empty()) {
		wxLogWarning("Automation: SetTexturePaths - no texture paths configured.");
		return 0;
	}

	NifFile* nif = project->GetWorkNif();
	if (!nif)
		return 0;

	std::vector<AutomationStep::TexturePath> texturePaths;
	for (const auto& path : step.texturePaths) {
		int index = ResolveTexturePathIndex(path);
		if (index < 0) {
			wxLogWarning("Automation: SetTexturePaths - texture slot '%s' could not be resolved; skipping.", path.name.c_str());
			continue;
		}

		int namedIndex = path.name.empty() ? -1 : TexturePathIndexForName(path.name);
		if (path.index >= 0 && namedIndex >= 0 && namedIndex != path.index) {
			wxLogWarning("Automation: SetTexturePaths - slot name '%s' points to %d but index %d was specified; using index %d.",
				path.name.c_str(),
				namedIndex,
				path.index,
				path.index);
		}

		AutomationStep::TexturePath resolvedPath = path;
		resolvedPath.index = index;
		if (resolvedPath.name.empty())
			resolvedPath.name = TexturePathNameForIndex(index);
		texturePaths.push_back(std::move(resolvedPath));
	}

	if (texturePaths.empty()) {
		wxLogWarning("Automation: SetTexturePaths - no valid texture slots configured.");
		return 0;
	}

	auto targetShapes = ResolveTargetShapes(step);
	if (targetShapes.empty()) {
		wxLogWarning("Automation: SetTexturePaths - no target shapes found.");
		return 0;
	}

	std::vector<NiShape*> changedShapes;
	int updatedShapes = 0;
	int updatedValues = 0;
	int skippedValues = 0;

	for (auto* shape : targetShapes) {
		if (!shape)
			continue;

		BSShaderTextureSet* textureSet = GetOrCreateBSShaderTextureSet(nif, shape);
		if (!textureSet) {
			skippedValues += static_cast<int>(texturePaths.size());
			wxLogWarning("Automation: SetTexturePaths - '%s' has no BSShaderTextureSet-capable shader; skipped.", shape->name.get());
			continue;
		}

		int expectedCount = ExpectedBSShaderTextureCount(nif->GetHeader().GetVersion());
		int shapeUpdates = 0;
		for (const auto& path : texturePaths) {
			if (path.index < 0 || path.index >= expectedCount) {
				skippedValues++;
				wxLogWarning("Automation: SetTexturePaths - slot %d is not valid for this NIF version on '%s'.", path.index, shape->name.get());
				continue;
			}

			std::string texturePath = ToBackslashes(path.path);
			if (textureSet->textures[path.index].get() == texturePath)
				continue;

			textureSet->textures[path.index].get() = texturePath;
			shapeUpdates++;
		}

		if (shapeUpdates > 0) {
			updatedShapes++;
			updatedValues += shapeUpdates;
			changedShapes.push_back(shape);
			wxLogMessage("Automation: SetTexturePaths - updated %d texture paths on '%s'.", shapeUpdates, shape->name.get());
		}
	}

	if (updatedShapes > 0) {
		nif->TrimTexturePaths();
		for (auto* shape : changedShapes) {
			if (!shape)
				continue;
			project->SetTextures(shape);
			outfitStudio->MeshFromProj(shape, true);
		}

		outfitStudio->SetPendingChanges();
		outfitStudio->glView->Render();
		wxLogMessage("Automation: SetTexturePaths - updated %d texture paths on %d shapes (%d skipped).", updatedValues, updatedShapes, skippedValues);
	}
	else {
		wxLogWarning("Automation: SetTexturePaths - found no applicable texture paths on target shapes (%d skipped).", skippedValues);
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

int AutomationDialog::ExecuteStepFixClipping(const AutomationStep& step) {
	nifly::NiShape* refShape = project->GetBaseShape();
	if (!refShape) {
		wxLogError("Automation: FixClipping - no reference shape set.");
		return 1;
	}

	ClippingFixOptions options;
	options.strength = std::max(0.0f, std::min(1.0f, step.fixClipStrength));
	if (options.strength <= 0.0f) {
		wxLogWarning("Automation: FixClipping - strength is 0, nothing to do.");
		return 0;
	}

	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: FixClipping - no target shapes found.");
		return 0;
	}

	if (step.fixClipMode == 0) {
		// Shapes mode: fix base geometry of target shapes with no sliders applied
		wxLogMessage("Automation: FixClipping (Shapes mode, strength=%.0f%%)...", step.fixClipStrength * 100.0f);

		std::vector<nifly::Vector3> bodyVerts;
		std::vector<nifly::Triangle> bodyTris;
		project->GetWorkNif()->GetVertsForShape(refShape, bodyVerts);
		refShape->GetTriangles(bodyTris);

		for (auto* shape : shapes) {
			if (project->IsBaseShape(shape))
				continue;

			if (!ClippingFixer::IsEligibleForFix(*project->GetWorkNif(), shape))
				continue;

			std::vector<nifly::Vector3> outfitVerts;
			project->GetWorkNif()->GetVertsForShape(shape, outfitVerts);

			std::vector<nifly::Triangle> outfitTris;
			shape->GetTriangles(outfitTris);

			std::vector<nifly::Vector3> fixedVerts = outfitVerts;
			ClippingFixer::FixClipping(bodyVerts, bodyTris, fixedVerts, outfitTris, options);

			bool changed = false;
			for (size_t i = 0; i < outfitVerts.size(); i++) {
				nifly::Vector3 diff = fixedVerts[i] - outfitVerts[i];
				if (!diff.IsZero(true)) {
					changed = true;
					break;
				}
			}

			if (changed) {
				wxLogMessage("Automation: FixClipping - fixed base geometry for '%s'.", shape->name.get());
				project->GetWorkNif()->SetVertsForShape(shape, fixedVerts);
			}
		}

		outfitStudio->ApplySliders();
	}
	else if (step.fixClipMode == 1) {
		// Sliders mode: fix clipping for each slider individually
		wxLogMessage("Automation: FixClipping (Sliders mode, strength=%.0f%%)...", step.fixClipStrength * 100.0f);

		// Build list of sliders to process
		std::vector<size_t> sliderIndices;
		if (step.fixClipSliderNames.empty()) {
			// Process all non-zap/non-UV sliders that have morph data
			for (size_t i = 0; i < project->SliderCount(); i++) {
				if (project->activeSet[i].bZap || project->activeSet[i].bUV)
					continue;
				sliderIndices.push_back(i);
			}
			wxLogMessage("Automation: FixClipping - processing all %zu non-zap/non-UV sliders.", sliderIndices.size());
		}
		else {
			for (const auto& name : step.fixClipSliderNames) {
				size_t idx;
				if (!project->SliderIndexFromName(name, idx)) {
					wxLogError("Automation: FixClipping - slider '%s' not found.", name);
					return 1;
				}
				sliderIndices.push_back(idx);
			}
		}

		if (sliderIndices.empty()) {
			wxLogWarning("Automation: FixClipping - no sliders to process.");
			return 0;
		}

		// Save current slider values
		std::vector<float> savedValues(project->SliderCount());
		for (size_t i = 0; i < project->SliderCount(); i++)
			savedValues[i] = project->SliderValue(i);

		for (size_t si : sliderIndices) {
			std::string sliderName = project->GetSliderName(si);
			wxLogMessage("Automation: FixClipping - processing slider '%s'...", sliderName);

			// Set all sliders to 0%, then this slider to 100%
			for (size_t i = 0; i < project->SliderCount(); i++)
				outfitStudio->SetSliderValue(i, 0);
			outfitStudio->SetSliderValue(si, 100);
			outfitStudio->ApplySliders();

			for (auto* shape : shapes) {
				if (project->IsBaseShape(shape))
					continue;

				// Check if this shape has morph data for this slider
				TargetDataDiffs* diffSet = project->GetDiffSet(project->activeSet[si], shape);
				if (!diffSet || diffSet->empty())
					continue;

				TargetDataDiffs morphDiffs;
				project->CalcSliderClippingCorrection(shape, sliderName, options.strength, morphDiffs);

				if (!morphDiffs.empty()) {
					wxLogMessage("Automation: FixClipping - updated %zu vertices for '%s' slider '%s'.",
								 morphDiffs.size(), shape->name.get(), sliderName);
					project->UpdateMorphResult(shape, sliderName, morphDiffs);
				}
			}
		}

		// Restore original slider values
		for (size_t i = 0; i < project->SliderCount(); i++)
			outfitStudio->SetSliderValue(i, static_cast<int>(savedValues[i] * 100));
		outfitStudio->ApplySliders();
	}

	return 0;
}

int AutomationDialog::ExecuteStepFixBadBones(const AutomationStep& WXUNUSED(step)) {
	wxLogMessage("Automation: Fixing bad bones...");

	if (!project->CheckForBadBones(false))
		wxLogMessage("Automation: No bad bones found.");

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
		case AutomationStepType::ChangePartitions: return ExecuteStepChangePartitions(step);
		case AutomationStepType::MirrorShape: return ExecuteStepMirrorShape(step);
		case AutomationStepType::RecalcNormals: return ExecuteStepRecalcNormals(step);
		case AutomationStepType::ClearMask: return ExecuteStepClearMask(step);
		case AutomationStepType::LoadMask: return ExecuteStepLoadMask(step);
		case AutomationStepType::SetSliderProperties: return ExecuteStepSetSliderProperties(step);
		case AutomationStepType::SetShaderProperties: return ExecuteStepSetShaderProperties(step);
		case AutomationStepType::SetGeometryProperties: return ExecuteStepSetGeometryProperties(step);
		case AutomationStepType::SetExtraData: return ExecuteStepSetExtraData(step);
		case AutomationStepType::DeleteExtraData: return ExecuteStepDeleteExtraData(step);
		case AutomationStepType::SetTexturePaths: return ExecuteStepSetTexturePaths(step);
		case AutomationStepType::RemoveUnusedNodes: return ExecuteStepRemoveUnusedNodes(step);
		case AutomationStepType::FixClipping: return ExecuteStepFixClipping(step);
		case AutomationStepType::FixBadBones: return ExecuteStepFixBadBones(step);
	}

	return 0;
}

