/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "Automation.h"

#include "../utils/PlatformUtil.h"

#include <tinyxml2.h>

#include <cassert>
#include <cstdio>

using namespace tinyxml2;

static void SubstituteInString(std::string& str, const std::map<std::string, std::string>& variables);
static void SubstituteInStringVector(std::vector<std::string>& vec, const std::map<std::string, std::string>& variables);

namespace {

using Step = AutomationStep;

void LoadShaderProperties(AutomationStep& step, XMLElement* stepElem) {
	XMLElement* shaderPropsElem = stepElem->FirstChildElement("ShaderProperties");
	if (!shaderPropsElem)
		return;

	XMLElement* propElem = shaderPropsElem->FirstChildElement("Property");
	while (propElem) {
		AutomationStep::ShaderProperty prop;
		const char* name = propElem->Attribute("name");
		if (name) {
			prop.name = name;
			const char* stringValue = propElem->Attribute("stringValue");
			if (stringValue)
				prop.stringValue = stringValue;
			prop.value1 = propElem->FloatAttribute("value1", 0.0f);
			prop.value2 = propElem->FloatAttribute("value2", 0.0f);
			prop.value3 = propElem->FloatAttribute("value3", 0.0f);
			prop.value4 = propElem->FloatAttribute("value4", 1.0f);
			step.shaderProperties.push_back(std::move(prop));
		}
		propElem = propElem->NextSiblingElement("Property");
	}
}

void SaveShaderProperties(const AutomationStep& step, XMLDocument& doc, XMLElement* stepElem) {
	if (step.shaderProperties.empty())
		return;

	XMLElement* shaderPropsElem = doc.NewElement("ShaderProperties");
	stepElem->InsertEndChild(shaderPropsElem);
	for (const auto& prop : step.shaderProperties) {
		if (prop.name.empty())
			continue;
		XMLElement* propElem = doc.NewElement("Property");
		propElem->SetAttribute("name", prop.name.c_str());
		if (!prop.stringValue.empty())
			propElem->SetAttribute("stringValue", prop.stringValue.c_str());
		propElem->SetAttribute("value1", prop.value1);
		propElem->SetAttribute("value2", prop.value2);
		propElem->SetAttribute("value3", prop.value3);
		propElem->SetAttribute("value4", prop.value4);
		shaderPropsElem->InsertEndChild(propElem);
	}
}

void SubstituteShaderProperties(AutomationStep& step, const std::map<std::string, std::string>& vars) {
	for (auto& prop : step.shaderProperties) {
		SubstituteInString(prop.name, vars);
		SubstituteInString(prop.stringValue, vars);
	}
}

void LoadGeometryProperties(AutomationStep& step, XMLElement* stepElem) {
	XMLElement* geometryPropsElem = stepElem->FirstChildElement("GeometryProperties");
	if (!geometryPropsElem)
		return;

	XMLElement* propElem = geometryPropsElem->FirstChildElement("Property");
	while (propElem) {
		AutomationStep::GeometryProperty prop;
		const char* name = propElem->Attribute("name");
		if (name) {
			prop.name = name;
			prop.enabled = propElem->BoolAttribute("enabled", false);
			step.geometryProperties.push_back(std::move(prop));
		}
		propElem = propElem->NextSiblingElement("Property");
	}
}

void SaveGeometryProperties(const AutomationStep& step, XMLDocument& doc, XMLElement* stepElem) {
	if (step.geometryProperties.empty())
		return;

	XMLElement* geometryPropsElem = doc.NewElement("GeometryProperties");
	stepElem->InsertEndChild(geometryPropsElem);
	for (const auto& prop : step.geometryProperties) {
		if (prop.name.empty())
			continue;
		XMLElement* propElem = doc.NewElement("Property");
		propElem->SetAttribute("name", prop.name.c_str());
		propElem->SetAttribute("enabled", prop.enabled);
		geometryPropsElem->InsertEndChild(propElem);
	}
}

void LoadTexturePaths(AutomationStep& step, XMLElement* stepElem) {
	XMLElement* texturePathsElem = stepElem->FirstChildElement("TexturePaths");
	if (!texturePathsElem)
		return;

	XMLElement* pathElem = texturePathsElem->FirstChildElement("Path");
	while (pathElem) {
		AutomationStep::TexturePath path;
		path.index = pathElem->IntAttribute("index", -1);
		const char* name = pathElem->Attribute("name");
		if (name)
			path.name = name;
		const char* value = pathElem->Attribute("value");
		if (value)
			path.path = value;

		if (path.index >= 0 || !path.name.empty())
			step.texturePaths.push_back(std::move(path));
		pathElem = pathElem->NextSiblingElement("Path");
	}
}

void SaveTexturePaths(const AutomationStep& step, XMLDocument& doc, XMLElement* stepElem) {
	if (step.texturePaths.empty())
		return;

	XMLElement* texturePathsElem = doc.NewElement("TexturePaths");
	stepElem->InsertEndChild(texturePathsElem);
	for (const auto& path : step.texturePaths) {
		if (path.index < 0 && path.name.empty())
			continue;
		XMLElement* pathElem = doc.NewElement("Path");
		if (path.index >= 0)
			pathElem->SetAttribute("index", path.index);
		if (!path.name.empty())
			pathElem->SetAttribute("name", path.name.c_str());
		pathElem->SetAttribute("value", path.path.c_str());
		texturePathsElem->InsertEndChild(pathElem);
	}
}

void SubstituteTexturePaths(AutomationStep& step, const std::map<std::string, std::string>& vars) {
	for (auto& path : step.texturePaths) {
		SubstituteInString(path.name, vars);
		SubstituteInString(path.path, vars);
	}
}

// LoadReference and AddProject describe the same project source parameters, but
// expose different subsets of them on their settings pages.
std::vector<AutomationField> ProjectSourceFields(const char* sourceControl,
												 const char* setControl,
												 const char* shapeControl,
												 const char* loadAllControl,
												 const char* mergeSlidersControl,
												 const char* mergeZapsControl,
												 const char* appendSlidersControl) {
	return {
		FieldString("SourceFile", &Step::refSourceFile, sourceControl, AutomationFieldUI::None),
		FieldString("Set", &Step::refSet, setControl, AutomationFieldUI::None),
		FieldString("Shape", &Step::refShape, shapeControl, AutomationFieldUI::None),
		FieldBool("LoadAll", &Step::refLoadAll, false, loadAllControl),
		FieldBool("MergeSliders", &Step::refMergeSliders, true, mergeSlidersControl),
		FieldBool("MergeZaps", &Step::refMergeZaps, true, mergeZapsControl),
		FieldBool("AppendNewSliders", &Step::refAppendNewSliders, false, appendSlidersControl),
	};
}

std::vector<AutomationStepInfo> BuildStepTypes() {
	std::vector<AutomationStepInfo> types;

	// Reserved so the reference returned by add() stays valid while hooks are attached
	types.reserve(AutomationStepTypeCount);

	auto add = [&types](AutomationStepType type, const char* xmlName, const char* category,
						const char* displayName, const char* xrcPage, std::vector<AutomationField> fields = {}) -> AutomationStepInfo& {
		AutomationStepInfo info;
		info.type = type;
		info.xmlName = xmlName;
		info.category = category;
		info.displayName = displayName;
		info.xrcPage = xrcPage;
		info.fields = std::move(fields);
		types.push_back(std::move(info));
		return types.back();
	};

	// Bones
	add(AutomationStepType::AddCustomBone, "AddCustomBone", wxTRANSLATE("Bones"), wxTRANSLATE("Bones: Add Custom Bone"), "pageAddCustomBone",
		{
			FieldString("BoneName", &Step::addBoneName, "txtAddBoneName"),
			FieldString("ParentBone", &Step::addBoneParent, "txtAddBoneParent"),
			FieldFloat("TransX", &Step::addBoneTransX, 0.0f, "txtAddBoneTransX"),
			FieldFloat("TransY", &Step::addBoneTransY, 0.0f, "txtAddBoneTransY"),
			FieldFloat("TransZ", &Step::addBoneTransZ, 0.0f, "txtAddBoneTransZ"),
			FieldFloat("RotX", &Step::addBoneRotX, 0.0f, "txtAddBoneRotX"),
			FieldFloat("RotY", &Step::addBoneRotY, 0.0f, "txtAddBoneRotY"),
			FieldFloat("RotZ", &Step::addBoneRotZ, 0.0f, "txtAddBoneRotZ"),
		});

	add(AutomationStepType::CopyBoneWeights, "CopyBoneWeights", wxTRANSLATE("Bones"), wxTRANSLATE("Bones: Copy Bone Weights"), "pageCopyBoneWeights",
		{
			FieldFloat("ProximityRadius", &Step::weightProximityRadius, 10.0f, "txtWeightRadius", AutomationFieldUI::Text, "%.1f"),
			FieldInt("MaxResults", &Step::weightMaxResults, 10, "txtWeightMaxResults"),
			FieldStringList("BoneList", &Step::weightBoneList, "txtWeightBoneList"),
		});

	add(AutomationStepType::DeleteBones, "DeleteBones", wxTRANSLATE("Bones"), wxTRANSLATE("Bones: Delete Bones"), "pageDeleteBones",
		{
			FieldStringList("BoneNames", &Step::deleteBoneNames, "txtDeleteBoneNames"),
			FieldBool("FromProject", &Step::deleteBoneFromProject, true, "chkDeleteBoneFromProject"),
		});

	add(AutomationStepType::EditBone, "EditBone", wxTRANSLATE("Bones"), wxTRANSLATE("Bones: Edit Custom Bone"), "pageEditBone",
		{
			FieldString("BoneName", &Step::editBoneName, "txtEditBoneName"),
			FieldString("ParentBone", &Step::editBoneParent, "txtEditBoneParent"),
			FieldFloat("TransX", &Step::editBoneTransX, 0.0f, "txtEditBoneTransX"),
			FieldFloat("TransY", &Step::editBoneTransY, 0.0f, "txtEditBoneTransY"),
			FieldFloat("TransZ", &Step::editBoneTransZ, 0.0f, "txtEditBoneTransZ"),
			FieldFloat("RotX", &Step::editBoneRotX, 0.0f, "txtEditBoneRotX"),
			FieldFloat("RotY", &Step::editBoneRotY, 0.0f, "txtEditBoneRotY"),
			FieldFloat("RotZ", &Step::editBoneRotZ, 0.0f, "txtEditBoneRotZ"),
		});

	add(AutomationStepType::RemoveSkinning, "RemoveSkinning", wxTRANSLATE("Bones"), wxTRANSLATE("Bones: Remove Skinning"), "pageRemoveSkinning");

	// Export
	add(AutomationStepType::ExportFile, "ExportFile", wxTRANSLATE("Export"), wxTRANSLATE("Export: File"), "pageExportFile",
		{
			FieldString("FilePath", &Step::exportFilePath, nullptr),
			FieldBool("WithRef", &Step::exportWithRef, true, "chkExportWithRef"),
			FieldBool("UseOriginalPath", &Step::exportUseOriginalPath, false),
			FieldString("Prefix", &Step::exportPrefix, "txtExportPrefix"),
			FieldString("Suffix", &Step::exportSuffix, "txtExportSuffix"),
		});

	add(AutomationStepType::SaveProject, "SaveProject", wxTRANSLATE("Export"), wxTRANSLATE("Export: Save Project"), "pageSaveProject",
		{
			FieldString("DisplayName", &Step::saveName, "txtSaveDisplayName"),
			FieldString("OutputFileName", &Step::saveOutputFileName, "txtSaveOutputFileName"),
			FieldString("OutputDataPath", &Step::saveOutputDataPath, "txtSaveOutputDataPath"),
			FieldString("SliderSetFile", &Step::saveSliderSetFile, "txtSaveSliderSetFile"),
			FieldString("ShapeDataFolder", &Step::saveShapeDataFolder, "txtSaveShapeDataFolder"),
			FieldString("ShapeDataFile", &Step::saveShapeDataFile, "txtSaveShapeDataFile"),
			FieldBool("GenWeights", &Step::saveGenWeights, true, "chkSaveGenWeights"),
			FieldBool("AutoCopyRef", &Step::saveAutoCopyRef, true, "chkSaveAutoCopyRef"),
			FieldBool("CopyRefFromProject", &Step::saveCopyRefFromProject, false, "chkSaveCopyRefFromProject"),
			FieldString("CopyRefShapeName", &Step::saveCopyRefShapeName, "txtSaveCopyRefShapeName"),
			FieldBool("UseOriginal", &Step::saveUseOriginal, false, "chkSaveUseOriginal"),
			FieldString("ReplaceFrom", &Step::saveReplaceFrom, "txtSaveReplaceFrom"),
			FieldString("ReplaceTo", &Step::saveReplaceTo, "txtSaveReplaceTo"),
			FieldString("Suffix", &Step::saveSuffix, "txtSaveSuffix"),
		});

	// Import
	add(AutomationStepType::ImportFile, "ImportFile", wxTRANSLATE("Import"), wxTRANSLATE("Import: File"), "pageImportFile",
		{
			FieldString("FilePath", &Step::importFilePath, nullptr),
			FieldBool("FromFolder", &Step::importFromFolder, false),
			FieldBool("BeforeBatchFile", &Step::importBeforeBatch, false, "chkImportBeforeBatch"),
		});

	add(AutomationStepType::ImportSliderData, "ImportSliderData", wxTRANSLATE("Import"), wxTRANSLATE("Import: Slider Data"), "pageImportSliderData",
		{
			FieldString("SliderDataFile", &Step::sliderDataFile, nullptr),
			FieldBool("FromFolder", &Step::sliderDataFromFolder, false),
			FieldBool("MergeSliders", &Step::sliderMerge, false, "chkSliderMerge"),
			FieldStringList("SliderNames", &Step::sliderNames, "txtSliderNames"),
		});

	// Project
	add(AutomationStepType::AddProject, "AddProject", wxTRANSLATE("Project"), wxTRANSLATE("Project: Add Project"), "pageAddProject",
		ProjectSourceFields(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "chkAddProjAppendSliders"));

	add(AutomationStepType::ClearProject, "ClearProject", wxTRANSLATE("Project"), wxTRANSLATE("Project: Clear Project"), "pageClearProject");
	add(AutomationStepType::ClearReference, "ClearReference", wxTRANSLATE("Project"), wxTRANSLATE("Project: Clear Reference"), "pageClearReference");

	add(AutomationStepType::LoadReference, "LoadReference", wxTRANSLATE("Project"), wxTRANSLATE("Project: Load Reference"), "pageLoadReference",
		ProjectSourceFields(nullptr, nullptr, nullptr, "chkRefLoadAll", "chkRefMergeSliders", "chkRefMergeZaps", "chkRefAppendNewSliders"));

	add(AutomationStepType::SetBaseShape, "SetBaseShape", wxTRANSLATE("Project"), wxTRANSLATE("Project: Set Base Shape"), "pageSetBaseShape");

	add(AutomationStepType::SetReferenceShape, "SetReferenceShape", wxTRANSLATE("Project"), wxTRANSLATE("Project: Set Reference Shape"), "pageSetReferenceShape",
		{
			FieldString("ShapeName", &Step::setRefShapeName, "txtSetRefShapeName"),
			FieldBool("UnsetReference", &Step::setRefUnset, false, "chkSetRefUnset"),
		});

	// Shapes
	add(AutomationStepType::ApplyPose, "ApplyPose", wxTRANSLATE("Shapes"), wxTRANSLATE("Shapes: Apply Pose"), "pageApplyPose",
		{
			FieldString("PoseName", &Step::poseName, "txtPoseName"),
		});

	add(AutomationStepType::DeleteShape, "DeleteShape", wxTRANSLATE("Shapes"), wxTRANSLATE("Shapes: Delete Shape"), "pageDeleteShape");

	add(AutomationStepType::DuplicateShape, "DuplicateShape", wxTRANSLATE("Shapes"), wxTRANSLATE("Shapes: Duplicate Shape"), "pageDuplicateShape",
		{
			FieldString("NewName", &Step::dupNewName, "txtDupNewName"),
		});

	add(AutomationStepType::ChangePartitions, "ChangePartitions", wxTRANSLATE("Shapes"), wxTRANSLATE("Shapes: Change Partitions"), "pageChangePartitions",
		{
			FieldString("SourcePartition", &Step::partitionSource, "choicePartitionSource", AutomationFieldUI::ChoiceString),
			FieldString("DestinationPartition", &Step::partitionDestination, "choicePartitionDestination", AutomationFieldUI::ChoiceString),
		});

	add(AutomationStepType::FixBadBones, "FixBadBones", wxTRANSLATE("Shapes"), wxTRANSLATE("Shapes: Fix Bad Bones"), "pageFixBadBones");

	add(AutomationStepType::FixClipping, "FixClipping", wxTRANSLATE("Shapes"), wxTRANSLATE("Shapes: Fix Clipping"), "pageFixClipping",
		{
			FieldInt("Mode", &Step::fixClipMode, 0, "choiceFixClipMode", AutomationFieldUI::ChoiceIndex),
			FieldFloat("Strength", &Step::fixClipStrength, 0.5f, "txtFixClipStrength", AutomationFieldUI::TextPercent),
			FieldStringList("SliderNames", &Step::fixClipSliderNames, "txtFixClipSliderNames"),
		});

	add(AutomationStepType::InvertUVs, "InvertUVs", wxTRANSLATE("Shapes"), wxTRANSLATE("Shapes: Invert UVs"), "pageInvertUVs",
		{
			FieldBool("InvertU", &Step::invertU, false, "chkInvertU"),
			FieldBool("InvertV", &Step::invertV, false, "chkInvertV"),
		});

	add(AutomationStepType::MirrorShape, "MirrorShape", wxTRANSLATE("Shapes"), wxTRANSLATE("Shapes: Mirror Shape"), "pageMirrorShape",
		{
			FieldBool("MirrorX", &Step::mirrorX, true, "chkMirrorX"),
			FieldBool("MirrorY", &Step::mirrorY, false, "chkMirrorY"),
			FieldBool("MirrorZ", &Step::mirrorZ, false, "chkMirrorZ"),
			FieldBool("SwapBonesX", &Step::mirrorSwapBonesX, false, "chkMirrorSwapBonesX"),
		});

	add(AutomationStepType::RecalcNormals, "RecalcNormals", wxTRANSLATE("Shapes"), wxTRANSLATE("Shapes: Recalculate Normals"), "pageRecalcNormals",
		{
			FieldBool("Force", &Step::normalsForce, true, "chkRecalcNormalsForce"),
			FieldInt("SeamSmooth", &Step::normalsSeamSmooth, -1, "choiceRecalcNormalsSeam", AutomationFieldUI::ChoiceTriState),
			FieldFloat("SeamAngle", &Step::normalsSeamAngle, -1.0f, "txtRecalcNormalsAngle", AutomationFieldUI::TextOptional, "%0.2f"),
			FieldInt("LockNormals", &Step::normalsLock, -1, "choiceRecalcNormalsLock", AutomationFieldUI::ChoiceTriState),
		});

	add(AutomationStepType::RefineMesh, "RefineMesh", wxTRANSLATE("Shapes"), wxTRANSLATE("Shapes: Refine Mesh"), "pageRefineMesh");

	add(AutomationStepType::RenameShape, "RenameShape", wxTRANSLATE("Shapes"), wxTRANSLATE("Shapes: Rename Shape"), "pageRenameShape",
		{
			FieldString("OldName", &Step::renameOldName, "txtRenameOldName"),
			FieldString("NewName", &Step::renameNewName, "txtRenameNewName"),
		});

	add(AutomationStepType::ResetTransforms, "ResetTransforms", wxTRANSLATE("Shapes"), wxTRANSLATE("Shapes: Reset Transforms"), "pageResetTransforms");

	add(AutomationStepType::TransformShape, "TransformShape", wxTRANSLATE("Shapes"), wxTRANSLATE("Shapes: Transform Shape"), "pageTransformShape",
		{
			FieldFloat("MoveX", &Step::moveX, 0.0f, "txtMoveX"),
			FieldFloat("MoveY", &Step::moveY, 0.0f, "txtMoveY"),
			FieldFloat("MoveZ", &Step::moveZ, 0.0f, "txtMoveZ"),
			FieldFloat("RotateX", &Step::rotateX, 0.0f, "txtRotateX"),
			FieldFloat("RotateY", &Step::rotateY, 0.0f, "txtRotateY"),
			FieldFloat("RotateZ", &Step::rotateZ, 0.0f, "txtRotateZ"),
			FieldFloat("ScaleX", &Step::scaleX, 1.0f, "txtScaleX"),
			FieldFloat("ScaleY", &Step::scaleY, 1.0f, "txtScaleY"),
			FieldFloat("ScaleZ", &Step::scaleZ, 1.0f, "txtScaleZ"),
			FieldFloat("InflateX", &Step::inflateX, 0.0f, "txtInflateX"),
			FieldFloat("InflateY", &Step::inflateY, 0.0f, "txtInflateY"),
			FieldFloat("InflateZ", &Step::inflateZ, 0.0f, "txtInflateZ"),
		});

	auto& geomProps = add(AutomationStepType::SetGeometryProperties, "SetGeometryProperties", wxTRANSLATE("Shapes"),
		wxTRANSLATE("Shapes: Set Geometry Properties"), "pageSetGeometryProperties");
	geomProps.loadExtra = &LoadGeometryProperties;
	geomProps.saveExtra = &SaveGeometryProperties;

	add(AutomationStepType::SetExtraData, "SetExtraData", wxTRANSLATE("Shapes"), wxTRANSLATE("Shapes: Set Extra Data"), "pageSetExtraData",
		{
			FieldString("BlockType", &Step::extraDataType, nullptr),
			FieldString("Name", &Step::extraDataName, "txtExtraDataName"),
			FieldString("Value", &Step::extraDataValue, "txtExtraDataValue"),
		});

	add(AutomationStepType::DeleteExtraData, "DeleteExtraData", wxTRANSLATE("Shapes"), wxTRANSLATE("Shapes: Delete Extra Data"), "pageDeleteExtraData",
		{
			FieldString("Name", &Step::extraDataName, "txtDeleteExtraDataName"),
		});

	// Sliders
	add(AutomationStepType::ConformSliders, "ConformSliders", wxTRANSLATE("Sliders"), wxTRANSLATE("Sliders: Conform Sliders"), "pageConformSliders",
		{
			FieldFloat("ProximityRadius", &Step::conformProximityRadius, 10.0f, "txtConformRadius", AutomationFieldUI::Text, "%.1f"),
			FieldInt("MaxResults", &Step::conformMaxResults, 10, "txtConformMaxResults"),
			FieldBool("SmoothResults", &Step::conformSmoothResults, false, "chkConformSmoothResults"),
			FieldInt("SmoothIterations", &Step::conformSmoothIterations, 2, "txtConformSmoothIterations"),
			FieldFloat("SmoothStrength", &Step::conformSmoothStrength, 0.5f, "txtConformSmoothStrength", AutomationFieldUI::Text, "%.2f"),
			FieldBool("NoSqueeze", &Step::conformNoSqueeze, false, "chkConformNoSqueeze"),
			FieldBool("SolidMode", &Step::conformSolidMode, false, "chkConformSolidMode"),
			FieldBool("AxisX", &Step::conformAxisX, true, "chkConformAxisX"),
			FieldBool("AxisY", &Step::conformAxisY, true, "chkConformAxisY"),
			FieldBool("AxisZ", &Step::conformAxisZ, true, "chkConformAxisZ"),
			FieldBool("FixClipping", &Step::conformFixClipping, false, "chkConformFixClipping"),
			FieldFloat("FixClippingStrength", &Step::conformFixClippingStrength, 0.5f, "txtConformFixClipStrength", AutomationFieldUI::TextPercent),
			FieldStringList("SliderNames", &Step::conformSliderNames, "txtConformSliderNames"),
		});

	add(AutomationStepType::DeleteSlider, "DeleteSlider", wxTRANSLATE("Sliders"), wxTRANSLATE("Sliders: Delete Slider"), "pageDeleteSlider",
		{
			FieldStringList("SliderName", &Step::deleteSliderNames, "txtDeleteSliderName"),
			FieldBool("Regex", &Step::deleteSliderRegex, false, "chkDeleteSliderRegex"),
		});

	add(AutomationStepType::SetSliderValues, "SetSliderValues", wxTRANSLATE("Sliders"), wxTRANSLATE("Sliders: Set Slider Values"), "pageSetSliderValues",
		{
			FieldStringList("SliderNames", &Step::setSliderNames, "txtSetSliderNames"),
			FieldFloat("Value", &Step::setSliderValue, 1.0f, "txtSetSliderValue", AutomationFieldUI::TextPercent),
		});

	add(AutomationStepType::SetSliderProperties, "SetSliderProperties", wxTRANSLATE("Sliders"), wxTRANSLATE("Sliders: Set Slider Properties"), "pageSetSliderProperties",
		{
			FieldStringList("SliderNames", &Step::sliderPropNames, "txtSliderPropNames"),
			FieldInt("Zap", &Step::sliderPropZap, -1, "choiceSliderPropZap", AutomationFieldUI::ChoiceTriState),
			FieldInt("Hidden", &Step::sliderPropHidden, -1, "choiceSliderPropHidden", AutomationFieldUI::ChoiceTriState),
			FieldInt("DefaultLo", &Step::sliderPropDefaultLo, -1),
			FieldInt("DefaultHi", &Step::sliderPropDefaultHi, -1),
		});

	// Shaders
	auto& shaderProps = add(AutomationStepType::SetShaderProperties, "SetShaderProperties", wxTRANSLATE("Shaders"),
		wxTRANSLATE("Shaders: Set Shader Properties"), "pageSetShaderProperties");
	shaderProps.loadExtra = &LoadShaderProperties;
	shaderProps.saveExtra = &SaveShaderProperties;
	shaderProps.substituteExtra = &SubstituteShaderProperties;

	auto& texPaths = add(AutomationStepType::SetTexturePaths, "SetTexturePaths", wxTRANSLATE("Shaders"),
		wxTRANSLATE("Shaders: Set Texture Paths"), "pageSetTexturePaths");
	texPaths.loadExtra = &LoadTexturePaths;
	texPaths.saveExtra = &SaveTexturePaths;
	texPaths.substituteExtra = &SubstituteTexturePaths;

	// Masks
	add(AutomationStepType::ClearMask, "ClearMask", wxTRANSLATE("Masks"), wxTRANSLATE("Masks: Clear Mask"), "pageClearMask");

	add(AutomationStepType::LoadMask, "LoadMask", wxTRANSLATE("Masks"), wxTRANSLATE("Masks: Load Mask"), "pageLoadMask",
		{
			FieldString("MaskFile", &Step::loadMaskFile, nullptr),
			FieldString("MaskName", &Step::loadMaskName, nullptr),
		});

	// Nodes
	add(AutomationStepType::RemoveUnusedNodes, "RemoveUnusedNodes", wxTRANSLATE("Nodes"), wxTRANSLATE("Nodes: Remove Unused Nodes"), "pageRemoveUnusedNodes");

	// A missing entry would silently drop the type's parameters and settings page
	assert(types.size() == AutomationStepTypeCount && "every AutomationStepType needs a table entry");

	return types;
}

} // namespace

