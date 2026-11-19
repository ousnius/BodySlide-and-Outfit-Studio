/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Automation steps: shader, geometry and texture properties and extra data

#include "AutomationRunner.h"
#include "OutfitProject.h"
#include "ProjectOperations.h"
#include "StudioSession.h"
#include "utils/StringStuff.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include <NifFile.hpp>
#include <wx/log.h>
#include <wx/msgdlg.h>

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

bool ConvertToSubIndexTriShape(NifFile* nif, IStudioSession* session, NiShape*& shape) {
	auto* bsTriShape = dynamic_cast<BSTriShape*>(shape);
	if (!bsTriShape || shape->HasType<BSSubIndexTriShape>())
		return false;

	auto bsSITS = std::make_unique<BSSubIndexTriShape>();
	*static_cast<BSTriShape*>(bsSITS.get()) = *bsTriShape;
	bsSITS->SetDefaultSegments();
	bsSITS->name.get() = bsTriShape->name.get();

	NiShape* newShape = bsSITS.get();
	session->UpdateShapeReference(bsTriShape, newShape);
	nif->GetHeader().ReplaceBlock(nif->GetBlockID(bsTriShape), std::move(bsSITS));
	shape = newShape;
	return true;
}

bool ConvertFromSubIndexTriShape(NifFile* nif, IStudioSession* session, NiShape*& shape) {
	auto* bsTriShape = dynamic_cast<BSTriShape*>(shape);
	if (!bsTriShape || !shape->HasType<BSSubIndexTriShape>())
		return false;

	auto bsTS = std::make_unique<BSTriShape>(*bsTriShape);
	bsTS->name.get() = bsTriShape->name.get();

	NiShape* newShape = bsTS.get();
	session->UpdateShapeReference(bsTriShape, newShape);
	nif->GetHeader().ReplaceBlock(nif->GetBlockID(bsTriShape), std::move(bsTS));
	shape = newShape;
	return true;
}

bool ConvertToDynamicTriShape(NifFile* nif, IStudioSession* session, NiShape*& shape) {
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
	session->UpdateShapeReference(bsTriShape, newShape);
	nif->GetHeader().ReplaceBlock(nif->GetBlockID(bsTriShape), std::move(bsDTS));
	shape = newShape;
	return true;
}

bool ConvertFromDynamicTriShape(NifFile* nif, IStudioSession* session, NiShape*& shape) {
	auto* bsTriShape = dynamic_cast<BSTriShape*>(shape);
	if (!bsTriShape || !shape->HasType<BSDynamicTriShape>())
		return false;

	auto bsTS = std::make_unique<BSTriShape>(*bsTriShape);
	bsTS->name.get() = bsTriShape->name.get();

	bsTS->vertexDesc.SetFlag(VF_VERTEX);
	bsTS->vertexDesc.RemoveFlag(VF_FULLPREC);
	bsTS->CalcDataSizes(nif->GetHeader().GetVersion());

	NiShape* newShape = bsTS.get();
	session->UpdateShapeReference(bsTriShape, newShape);
	nif->GetHeader().ReplaceBlock(nif->GetBlockID(bsTriShape), std::move(bsTS));
	shape = newShape;
	return true;
}

bool ApplyAutomationGeometryProperty(NifFile* nif,
									 IStudioSession* session,
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

		return prop.enabled ? ConvertToSubIndexTriShape(nif, session, shape) : ConvertFromSubIndexTriShape(nif, session, shape);
	}

	if (prop.name == "Dynamic") {
		if (version.Stream() != 100) {
			wxLogWarning("Automation: SetGeometryProperties - Dynamic requires stream 100; skipped '%s'.", shape->name.get());
			return false;
		}

		return prop.enabled ? ConvertToDynamicTriShape(nif, session, shape) : ConvertFromDynamicTriShape(nif, session, shape);
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
} // namespace

