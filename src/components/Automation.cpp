/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "Automation.h"

#include "../utils/PlatformUtil.h"

#include <regex>

using namespace tinyxml2;

std::string AutomationStepTypeToString(AutomationStepType type) {
	switch (type) {
		case AutomationStepType::AddCustomBone: return "AddCustomBone";
		case AutomationStepType::CopyBoneWeights: return "CopyBoneWeights";
		case AutomationStepType::DeleteBones: return "DeleteBones";
		case AutomationStepType::EditBone: return "EditBone";
		case AutomationStepType::RemoveSkinning: return "RemoveSkinning";
		case AutomationStepType::ExportFile: return "ExportFile";
		case AutomationStepType::SaveProject: return "SaveProject";
		case AutomationStepType::ImportFile: return "ImportFile";
		case AutomationStepType::ImportSliderData: return "ImportSliderData";
		case AutomationStepType::AddProject: return "AddProject";
		case AutomationStepType::ClearProject: return "ClearProject";
		case AutomationStepType::ClearReference: return "ClearReference";
		case AutomationStepType::LoadReference: return "LoadReference";
		case AutomationStepType::SetBaseShape: return "SetBaseShape";
		case AutomationStepType::SetReferenceShape: return "SetReferenceShape";
		case AutomationStepType::ApplyPose: return "ApplyPose";
		case AutomationStepType::DeleteShape: return "DeleteShape";
		case AutomationStepType::DuplicateShape: return "DuplicateShape";
		case AutomationStepType::InvertUVs: return "InvertUVs";
		case AutomationStepType::MirrorShape: return "MirrorShape";
		case AutomationStepType::RefineMesh: return "RefineMesh";
		case AutomationStepType::RenameShape: return "RenameShape";
		case AutomationStepType::ResetTransforms: return "ResetTransforms";
		case AutomationStepType::TransformShape: return "TransformShape";
		case AutomationStepType::ConformSliders: return "ConformSliders";
		case AutomationStepType::DeleteSlider: return "DeleteSlider";
		case AutomationStepType::SetSliderValues: return "SetSliderValues";
		case AutomationStepType::LoadMask: return "LoadMask";
		case AutomationStepType::RemoveUnusedNodes: return "RemoveUnusedNodes";
		default: return "LoadReference";
	}
}