const std::vector<AutomationStepInfo>& GetAutomationStepTypes() {
	static const std::vector<AutomationStepInfo> types = BuildStepTypes();
	return types;
}

const AutomationStepInfo& GetAutomationStepInfo(AutomationStepType type) {
	const auto& types = GetAutomationStepTypes();
	for (const auto& info : types)
		if (info.type == type)
			return info;

	// Every enum value has a table entry; fall back to the first one so callers
	// never see a dangling reference if that ever stops being true.
	return types.front();
}

std::string AutomationStepTypeToString(AutomationStepType type) {
	return GetAutomationStepInfo(type).xmlName;
}

AutomationStepType AutomationStepTypeFromString(const std::string& str) {
	for (const auto& info : GetAutomationStepTypes())
		if (str == info.xmlName)
			return info.type;

	return AutomationStepType::LoadReference;
}

std::string AutomationBatchModeToString(AutomationBatchMode mode) {
	switch (mode) {
		case AutomationBatchMode::None: return "None";
		case AutomationBatchMode::FolderScan: return "FolderScan";
		case AutomationBatchMode::SliderSets: return "SliderSets";
		default: return "None";
	}
}

AutomationBatchMode AutomationBatchModeFromString(const std::string& str) {
	if (str == "FolderScan") return AutomationBatchMode::FolderScan;
	if (str == "SliderSets") return AutomationBatchMode::SliderSets;
	return AutomationBatchMode::None;
}