int AutomationRunner::ExecuteStepSetShaderProperties(const AutomationStep& step) {
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
			session->MeshFromProj(shape, true);
			wxLogMessage("Automation: SetShaderProperties - updated %d shader properties on '%s'.",
				shapeUpdates,
				shape->name.get());
		}
	}

	if (updatedShapes > 0) {
		session->SetPendingChanges();
		session->Render();
		wxLogMessage("Automation: SetShaderProperties - updated %d shader values on %d shapes.", updatedValues, updatedShapes);
	}
	else {
		wxLogWarning("Automation: SetShaderProperties - found no matching shader properties on target shapes.");
	}

	return 0;
}

int AutomationRunner::ExecuteStepSetGeometryProperties(const AutomationStep& step) {
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
				if (prop.name == propertyName && ApplyAutomationGeometryProperty(nif, session, project, shape, prop, removedSkinning))
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
			session->UpdateAnimationGUI();
		}

		for (auto* shape : changedShapes) {
			if (!shape)
				continue;
			project->SetTextures(shape);
			session->MeshFromProj(shape, true);
		}

		session->SetPendingChanges();
		session->Render();
		wxLogMessage("Automation: SetGeometryProperties - updated %d geometry values on %d shapes.", updatedValues, updatedShapes);
	}
	else {
		wxLogWarning("Automation: SetGeometryProperties - found no applicable geometry properties on target shapes.");
	}

	return 0;
}

int AutomationRunner::ExecuteStepSetExtraData(const AutomationStep& step) {
	NifFile* nif = project->GetWorkNif();
	if (!nif)
		return 0;

	std::string type = TrimString(step.extraDataType.empty() ? "NiStringExtraData" : step.extraDataType);
	std::string name = TrimString(step.extraDataName);
	if (name.empty()) {
		wxLogError("Automation: SetExtraData - extra data name is empty.");
		return 1;
	}
	// A physics link set or deleted on the root by hand is the one the output gets, not one of the loaded files
	if (step.targetMeshes.empty() && ToLower(name) == "hdt skinned mesh physics object")
		project->ForgetCollectedPhysicsLinks();

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
		session->SetPendingChanges();
		session->Render();
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

int AutomationRunner::ExecuteStepDeleteExtraData(const AutomationStep& step) {
	NifFile* nif = project->GetWorkNif();
	if (!nif)
		return 0;

	std::string name = TrimString(step.extraDataName);
	if (name.empty()) {
		wxLogError("Automation: DeleteExtraData - extra data name is empty.");
		return 1;
	}
	// A physics link set or deleted on the root by hand is the one the output gets, not one of the loaded files
	if (step.targetMeshes.empty() && ToLower(name) == "hdt skinned mesh physics object")
		project->ForgetCollectedPhysicsLinks();

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
		session->SetPendingChanges();
		session->Render();
		wxLogMessage("Automation: DeleteExtraData - removed %d extra data blocks.", deletedBlocks);
	}
	else {
		wxLogWarning("Automation: DeleteExtraData - no extra data named '%s' found.", name.c_str());
	}

	return 0;
}

int AutomationRunner::ExecuteStepSetTexturePaths(const AutomationStep& step) {
	if (step.texturePaths.empty()) {
		wxLogWarning("Automation: SetTexturePaths - no texture paths configured.");
		return 0;
	}

	NifFile* nif = project->GetWorkNif();
	if (!nif)
		return 0;

	std::vector<AutomationStep::TexturePath> texturePaths;
	for (const auto& path : step.texturePaths) {
		int index = ResolveAutomationTexturePathIndex(path);
		if (index < 0) {
			wxLogWarning("Automation: SetTexturePaths - texture slot '%s' could not be resolved; skipping.", path.name.c_str());
			continue;
		}

		int namedIndex = path.name.empty() ? -1 : AutomationTexturePathIndexForName(path.name);
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
			resolvedPath.name = AutomationTexturePathNameForIndex(index);
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
			session->MeshFromProj(shape, true);
		}

		session->SetPendingChanges();
		session->Render();
		wxLogMessage("Automation: SetTexturePaths - updated %d texture paths on %d shapes (%d skipped).", updatedValues, updatedShapes, skippedValues);
	}
	else {
		wxLogWarning("Automation: SetTexturePaths - found no applicable texture paths on target shapes (%d skipped).", skippedValues);
	}

	return 0;
}