AutomationStepType AutomationStepTypeFromString(const std::string& str) {
	if (str == "AddCustomBone") return AutomationStepType::AddCustomBone;
	if (str == "CopyBoneWeights") return AutomationStepType::CopyBoneWeights;
	if (str == "DeleteBones") return AutomationStepType::DeleteBones;
	if (str == "EditBone") return AutomationStepType::EditBone;
	if (str == "RemoveSkinning") return AutomationStepType::RemoveSkinning;
	if (str == "ExportFile") return AutomationStepType::ExportFile;
	if (str == "SaveProject") return AutomationStepType::SaveProject;
	if (str == "ImportFile") return AutomationStepType::ImportFile;
	if (str == "ImportSliderData") return AutomationStepType::ImportSliderData;
	if (str == "AddProject") return AutomationStepType::AddProject;
	if (str == "ClearProject") return AutomationStepType::ClearProject;
	if (str == "ClearReference") return AutomationStepType::ClearReference;
	if (str == "LoadReference") return AutomationStepType::LoadReference;
	if (str == "SetBaseShape") return AutomationStepType::SetBaseShape;
	if (str == "SetReferenceShape") return AutomationStepType::SetReferenceShape;
	if (str == "ApplyPose") return AutomationStepType::ApplyPose;
	if (str == "DeleteShape") return AutomationStepType::DeleteShape;
	if (str == "DuplicateShape") return AutomationStepType::DuplicateShape;
	if (str == "InvertUVs") return AutomationStepType::InvertUVs;
	if (str == "MirrorShape") return AutomationStepType::MirrorShape;
	if (str == "RefineMesh") return AutomationStepType::RefineMesh;
	if (str == "RenameShape") return AutomationStepType::RenameShape;
	if (str == "ResetTransforms") return AutomationStepType::ResetTransforms;
	if (str == "TransformShape") return AutomationStepType::TransformShape;
	if (str == "ConformSliders") return AutomationStepType::ConformSliders;
	if (str == "DeleteSlider") return AutomationStepType::DeleteSlider;
	if (str == "SetSliderValues") return AutomationStepType::SetSliderValues;
	if (str == "LoadMask") return AutomationStepType::LoadMask;
	if (str == "RemoveUnusedNodes") return AutomationStepType::RemoveUnusedNodes;
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
	batchExtension.clear();
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

		switch (step.type) {
			case AutomationStepType::ClearProject:
				// No additional params
				break;

			case AutomationStepType::LoadReference:
			case AutomationStepType::AddProject: {
				const char* src = GetChildText(stepElem, "SourceFile");
				if (src)
					step.refSourceFile = src;
				const char* set = GetChildText(stepElem, "Set");
				if (set)
					step.refSet = set;
				const char* shape = GetChildText(stepElem, "Shape");
				if (shape)
					step.refShape = shape;
				step.refLoadAll = GetChildBool(stepElem, "LoadAll", false);
				step.refMergeSliders = GetChildBool(stepElem, "MergeSliders", true);
				step.refMergeZaps = GetChildBool(stepElem, "MergeZaps", true);
				step.refAppendNewSliders = GetChildBool(stepElem, "AppendNewSliders", false);
				break;
			}
			case AutomationStepType::ConformSliders: {
				step.conformProximityRadius = GetChildFloat(stepElem, "ProximityRadius", 10.0f);
				step.conformMaxResults = GetChildInt(stepElem, "MaxResults", 10);
				step.conformNoSqueeze = GetChildBool(stepElem, "NoSqueeze", false);
				step.conformSolidMode = GetChildBool(stepElem, "SolidMode", false);
				step.conformAxisX = GetChildBool(stepElem, "AxisX", true);
				step.conformAxisY = GetChildBool(stepElem, "AxisY", true);
				step.conformAxisZ = GetChildBool(stepElem, "AxisZ", true);
				const char* csn = GetChildText(stepElem, "SliderNames");
				if (csn)
					step.conformSliderNames = SplitCommaSeparated(csn);
				break;
			}
			case AutomationStepType::CopyBoneWeights: {
				step.weightProximityRadius = GetChildFloat(stepElem, "ProximityRadius", 10.0f);
				step.weightMaxResults = GetChildInt(stepElem, "MaxResults", 10);
				const char* bones = GetChildText(stepElem, "BoneList");
				if (bones)
					step.weightBoneList = SplitCommaSeparated(bones);
				break;
			}
			case AutomationStepType::ImportSliderData: {
				const char* sdf = GetChildText(stepElem, "SliderDataFile");
				if (sdf)
					step.sliderDataFile = sdf;
				step.sliderDataFromFolder = GetChildBool(stepElem, "FromFolder", false);
				step.sliderMerge = GetChildBool(stepElem, "MergeSliders", false);
				const char* sn = GetChildText(stepElem, "SliderNames");
				if (sn)
					step.sliderNames = SplitCommaSeparated(sn);
				break;
			}
			case AutomationStepType::SetSliderValues: {
				const char* svn = GetChildText(stepElem, "SliderNames");
				if (svn)
					step.setSliderNames = SplitCommaSeparated(svn);
				step.setSliderValue = GetChildFloat(stepElem, "Value", 1.0f);
				break;
			}
			case AutomationStepType::ImportFile: {
				const char* filePath = GetChildText(stepElem, "FilePath");
				if (filePath)
					step.importFilePath = filePath;
				step.importFromFolder = GetChildBool(stepElem, "FromFolder", false);
				break;
			}
			case AutomationStepType::DeleteShape:
				// No additional params (uses target meshes)
				break;
			case AutomationStepType::RenameShape: {
				const char* on = GetChildText(stepElem, "OldName");
				if (on)
					step.renameOldName = on;
				const char* nn = GetChildText(stepElem, "NewName");
				if (nn)
					step.renameNewName = nn;
				break;
			}
			case AutomationStepType::DeleteSlider: {
				const char* sn = GetChildText(stepElem, "SliderName");
				if (sn)
					step.deleteSliderName = sn;
				const char* re = GetChildText(stepElem, "Regex");
				if (re)
					step.deleteSliderRegex = (std::string(re) == "true");
				break;
			}
			case AutomationStepType::SetReferenceShape: {
				const char* sn = GetChildText(stepElem, "ShapeName");
				if (sn)
					step.setRefShapeName = sn;
				break;
			}
			case AutomationStepType::RefineMesh:
			case AutomationStepType::RemoveSkinning:
				// No additional params (uses target meshes)
				break;
			case AutomationStepType::TransformShape: {
				step.moveX = GetChildFloat(stepElem, "MoveX", 0.0f);
				step.moveY = GetChildFloat(stepElem, "MoveY", 0.0f);
				step.moveZ = GetChildFloat(stepElem, "MoveZ", 0.0f);
				step.rotateX = GetChildFloat(stepElem, "RotateX", 0.0f);
				step.rotateY = GetChildFloat(stepElem, "RotateY", 0.0f);
				step.rotateZ = GetChildFloat(stepElem, "RotateZ", 0.0f);
				step.scaleX = GetChildFloat(stepElem, "ScaleX", 1.0f);
				step.scaleY = GetChildFloat(stepElem, "ScaleY", 1.0f);
				step.scaleZ = GetChildFloat(stepElem, "ScaleZ", 1.0f);
				step.inflateX = GetChildFloat(stepElem, "InflateX", 0.0f);
				step.inflateY = GetChildFloat(stepElem, "InflateY", 0.0f);
				step.inflateZ = GetChildFloat(stepElem, "InflateZ", 0.0f);
				break;
			}
			case AutomationStepType::InvertUVs: {
				step.invertU = GetChildBool(stepElem, "InvertU", false);
				step.invertV = GetChildBool(stepElem, "InvertV", false);
				break;
			}
			case AutomationStepType::DeleteBones: {
				const char* bn = GetChildText(stepElem, "BoneNames");
				if (bn)
					step.deleteBoneNames = SplitCommaSeparated(bn);
				step.deleteBoneFromProject = GetChildBool(stepElem, "FromProject", true);
				break;
			}
			case AutomationStepType::AddCustomBone: {
				const char* bn = GetChildText(stepElem, "BoneName");
				if (bn)
					step.addBoneName = bn;
				const char* pb = GetChildText(stepElem, "ParentBone");
				if (pb)
					step.addBoneParent = pb;
				step.addBoneTransX = GetChildFloat(stepElem, "TransX", 0.0f);
				step.addBoneTransY = GetChildFloat(stepElem, "TransY", 0.0f);
				step.addBoneTransZ = GetChildFloat(stepElem, "TransZ", 0.0f);
				step.addBoneRotX = GetChildFloat(stepElem, "RotX", 0.0f);
				step.addBoneRotY = GetChildFloat(stepElem, "RotY", 0.0f);
				step.addBoneRotZ = GetChildFloat(stepElem, "RotZ", 0.0f);
				break;
			}
			case AutomationStepType::EditBone: {
				const char* bn = GetChildText(stepElem, "BoneName");
				if (bn)
					step.editBoneName = bn;
				const char* pb = GetChildText(stepElem, "ParentBone");
				if (pb)
					step.editBoneParent = pb;
				step.editBoneTransX = GetChildFloat(stepElem, "TransX", 0.0f);
				step.editBoneTransY = GetChildFloat(stepElem, "TransY", 0.0f);
				step.editBoneTransZ = GetChildFloat(stepElem, "TransZ", 0.0f);
				step.editBoneRotX = GetChildFloat(stepElem, "RotX", 0.0f);
				step.editBoneRotY = GetChildFloat(stepElem, "RotY", 0.0f);
				step.editBoneRotZ = GetChildFloat(stepElem, "RotZ", 0.0f);
				break;
			}
			case AutomationStepType::ApplyPose: {
				const char* pn = GetChildText(stepElem, "PoseName");
				if (pn)
					step.poseName = pn;
				break;
			}
			case AutomationStepType::SaveProject: {
				const char* dn = GetChildText(stepElem, "DisplayName");
				if (dn)
					step.saveName = dn;
				const char* ofn = GetChildText(stepElem, "OutputFileName");
				if (ofn)
					step.saveOutputFileName = ofn;
				const char* odp = GetChildText(stepElem, "OutputDataPath");
				if (odp)
					step.saveOutputDataPath = odp;
				const char* ssf = GetChildText(stepElem, "SliderSetFile");
				if (ssf)
					step.saveSliderSetFile = ssf;
				const char* sdf = GetChildText(stepElem, "ShapeDataFolder");
				if (sdf)
					step.saveShapeDataFolder = sdf;
				const char* sdfile = GetChildText(stepElem, "ShapeDataFile");
				if (sdfile)
					step.saveShapeDataFile = sdfile;
				step.saveGenWeights = GetChildBool(stepElem, "GenWeights", true);
				step.saveAutoCopyRef = GetChildBool(stepElem, "AutoCopyRef", true);
				step.saveCopyRefFromProject = GetChildBool(stepElem, "CopyRefFromProject", false);
				const char* crsn = GetChildText(stepElem, "CopyRefShapeName");
				if (crsn)
					step.saveCopyRefShapeName = crsn;
				step.saveUseOriginal = GetChildBool(stepElem, "UseOriginal", false);
				const char* srf = GetChildText(stepElem, "ReplaceFrom");
				if (srf)
					step.saveReplaceFrom = srf;
				const char* srt = GetChildText(stepElem, "ReplaceTo");
				if (srt)
					step.saveReplaceTo = srt;
				const char* ssuffix = GetChildText(stepElem, "Suffix");
				if (ssuffix)
					step.saveSuffix = ssuffix;
				break;
			}
			case AutomationStepType::ExportFile: {
				const char* efp = GetChildText(stepElem, "FilePath");
				if (efp)
					step.exportFilePath = efp;
				step.exportWithRef = GetChildBool(stepElem, "WithRef", true);
				step.exportUseOriginalPath = GetChildBool(stepElem, "UseOriginalPath", false);
				const char* epre = GetChildText(stepElem, "Prefix");
				if (epre)
					step.exportPrefix = epre;
				const char* esuf = GetChildText(stepElem, "Suffix");
				if (esuf)
					step.exportSuffix = esuf;
				break;
			}
			case AutomationStepType::ResetTransforms:
				// No additional params
				break;
			case AutomationStepType::DuplicateShape: {
				const char* dn = GetChildText(stepElem, "NewName");
				if (dn)
					step.dupNewName = dn;
				break;
			}
			case AutomationStepType::MirrorShape: {
				step.mirrorX = GetChildBool(stepElem, "MirrorX", true);
				step.mirrorY = GetChildBool(stepElem, "MirrorY", false);
				step.mirrorZ = GetChildBool(stepElem, "MirrorZ", false);
				step.mirrorSwapBonesX = GetChildBool(stepElem, "SwapBonesX", false);
				break;
			}
			case AutomationStepType::LoadMask: {
				const char* mf = GetChildText(stepElem, "MaskFile");
				if (mf)
					step.loadMaskFile = mf;
				const char* mn = GetChildText(stepElem, "MaskName");
				if (mn)
					step.loadMaskName = mn;
				break;
			}
			case AutomationStepType::RemoveUnusedNodes:
				// No additional params
				break;
		}

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
		SetChildText(doc, batchElem, "Folder", batchFolder);
		SetChildText(doc, batchElem, "Extension", batchExtension);
		SetChildBool(doc, batchElem, "Subdirectories", batchSubdirectories, false);
		SetChildText(doc, batchElem, "FileFilter", batchFileFilter);
		SetChildBool(doc, batchElem, "FileFilterRegex", batchFileFilterRegex, false);
		SetChildText(doc, batchElem, "SliderSetFilter", batchSliderSetFilter);
		SetChildBool(doc, batchElem, "SliderSetFilterRegex", batchSliderSetFilterRegex, false);
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

		switch (step.type) {
			case AutomationStepType::ClearProject:
				// No additional params
				break;

			case AutomationStepType::LoadReference:
			case AutomationStepType::AddProject:
				SetChildText(doc, stepElem, "SourceFile", step.refSourceFile);
				SetChildText(doc, stepElem, "Set", step.refSet);
				SetChildText(doc, stepElem, "Shape", step.refShape);
				SetChildBool(doc, stepElem, "LoadAll", step.refLoadAll, false);
				SetChildBool(doc, stepElem, "MergeSliders", step.refMergeSliders, true);
				SetChildBool(doc, stepElem, "MergeZaps", step.refMergeZaps, true);
				SetChildBool(doc, stepElem, "AppendNewSliders", step.refAppendNewSliders, false);
				break;

			case AutomationStepType::ConformSliders:
				SetChildFloat(doc, stepElem, "ProximityRadius", step.conformProximityRadius, 10.0f);
				SetChildInt(doc, stepElem, "MaxResults", step.conformMaxResults, 10);
				SetChildBool(doc, stepElem, "NoSqueeze", step.conformNoSqueeze, false);
				SetChildBool(doc, stepElem, "SolidMode", step.conformSolidMode, false);
				SetChildBool(doc, stepElem, "AxisX", step.conformAxisX, true);
				SetChildBool(doc, stepElem, "AxisY", step.conformAxisY, true);
				SetChildBool(doc, stepElem, "AxisZ", step.conformAxisZ, true);
				if (!step.conformSliderNames.empty())
					SetChildText(doc, stepElem, "SliderNames", JoinStrings(step.conformSliderNames, ", "));
				break;

			case AutomationStepType::CopyBoneWeights:
				SetChildFloat(doc, stepElem, "ProximityRadius", step.weightProximityRadius, 10.0f);
				SetChildInt(doc, stepElem, "MaxResults", step.weightMaxResults, 10);
				if (!step.weightBoneList.empty())
					SetChildText(doc, stepElem, "BoneList", JoinStrings(step.weightBoneList, ", "));
				break;

			case AutomationStepType::ImportSliderData:
				SetChildText(doc, stepElem, "SliderDataFile", step.sliderDataFile);
				SetChildBool(doc, stepElem, "FromFolder", step.sliderDataFromFolder, false);
				SetChildBool(doc, stepElem, "MergeSliders", step.sliderMerge, false);
				if (!step.sliderNames.empty())
					SetChildText(doc, stepElem, "SliderNames", JoinStrings(step.sliderNames, ", "));
				break;

			case AutomationStepType::ImportFile:
				SetChildText(doc, stepElem, "FilePath", step.importFilePath);
				SetChildBool(doc, stepElem, "FromFolder", step.importFromFolder, false);
				break;

			case AutomationStepType::SetSliderValues:
				if (!step.setSliderNames.empty())
					SetChildText(doc, stepElem, "SliderNames", JoinStrings(step.setSliderNames, ", "));
				SetChildFloat(doc, stepElem, "Value", step.setSliderValue, 1.0f);
				break;

			case AutomationStepType::DeleteShape:
				// No additional params (uses target meshes)
				break;

			case AutomationStepType::RenameShape:
				SetChildText(doc, stepElem, "OldName", step.renameOldName);
				SetChildText(doc, stepElem, "NewName", step.renameNewName);
				break;

			case AutomationStepType::DeleteSlider:
				SetChildText(doc, stepElem, "SliderName", step.deleteSliderName);
				if (step.deleteSliderRegex)
					SetChildText(doc, stepElem, "Regex", "true");
				break;

			case AutomationStepType::SetReferenceShape:
				SetChildText(doc, stepElem, "ShapeName", step.setRefShapeName);
				break;

			case AutomationStepType::RefineMesh:
			case AutomationStepType::SetBaseShape:
			case AutomationStepType::ClearReference:
			case AutomationStepType::RemoveSkinning:
				// No additional params
				break;

			case AutomationStepType::TransformShape:
				SetChildFloat(doc, stepElem, "MoveX", step.moveX, 0.0f);
				SetChildFloat(doc, stepElem, "MoveY", step.moveY, 0.0f);
				SetChildFloat(doc, stepElem, "MoveZ", step.moveZ, 0.0f);
				SetChildFloat(doc, stepElem, "RotateX", step.rotateX, 0.0f);
				SetChildFloat(doc, stepElem, "RotateY", step.rotateY, 0.0f);
				SetChildFloat(doc, stepElem, "RotateZ", step.rotateZ, 0.0f);
				SetChildFloat(doc, stepElem, "ScaleX", step.scaleX, 1.0f);
				SetChildFloat(doc, stepElem, "ScaleY", step.scaleY, 1.0f);
				SetChildFloat(doc, stepElem, "ScaleZ", step.scaleZ, 1.0f);
				SetChildFloat(doc, stepElem, "InflateX", step.inflateX, 0.0f);
				SetChildFloat(doc, stepElem, "InflateY", step.inflateY, 0.0f);
				SetChildFloat(doc, stepElem, "InflateZ", step.inflateZ, 0.0f);
				break;

			case AutomationStepType::InvertUVs:
				SetChildBool(doc, stepElem, "InvertU", step.invertU, false);
				SetChildBool(doc, stepElem, "InvertV", step.invertV, false);
				break;

			case AutomationStepType::DeleteBones:
				if (!step.deleteBoneNames.empty())
					SetChildText(doc, stepElem, "BoneNames", JoinStrings(step.deleteBoneNames, ", "));
				SetChildBool(doc, stepElem, "FromProject", step.deleteBoneFromProject, true);
				break;

			case AutomationStepType::AddCustomBone:
				SetChildText(doc, stepElem, "BoneName", step.addBoneName);
				SetChildText(doc, stepElem, "ParentBone", step.addBoneParent);
				SetChildFloat(doc, stepElem, "TransX", step.addBoneTransX, 0.0f);
				SetChildFloat(doc, stepElem, "TransY", step.addBoneTransY, 0.0f);
				SetChildFloat(doc, stepElem, "TransZ", step.addBoneTransZ, 0.0f);
				SetChildFloat(doc, stepElem, "RotX", step.addBoneRotX, 0.0f);
				SetChildFloat(doc, stepElem, "RotY", step.addBoneRotY, 0.0f);
				SetChildFloat(doc, stepElem, "RotZ", step.addBoneRotZ, 0.0f);
				break;

			case AutomationStepType::EditBone:
				SetChildText(doc, stepElem, "BoneName", step.editBoneName);
				SetChildText(doc, stepElem, "ParentBone", step.editBoneParent);
				SetChildFloat(doc, stepElem, "TransX", step.editBoneTransX, 0.0f);
				SetChildFloat(doc, stepElem, "TransY", step.editBoneTransY, 0.0f);
				SetChildFloat(doc, stepElem, "TransZ", step.editBoneTransZ, 0.0f);
				SetChildFloat(doc, stepElem, "RotX", step.editBoneRotX, 0.0f);
				SetChildFloat(doc, stepElem, "RotY", step.editBoneRotY, 0.0f);
				SetChildFloat(doc, stepElem, "RotZ", step.editBoneRotZ, 0.0f);
				break;

			case AutomationStepType::ApplyPose:
				SetChildText(doc, stepElem, "PoseName", step.poseName);
				break;

			case AutomationStepType::SaveProject:
				SetChildText(doc, stepElem, "DisplayName", step.saveName);
				SetChildText(doc, stepElem, "OutputFileName", step.saveOutputFileName);
				SetChildText(doc, stepElem, "OutputDataPath", step.saveOutputDataPath);
				SetChildText(doc, stepElem, "SliderSetFile", step.saveSliderSetFile);
				SetChildText(doc, stepElem, "ShapeDataFolder", step.saveShapeDataFolder);
				SetChildText(doc, stepElem, "ShapeDataFile", step.saveShapeDataFile);
				SetChildBool(doc, stepElem, "GenWeights", step.saveGenWeights, true);
				SetChildBool(doc, stepElem, "AutoCopyRef", step.saveAutoCopyRef, true);
				SetChildBool(doc, stepElem, "CopyRefFromProject", step.saveCopyRefFromProject, false);
				SetChildText(doc, stepElem, "CopyRefShapeName", step.saveCopyRefShapeName);
				SetChildBool(doc, stepElem, "UseOriginal", step.saveUseOriginal, false);
				SetChildText(doc, stepElem, "ReplaceFrom", step.saveReplaceFrom);
				SetChildText(doc, stepElem, "ReplaceTo", step.saveReplaceTo);
				SetChildText(doc, stepElem, "Suffix", step.saveSuffix);
				break;

			case AutomationStepType::ExportFile:
				SetChildText(doc, stepElem, "FilePath", step.exportFilePath);
				SetChildBool(doc, stepElem, "WithRef", step.exportWithRef, true);
				SetChildBool(doc, stepElem, "UseOriginalPath", step.exportUseOriginalPath, false);
				SetChildText(doc, stepElem, "Prefix", step.exportPrefix);
				SetChildText(doc, stepElem, "Suffix", step.exportSuffix);
				break;

			case AutomationStepType::ResetTransforms:
				// No additional params
				break;

			case AutomationStepType::DuplicateShape:
				SetChildText(doc, stepElem, "NewName", step.dupNewName);
				break;

			case AutomationStepType::MirrorShape:
				SetChildBool(doc, stepElem, "MirrorX", step.mirrorX, true);
				SetChildBool(doc, stepElem, "MirrorY", step.mirrorY, false);
				SetChildBool(doc, stepElem, "MirrorZ", step.mirrorZ, false);
				SetChildBool(doc, stepElem, "SwapBonesX", step.mirrorSwapBonesX, false);
				break;

			case AutomationStepType::LoadMask:
				SetChildText(doc, stepElem, "MaskFile", step.loadMaskFile);
				SetChildText(doc, stepElem, "MaskName", step.loadMaskName);
				break;

			case AutomationStepType::RemoveUnusedNodes:
				// No additional params
				break;
		}
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

void AutomationScript::SubstitutePlaceholders(const std::map<std::string, std::string>& vars) {
	for (auto& step : steps) {
		if (!step.active)
			continue;

		SubstituteInString(step.note, vars);
		for (auto& m : step.targetMeshes)
			SubstituteInString(m, vars);

		SubstituteInString(step.refSourceFile, vars);
		SubstituteInString(step.refSet, vars);
		SubstituteInString(step.refShape, vars);
		SubstituteInString(step.sliderDataFile, vars);
		SubstituteInString(step.importFilePath, vars);
		SubstituteInString(step.renameOldName, vars);
		SubstituteInString(step.renameNewName, vars);
		SubstituteInString(step.deleteSliderName, vars);
		SubstituteInString(step.setRefShapeName, vars);
		SubstituteInString(step.addBoneName, vars);
		SubstituteInString(step.addBoneParent, vars);
		SubstituteInString(step.editBoneName, vars);
		SubstituteInString(step.editBoneParent, vars);
		SubstituteInString(step.poseName, vars);
		for (auto& b : step.deleteBoneNames)
			SubstituteInString(b, vars);
		SubstituteInString(step.saveName, vars);
		SubstituteInString(step.saveOutputFileName, vars);
		SubstituteInString(step.saveOutputDataPath, vars);
		SubstituteInString(step.saveSliderSetFile, vars);
		SubstituteInString(step.saveShapeDataFolder, vars);
		SubstituteInString(step.saveShapeDataFile, vars);
		SubstituteInString(step.saveReplaceFrom, vars);
		SubstituteInString(step.saveReplaceTo, vars);
		SubstituteInString(step.saveSuffix, vars);
		SubstituteInString(step.saveCopyRefShapeName, vars);

		SubstituteInString(step.exportFilePath, vars);
		SubstituteInString(step.exportPrefix, vars);
		SubstituteInString(step.exportSuffix, vars);
		SubstituteInString(step.dupNewName, vars);
		SubstituteInString(step.loadMaskFile, vars);
		SubstituteInString(step.loadMaskName, vars);

		for (auto& s : step.setSliderNames)
			SubstituteInString(s, vars);
		for (auto& s : step.conformSliderNames)
			SubstituteInString(s, vars);
		for (auto& s : step.weightBoneList)
			SubstituteInString(s, vars);
		for (auto& s : step.sliderNames)
			SubstituteInString(s, vars);
	}
}