static const char* GetChildText(XMLElement* parent, const char* childName) {
	XMLElement* child = parent->FirstChildElement(childName);
	if (child && child->GetText())
		return child->GetText();
	return nullptr;
}

static bool GetChildBool(XMLElement* parent, const char* childName, bool defaultVal) {
	const char* text = GetChildText(parent, childName);
	if (!text)
		return defaultVal;
	std::string s(text);
	return (s == "true" || s == "1");
}

static float GetChildFloat(XMLElement* parent, const char* childName, float defaultVal) {
	const char* text = GetChildText(parent, childName);
	if (!text)
		return defaultVal;
	try {
		return std::stof(text);
	}
	catch (...) {
		return defaultVal;
	}
}

static int GetChildInt(XMLElement* parent, const char* childName, int defaultVal) {
	const char* text = GetChildText(parent, childName);
	if (!text)
		return defaultVal;
	try {
		return std::stoi(text);
	}
	catch (...) {
		return defaultVal;
	}
}

static void SetChildText(XMLDocument& doc, XMLElement* parent, const char* childName, const std::string& value) {
	if (value.empty())
		return;
	XMLElement* child = doc.NewElement(childName);
	child->SetText(value.c_str());
	parent->InsertEndChild(child);
}

static void SetChildBool(XMLDocument& doc, XMLElement* parent, const char* childName, bool value, bool defaultVal) {
	if (value == defaultVal)
		return;
	XMLElement* child = doc.NewElement(childName);
	child->SetText(value ? "true" : "false");
	parent->InsertEndChild(child);
}

static void SetChildFloat(XMLDocument& doc, XMLElement* parent, const char* childName, float value, float defaultVal) {
	if (value == defaultVal)
		return;
	XMLElement* child = doc.NewElement(childName);
	child->SetText(std::to_string(value).c_str());
	parent->InsertEndChild(child);
}

static void SetChildInt(XMLDocument& doc, XMLElement* parent, const char* childName, int value, int defaultVal) {
	if (value == defaultVal)
		return;
	XMLElement* child = doc.NewElement(childName);
	child->SetText(std::to_string(value).c_str());
	parent->InsertEndChild(child);
}

int AutomationScript::Load(const std::string& fileName) {
	XMLDocument doc;

	FILE* fp = nullptr;
#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(fileName);
	int err = _wfopen_s(&fp, winFileName.c_str(), L"rb");
	if (err || !fp)
		return 1;
#else
	fp = fopen(fileName.c_str(), "rb");
	if (!fp)
		return 1;
#endif

	XMLError error = doc.LoadFile(fp);
	fclose(fp);

	if (error != XML_SUCCESS)
		return 2;

	XMLElement* root = doc.FirstChildElement("AutomationScript");
	if (!root)
		return 3;

	steps.clear();
	variables.clear();

	// Reset batch settings
	batchMode = AutomationBatchMode::None;
	batchFolder.clear();
	batchExtension = DefaultAutomationBatchExtension;
	batchSubdirectories = false;
	batchFileFilter.clear();
	batchFileFilterRegex = false;
	batchSliderSetFilter.clear();
	batchSliderSetFilterRegex = false;

	// Load variables
	XMLElement* varsElem = root->FirstChildElement("Variables");
	if (varsElem) {
		XMLElement* varElem = varsElem->FirstChildElement("Var");
		while (varElem) {
			const char* key = varElem->Attribute("key");
			const char* val = varElem->GetText();
			if (key)
				variables[key] = val ? val : "";
			varElem = varElem->NextSiblingElement("Var");
		}
	}

	// Load batch settings
	XMLElement* batchElem = root->FirstChildElement("Batch");
	if (batchElem) {
		if (batchElem->Attribute("mode"))
			batchMode = AutomationBatchModeFromString(batchElem->Attribute("mode"));
		const char* folder = GetChildText(batchElem, "Folder");
		if (folder)
			batchFolder = folder;
		const char* ext = GetChildText(batchElem, "Extension");
		if (ext)
			batchExtension = ext;
		batchSubdirectories = GetChildBool(batchElem, "Subdirectories", false);
		const char* filter = GetChildText(batchElem, "FileFilter");
		if (filter)
			batchFileFilter = filter;
		batchFileFilterRegex = GetChildBool(batchElem, "FileFilterRegex", false);
		const char* ssFilter = GetChildText(batchElem, "SliderSetFilter");
		if (ssFilter)
			batchSliderSetFilter = ssFilter;
		batchSliderSetFilterRegex = GetChildBool(batchElem, "SliderSetFilterRegex", false);
	}

	XMLElement* stepElem = root->FirstChildElement("Step");
	while (stepElem) {
		AutomationStep step;

		if (stepElem->Attribute("type"))
			step.type = AutomationStepTypeFromString(stepElem->Attribute("type"));

		step.active = stepElem->BoolAttribute("active", true);

		const char* noteText = GetChildText(stepElem, "Note");
		if (noteText)
			step.note = noteText;

		const char* meshesText = GetChildText(stepElem, "TargetMeshes");
		if (meshesText)
			step.targetMeshes = SplitCommaSeparated(meshesText);

		step.targetRegex = GetChildBool(stepElem, "TargetRegex", false);

		// Type specific parameters come from the step type's field table
		const AutomationStepInfo& info = GetAutomationStepInfo(step.type);
		for (const AutomationField& field : info.fields) {
			switch (field.kind) {
				case AutomationFieldKind::Bool:
					step.*field.member.asBool = GetChildBool(stepElem, field.xmlName, field.defaultValue.asBool);
					break;
				case AutomationFieldKind::Int:
					step.*field.member.asInt = GetChildInt(stepElem, field.xmlName, field.defaultValue.asInt);
					break;
				case AutomationFieldKind::Float:
					step.*field.member.asFloat = GetChildFloat(stepElem, field.xmlName, field.defaultValue.asFloat);
					break;
				case AutomationFieldKind::String: {
					const char* text = GetChildText(stepElem, field.xmlName);
					if (text)
						step.*field.member.asString = text;
					break;
				}
				case AutomationFieldKind::StringList: {
					const char* text = GetChildText(stepElem, field.xmlName);
					if (text)
						step.*field.member.asStringList = SplitCommaSeparated(text);
					break;
				}
			}
		}

		if (info.loadExtra)
			info.loadExtra(step, stepElem);

		steps.push_back(std::move(step));
		stepElem = stepElem->NextSiblingElement("Step");
	}

	return 0;
}

int AutomationScript::Save(const std::string& fileName) {
	XMLDocument doc;
	doc.InsertFirstChild(doc.NewDeclaration());

	XMLElement* root = doc.NewElement("AutomationScript");
	root->SetAttribute("version", 1);
	doc.InsertEndChild(root);

	// Save variables
	if (!variables.empty()) {
		XMLElement* varsElem = doc.NewElement("Variables");
		root->InsertEndChild(varsElem);
		for (const auto& [key, value] : variables) {
			XMLElement* varElem = doc.NewElement("Var");
			varElem->SetAttribute("key", key.c_str());
			if (!value.empty())
				varElem->SetText(value.c_str());
			varsElem->InsertEndChild(varElem);
		}
	}

	// Save batch settings
	if (batchMode != AutomationBatchMode::None) {
		XMLElement* batchElem = doc.NewElement("Batch");
		batchElem->SetAttribute("mode", AutomationBatchModeToString(batchMode).c_str());
		root->InsertEndChild(batchElem);
		
		if (batchMode == AutomationBatchMode::FolderScan) {
			SetChildText(doc, batchElem, "Folder", batchFolder);
			SetChildText(doc, batchElem, "Extension", batchExtension);
			SetChildBool(doc, batchElem, "Subdirectories", batchSubdirectories, false);
			SetChildText(doc, batchElem, "FileFilter", batchFileFilter);
			SetChildBool(doc, batchElem, "FileFilterRegex", batchFileFilterRegex, false);
		}
		else if (batchMode == AutomationBatchMode::SliderSets) {
			SetChildText(doc, batchElem, "SliderSetFilter", batchSliderSetFilter);
			SetChildBool(doc, batchElem, "SliderSetFilterRegex", batchSliderSetFilterRegex, false);
		}
	}

	for (const auto& step : steps) {
		XMLElement* stepElem = doc.NewElement("Step");
		stepElem->SetAttribute("type", AutomationStepTypeToString(step.type).c_str());
		stepElem->SetAttribute("active", step.active);
		root->InsertEndChild(stepElem);

		SetChildText(doc, stepElem, "Note", step.note);

		if (!step.targetMeshes.empty())
			SetChildText(doc, stepElem, "TargetMeshes", JoinStrings(step.targetMeshes, ", "));

		SetChildBool(doc, stepElem, "TargetRegex", step.targetRegex, false);

		// Type specific parameters come from the step type's field table
		const AutomationStepInfo& info = GetAutomationStepInfo(step.type);
		for (const AutomationField& field : info.fields) {
			switch (field.kind) {
				case AutomationFieldKind::Bool:
					SetChildBool(doc, stepElem, field.xmlName, step.*field.member.asBool, field.defaultValue.asBool);
					break;
				case AutomationFieldKind::Int:
					SetChildInt(doc, stepElem, field.xmlName, step.*field.member.asInt, field.defaultValue.asInt);
					break;
				case AutomationFieldKind::Float:
					SetChildFloat(doc, stepElem, field.xmlName, step.*field.member.asFloat, field.defaultValue.asFloat);
					break;
				case AutomationFieldKind::String:
					SetChildText(doc, stepElem, field.xmlName, step.*field.member.asString);
					break;
				case AutomationFieldKind::StringList: {
					const std::vector<std::string>& values = step.*field.member.asStringList;
					if (!values.empty())
						SetChildText(doc, stepElem, field.xmlName, JoinStrings(values, ", "));
					break;
				}
			}
		}

		if (info.saveExtra)
			info.saveExtra(step, doc, stepElem);
	}

	FILE* fp = nullptr;
#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(fileName);
	int err = _wfopen_s(&fp, winFileName.c_str(), L"w");
	if (err || !fp)
		return 1;
#else
	fp = fopen(fileName.c_str(), "w");
	if (!fp)
		return 1;
#endif

	XMLError error = doc.SaveFile(fp);
	fclose(fp);

	return (error != XML_SUCCESS) ? 2 : 0;
}

static void SubstituteInString(std::string& str, const std::map<std::string, std::string>& variables) {
	if (str.empty())
		return;

	for (const auto& [key, value] : variables) {
		std::string placeholder = "{{" + key + "}}";
		size_t pos = 0;
		while ((pos = str.find(placeholder, pos)) != std::string::npos) {
			str.replace(pos, placeholder.length(), value);
			pos += value.length();
		}
	}
}

static void SubstituteInStringVector(std::vector<std::string>& vec, const std::map<std::string, std::string>& variables) {
	std::vector<std::string> result;
	for (auto& s : vec) {
		SubstituteInString(s, variables);
		auto parts = SplitCommaSeparated(s);
		for (auto& p : parts)
			result.push_back(std::move(p));
	}
	vec = std::move(result);
}

void AutomationScript::SubstitutePlaceholders(const std::map<std::string, std::string>& vars) {
	for (auto& step : steps) {
		if (!step.active)
			continue;

		SubstituteInString(step.note, vars);
		SubstituteInStringVector(step.targetMeshes, vars);

		// Every text parameter of the step type takes placeholders
		const AutomationStepInfo& info = GetAutomationStepInfo(step.type);
		for (const AutomationField& field : info.fields) {
			if (field.kind == AutomationFieldKind::String)
				SubstituteInString(step.*field.member.asString, vars);
			else if (field.kind == AutomationFieldKind::StringList)
				SubstituteInStringVector(step.*field.member.asStringList, vars);
		}

		if (info.substituteExtra)
			info.substituteExtra(step, vars);
	}
}
