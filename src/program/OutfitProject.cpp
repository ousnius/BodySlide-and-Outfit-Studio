/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitProject.h"
#include "../files/ObjFile.h"
#include "../files/TriFile.h"
#include "../files/FBXWrangler.h"
#include "../program/FBXImportDialog.h"
#include "../utils/PlatformUtil.h"
#include "../components/WeightNorm.h"

#include "../FSEngine/FSManager.h"
#include "../FSEngine/FSEngine.h"

#include <sstream>
#include <regex>

extern ConfigurationManager Config;

OutfitProject::OutfitProject(OutfitStudioFrame* inOwner) {
	owner = inOwner;

	std::string defSkelFile = Config["Anim/DefaultSkeletonReference"];
	if (wxFileName(wxString::FromUTF8(defSkelFile)).IsRelative())
		LoadSkeletonReference(Config["AppDir"] + PathSepStr + defSkelFile);
	else
		LoadSkeletonReference(defSkelFile);

	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");
	if (targetGame == SKYRIM || targetGame == SKYRIMSE || targetGame == SKYRIMVR)
		mGenWeights = true;
}

OutfitProject::~OutfitProject() {
	for (auto &cloth : clothData)
		delete cloth.second;
}

std::string OutfitProject::Save(const wxString& strFileName,
	const wxString& strOutfitName,
	const wxString& strDataDir,
	const wxString& strBaseFile,
	const wxString& strGamePath,
	const wxString& strGameFile,
	bool genWeights,
	bool copyRef) {

	owner->UpdateProgress(1, _("Checking destination..."));
	std::string errmsg = "";
	std::string outfit{strOutfitName.ToUTF8()};
	std::string baseFile{strBaseFile.ToUTF8()};
	std::string gameFile{strGameFile.ToUTF8()};

	ReplaceForbidden(outfit);
	ReplaceForbidden(baseFile);
	ReplaceForbidden(gameFile);

	SliderSet outSet;
	outSet.SetName(outfit);
	outSet.SetDataFolder(strDataDir.ToUTF8().data());
	outSet.SetInputFile(baseFile);
	outSet.SetOutputPath(strGamePath.ToUTF8().data());
	outSet.SetOutputFile(gameFile);
	outSet.SetGenWeights(genWeights);

	wxString sliderSetsStr = "SliderSets";
	sliderSetsStr.Append(PathSepChar);

	wxString ssFileName = strFileName;
	if (ssFileName.Find(sliderSetsStr) == wxNOT_FOUND)
		ssFileName = ssFileName.Prepend(sliderSetsStr);

	mFileName = ssFileName;
	mOutfitName = wxString::FromUTF8(outfit);
	mDataDir = strDataDir;
	mBaseFile = wxString::FromUTF8(baseFile);
	mGamePath = strGamePath;
	mGameFile = wxString::FromUTF8(gameFile);
	mCopyRef = copyRef;
	mGenWeights = genWeights;

	auto shapes = workNif.GetShapes();

	wxString folder(wxString::Format("%s/%s/%s", wxString::FromUTF8(Config["AppDir"]), "ShapeData", strDataDir));
	wxFileName::Mkdir(folder, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	int prog = 5;
	int step = 10 / shapes.size();
	owner->UpdateProgress(prog);

	if (copyRef && baseShape) {
		// Add all the reference shapes to the target list.
		std::string baseShapeName = baseShape->GetName();
		outSet.AddShapeTarget(baseShapeName, ShapeToTarget(baseShapeName));
		outSet.AddTargetDataFolder(ShapeToTarget(baseShapeName), activeSet.ShapeToDataFolder(baseShapeName));
		outSet.SetSmoothSeamNormals(baseShapeName, activeSet.GetSmoothSeamNormals(baseShapeName));
		outSet.SetLockNormals(baseShapeName, activeSet.GetLockNormals(baseShapeName));
		owner->UpdateProgress(prog += step, _("Adding reference shapes..."));
	}

	// Add all the outfit shapes to the target list.
	for (auto &s : shapes) {
		if (IsBaseShape(s))
			continue;

		std::string shapeName = s->GetName();
		outSet.AddShapeTarget(shapeName, ShapeToTarget(shapeName));

		// Reference only if not local folder
		std::string shapeDataFolder = activeSet.ShapeToDataFolder(shapeName);
		if (shapeDataFolder != activeSet.GetDefaultDataFolder())
			outSet.AddTargetDataFolder(ShapeToTarget(shapeName), activeSet.ShapeToDataFolder(shapeName));

		outSet.SetSmoothSeamNormals(shapeName, activeSet.GetSmoothSeamNormals(shapeName));
		outSet.SetLockNormals(shapeName, activeSet.GetLockNormals(shapeName));
		owner->UpdateProgress(prog += step, _("Adding outfit shapes..."));
	}
	
	std::string osdFileName = baseFile.substr(0, baseFile.find_last_of('.')) + ".osd";

	if (activeSet.size() > 0) {
		// Copy the reference slider info and add the outfit data to them.
		int id;
		std::string targ;
		std::string targSlider;
		std::string targSliderData;

		prog = 10;
		step = 20 / activeSet.size();
		owner->UpdateProgress(prog);

		for (int i = 0; i < activeSet.size(); i++) {
			id = outSet.CopySlider(&activeSet[i]);
			outSet[id].Clear();
			if (copyRef && baseShape) {
				std::string baseShapeName = baseShape->GetName();
				targ = ShapeToTarget(baseShapeName);
				targSlider = activeSet[i].TargetDataName(targ);
				if (baseDiffData.GetDiffSet(targSlider) && baseDiffData.GetDiffSet(targSlider)->size() > 0) {
					if (activeSet[i].IsLocalData(targSlider)) {
						targSliderData = osdFileName + PathSepStr + targSlider;
						outSet[id].AddDataFile(targ, targSlider, targSliderData);
					}
					else {
						targSliderData = activeSet[i].DataFileName(targSlider);
						outSet[id].AddDataFile(targ, targSlider, targSliderData, false);
					}
				}
			}

			for (auto &s : shapes) {
				if (IsBaseShape(s))
					continue;

				std::string shapeName = s->GetName();
				targ = ShapeToTarget(shapeName);
				targSlider = activeSet[i].TargetDataName(targ);
				if (targSlider.empty())
					targSlider = targ + outSet[i].name;

				if (morpher.GetResultDiffSize(shapeName, activeSet[i].name) > 0) {
					std::string shapeDataFolder = activeSet.ShapeToDataFolder(shapeName);
					if (shapeDataFolder == activeSet.GetDefaultDataFolder() || activeSet[i].IsLocalData(targSlider)) {
						targSliderData = osdFileName + PathSepStr + targSlider;
						outSet[i].AddDataFile(targ, targSlider, targSliderData);
					}
					else {
						targSliderData = activeSet[i].DataFileName(targSlider);
						outSet[i].AddDataFile(targ, targSlider, targSliderData, false);
					}
				}
			}
			owner->UpdateProgress(prog += step, _("Calculating slider data..."));
		}
	}

	std::string saveDataPath = Config["AppDir"] + PathSepStr + "ShapeData" + PathSepStr + mDataDir.ToUTF8().data();
	SaveSliderData(saveDataPath + PathSepStr + osdFileName, copyRef);
	
	prog = 60;
	owner->UpdateProgress(prog, _("Creating slider set file..."));

	if (wxFileName(ssFileName).IsRelative())
		ssFileName = ssFileName.Prepend(wxString::FromUTF8(Config["AppDir"] + PathSepStr));

	std::string ssUFileName{ssFileName.ToUTF8()};
	SliderSetFile ssf(ssUFileName);
	if (ssf.fail()) {
		ssf.New(ssUFileName);
		if (ssf.fail()) {
			errmsg = _("Failed to open or create slider set file: ") + ssUFileName;
			return errmsg;
		}
	}

	auto it = strFileName.rfind('/');
	if (it == std::string::npos)
		it = strFileName.rfind('\\');
	if (it != std::string::npos) {
		wxString ssNewFolder(wxString::Format("%s/%s", wxString::FromUTF8(Config["AppDir"]), strFileName.substr(0, it)));
		wxFileName::Mkdir(ssNewFolder, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	}
	else {
		wxString ssNewFolder(wxString::Format("%s/%s", wxString::FromUTF8(Config["AppDir"]), "SliderSets"));
		wxFileName::Mkdir(ssNewFolder, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	}

	owner->UpdateProgress(61, _("Saving slider set file..."));
	ssf.UpdateSet(outSet);
	if (!ssf.Save()) {
		errmsg = _("Failed to write to slider set file: ") + ssUFileName;
		return errmsg;
	}

	owner->UpdateProgress(70, _("Saving NIF file..."));

	std::string saveFileName = saveDataPath + PathSepStr + baseFile;

	if (workNif.IsValid()) {
		workAnim.CleanupBones();
		owner->AnimationGUIFromProj();

		NifFile clone(workNif);
		ChooseClothData(clone);

		if (!copyRef && baseShape) {
			std::string baseShapeName = baseShape->GetName();
			auto shape = clone.FindBlockByName<NiShape>(baseShapeName);
			clone.DeleteShape(shape);
			workAnim.WriteToNif(&clone, baseShapeName);
		}
		else
			workAnim.WriteToNif(&clone);

		for (auto &s : clone.GetShapes())
			clone.UpdateSkinPartitions(s);

		clone.SetShapeOrder(owner->GetShapeList());
		clone.GetHeader().SetExportInfo("Exported using Outfit Studio.");

		std::fstream file;
		PlatformUtil::OpenFileStream(file, saveFileName, std::ios::out | std::ios::binary);

		if (clone.Save(file)) {
			errmsg = _("Failed to write base .nif file: ") + saveFileName;
			return errmsg;
		}
	}

	owner->ShowPartition();
	owner->UpdateProgress(100, _("Finished"));
	return errmsg;
}

bool OutfitProject::SaveSliderData(const std::string& fileName, bool copyRef) {
	auto shapes = workNif.GetShapes();

	if (activeSet.size() > 0) {
		std::string targ;
		std::string targSlider;

		DiffDataSets osdDiffs;
		std::map<std::string, std::map<std::string, std::string>> osdNames;
		
		// Copy the changed reference slider data and add the outfit data to them.
		for (int i = 0; i < activeSet.size(); i++) {
			if (copyRef && baseShape) {
				std::string baseShapeName = baseShape->GetName();
				targ = ShapeToTarget(baseShapeName);
				targSlider = activeSet[i].TargetDataName(targ);
				if (baseDiffData.GetDiffSet(targSlider) && baseDiffData.GetDiffSet(targSlider)->size() > 0) {
					if (activeSet[i].IsLocalData(targSlider)) {
						std::unordered_map<ushort, Vector3>* diff = baseDiffData.GetDiffSet(targSlider);
						osdDiffs.LoadSet(targSlider, targ, *diff);
						osdNames[fileName][targSlider] = targ;
					}
				}
			}

			for (auto &s : shapes) {
				if (IsBaseShape(s))
					continue;

				std::string shapeName = s->GetName();
				targ = ShapeToTarget(shapeName);
				targSlider = activeSet[i].TargetDataName(targ);
				if (targSlider.empty())
					targSlider = targ + activeSet[i].name;

				if (morpher.GetResultDiffSize(shapeName, activeSet[i].name) > 0) {
					std::string shapeDataFolder = activeSet.ShapeToDataFolder(shapeName);
					if (shapeDataFolder == activeSet.GetDefaultDataFolder() || activeSet[i].IsLocalData(targSlider)) {
						std::unordered_map<ushort, Vector3> diff;
						morpher.GetRawResultDiff(shapeName, activeSet[i].name, diff);
						osdDiffs.LoadSet(targSlider, targ, diff);
						osdNames[fileName][targSlider] = targ;
					}
				}
			}
		}

		if (!osdDiffs.SaveData(osdNames))
			return false;
	}

	return true;
}

void OutfitProject::SetBaseShape(NiShape* shape) {
	if (baseShape != shape) {
		// Copy data from base shape to regular shape
		if (baseShape) {
			std::string shapeName = baseShape->GetName();
			std::string srcTarget = ShapeToTarget(shapeName);

			for (int i = 0; i < activeSet.size(); i++) {
				std::string srcTargetData = activeSet[i].TargetDataName(srcTarget);

				auto diff = baseDiffData.GetDiffSet(srcTargetData);
				if (diff)
					morpher.SetResultDiff(shapeName, activeSet[i].name, *diff);

				activeSet[i].RenameTarget(srcTarget, shapeName);
				baseDiffData.ClearSet(srcTargetData);
			}

			activeSet.AddShapeTarget(shapeName, shapeName);
			activeSet.Retarget(srcTarget, shapeName);
		}

		// Copy data from regular shape to base shape
		if (shape) {
			std::string shapeName = shape->GetName();
			std::string target = ShapeToTarget(shapeName);

			for (int i = 0; i < activeSet.size(); i++) {
				std::string sliderName = activeSet[i].name;
				std::string targetData = activeSet[i].TargetDataName(target);
				if (targetData.empty()) {
					targetData = target + sliderName;
					activeSet[i].AddDataFile(target, target + sliderName, target + sliderName);
				}
				else
					activeSet[i].SetLocalData(targetData);

				std::unordered_map<ushort, Vector3> diff;
				morpher.GetRawResultDiff(shapeName, sliderName, diff);
				morpher.ClearResultSet(targetData);

				baseDiffData.LoadSet(targetData, shapeName, diff);
			}

			activeSet.AddShapeTarget(shapeName, shapeName);
		}
	}

	baseShape = shape;
}


std::string OutfitProject::SliderSetName() {
	return activeSet.GetName();
}

std::string OutfitProject::SliderSetFileName() {
	return activeSet.GetInputFileName();
}

std::string OutfitProject::OutfitName() {
	return outfitName;
}

void OutfitProject::ReplaceForbidden(std::string& str, const char& replacer) {
	const std::string forbiddenChars = "\\/:*?\"<>|";

	std::transform(str.begin(), str.end(), str.begin(), [&forbiddenChars, &replacer](char c) {
		return forbiddenChars.find(c) != std::string::npos ? replacer : c;
	});
}

bool OutfitProject::ValidSlider(int index) {
	if (index >= 0 && index < activeSet.size())
		return true;
	return false;
}

bool OutfitProject::ValidSlider(const std::string& sliderName) {
	return activeSet.SliderExists(sliderName);
}

bool OutfitProject::AllSlidersZero() {
	for (int i = 0; i < activeSet.size(); i++)
		if (activeSet[i].curValue != 0.0f)
			return false;
	return true;
}

int OutfitProject::SliderCount() {
	return activeSet.size();
}

void OutfitProject::GetSliderList(std::vector<std::string>& sliderNames) {
	for (int i = 0; i < activeSet.size(); i++)
		sliderNames.push_back(activeSet[i].name);
}

std::string OutfitProject::GetSliderName(int index) {
	if (!ValidSlider(index))
		return "";
	return activeSet[index].name;
}

void OutfitProject::AddEmptySlider(const std::string& newName) {
	int sliderID = activeSet.CreateSlider(newName);
	activeSet[sliderID].bShow = true;

	if (baseShape) {
		std::string baseShapeName = baseShape->GetName();
		std::string target = ShapeToTarget(baseShapeName);
		std::string shapeSlider = target + newName;
		activeSet[sliderID].AddDataFile(target, shapeSlider, shapeSlider);
		activeSet.AddShapeTarget(baseShapeName, target);
		baseDiffData.AddEmptySet(shapeSlider, target);
	}
}

void OutfitProject::AddZapSlider(const std::string& newName, std::unordered_map<ushort, float>& verts, NiShape* shape) {
	std::unordered_map<ushort, Vector3> diffData;
	Vector3 moveVec(0.0f, 1.0f, 0.0f);
	for (auto &v : verts)
		diffData[v.first] = moveVec;

	std::string target = ShapeToTarget(shape->GetName());
	std::string shapeSlider = target + newName;

	int sliderID = activeSet.CreateSlider(newName);
	activeSet[sliderID].bZap = true;
	activeSet[sliderID].defBigValue = 0.0f;
	activeSet[sliderID].defSmallValue = 0.0f;

	if (IsBaseShape(shape)) {
		activeSet[sliderID].AddDataFile(target, shapeSlider, shapeSlider);
		activeSet.AddShapeTarget(shape->GetName(), target);
		baseDiffData.AddEmptySet(shapeSlider, target);
		for (auto &i : diffData)
			baseDiffData.SumDiff(shapeSlider, target, i.first, i.second);
	}
	else
		morpher.SetResultDiff(shape->GetName(), newName, diffData);
}

void OutfitProject::AddCombinedSlider(const std::string& newName) {
	std::vector<Vector3> verts;
	std::unordered_map<ushort, Vector3> diffData;

	for (auto &s : workNif.GetShapes()) {
		if (IsBaseShape(s))
			continue;

		diffData.clear();
		GetLiveVerts(s, verts);
		workNif.CalcShapeDiff(s, &verts, diffData);
		morpher.SetResultDiff(s->GetName(), newName, diffData);
	}

	int sliderID = activeSet.CreateSlider(newName);
	if (baseShape) {
		std::string baseShapeName = baseShape->GetName();
		std::string target = ShapeToTarget(baseShapeName);
		std::string shapeSlider = target + newName;
		activeSet[sliderID].AddDataFile(target, shapeSlider, shapeSlider);
		baseDiffData.AddEmptySet(shapeSlider, target);

		GetLiveVerts(baseShape, verts);
		workNif.CalcShapeDiff(baseShape, &verts, diffData);

		for (auto &i : diffData)
			baseDiffData.SumDiff(shapeSlider, target, i.first, i.second);
	}
}

NiShape* OutfitProject::CreateNifShapeFromData(const std::string& shapeName, std::vector<Vector3>& v, std::vector<Triangle>& t, std::vector<Vector2>& uv, std::vector<Vector3>* norms) {
	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");

	if (!workNif.IsValid()) {
		NiVersion version;

		switch (targetGame) {
		case FO3:
		case FONV:
			version.SetFile(V20_2_0_7);
			version.SetUser(11);
			version.SetStream(34);
			break;
		case SKYRIM:
			version.SetFile(V20_2_0_7);
			version.SetUser(12);
			version.SetStream(83);
			break;
		case FO4:
		case FO4VR:
			version.SetFile(V20_2_0_7);
			version.SetUser(12);
			version.SetStream(130);
			break;
		case SKYRIMSE:
		case SKYRIMVR:
			version.SetFile(V20_2_0_7);
			version.SetUser(12);
			version.SetStream(100);
			break;
		}

		workNif.Create(version);
	}

	NiHeader& hdr = workNif.GetHeader();
	auto rootNode = workNif.GetRootNode();
	if (!rootNode)
		return nullptr;

	NiShape* shapeResult = nullptr;
	if (targetGame <= SKYRIM) {
		auto nifShapeData = new NiTriShapeData();
		nifShapeData->Create(&v, &t, &uv, norms);

		int dataID = hdr.AddBlock(nifShapeData);

		auto nifSkinData = new NiSkinData();
		int skinID = hdr.AddBlock(nifSkinData);

		auto nifSkinPartition = new NiSkinPartition();
		int partID = hdr.AddBlock(nifSkinPartition);

		auto nifDismemberInst = new BSDismemberSkinInstance();
		int dismemberID = hdr.AddBlock(nifDismemberInst);
		nifDismemberInst->SetDataRef(skinID);
		nifDismemberInst->SetSkinPartitionRef(partID);
		nifDismemberInst->SetSkeletonRootRef(workNif.GetBlockID(workNif.GetRootNode()));

		auto nifTexset = new BSShaderTextureSet(hdr.GetVersion());

		int shaderID;
		BSLightingShaderProperty* nifShader = nullptr;
		BSShaderPPLightingProperty* nifShaderPP = nullptr;
		switch (targetGame) {
		case FO3:
		case FONV:
			nifShaderPP = new BSShaderPPLightingProperty();
			shaderID = hdr.AddBlock(nifShaderPP);
			nifShaderPP->SetTextureSetRef(hdr.AddBlock(nifTexset));
			break;
		case SKYRIM:
		default:
			nifShader = new BSLightingShaderProperty(hdr.GetVersion());
			shaderID = hdr.AddBlock(nifShader);
			nifShader->SetTextureSetRef(hdr.AddBlock(nifTexset));
		}

		auto nifTriShape = new NiTriShape();
		int shapeID = hdr.AddBlock(nifTriShape);
		if (targetGame < SKYRIM)
			nifTriShape->GetProperties().AddBlockRef(shaderID);
		else
			nifTriShape->SetShaderPropertyRef(shaderID);

		nifTriShape->SetName(shapeName);
		nifTriShape->SetDataRef(dataID);
		nifTriShape->SetSkinInstanceRef(dismemberID);

		nifTriShape->SetGeomData(nifShapeData);
		workNif.SetDefaultPartition(nifTriShape);

		rootNode->GetChildren().AddBlockRef(shapeID);
		shapeResult = nifTriShape;
	}
	else if (targetGame == FO4 || targetGame == FO4VR) {
		BSTriShape* triShapeBase;
		std::string wetShaderName = "template/OutfitTemplate_Wet.bgsm";
		auto nifBSTriShape = new BSSubIndexTriShape();
		nifBSTriShape->Create(&v, &t, &uv, norms);
		int shapeID = hdr.AddBlock(nifBSTriShape);

		auto nifBSSkinInstance = new BSSkinInstance();
		int skinID = hdr.AddBlock(nifBSSkinInstance);
		nifBSSkinInstance->SetTargetRef(workNif.GetBlockID(workNif.GetRootNode()));

		auto nifBoneData = new BSSkinBoneData();
		int boneID = hdr.AddBlock(nifBoneData);
		nifBSSkinInstance->SetDataRef(boneID);
		nifBSTriShape->SetSkinInstanceRef(skinID);
		triShapeBase = nifBSTriShape;

		auto nifTexset = new BSShaderTextureSet(hdr.GetVersion());

		auto nifShader = new BSLightingShaderProperty(hdr.GetVersion());
		int shaderID = hdr.AddBlock(nifShader);
		nifShader->SetTextureSetRef(hdr.AddBlock(nifTexset));
		nifShader->SetWetMaterialName(wetShaderName);

		triShapeBase->SetName(shapeName);
		triShapeBase->SetShaderPropertyRef(shaderID);

		rootNode->GetChildren().AddBlockRef(shapeID);
		shapeResult = triShapeBase;
	}
	else {
		auto triShape = new BSTriShape();
		triShape->Create(&v, &t, &uv, norms);
		int shapeID = hdr.AddBlock(triShape);

		auto nifSkinData = new NiSkinData();
		int skinID = hdr.AddBlock(nifSkinData);

		auto nifSkinPartition = new NiSkinPartition();
		int partID = hdr.AddBlock(nifSkinPartition);

		auto nifDismemberInst = new BSDismemberSkinInstance();
		int dismemberID = hdr.AddBlock(nifDismemberInst);
		nifDismemberInst->SetDataRef(skinID);
		nifDismemberInst->SetSkinPartitionRef(partID);
		nifDismemberInst->SetSkeletonRootRef(workNif.GetBlockID(workNif.GetRootNode()));
		triShape->SetSkinInstanceRef(dismemberID);
		triShape->SetSkinned(true);

		auto nifTexset = new BSShaderTextureSet(hdr.GetVersion());

		auto nifShader = new BSLightingShaderProperty(hdr.GetVersion());
		int shaderID = hdr.AddBlock(nifShader);
		nifShader->SetTextureSetRef(hdr.AddBlock(nifTexset));

		triShape->SetName(shapeName);
		triShape->SetShaderPropertyRef(shaderID);

		workNif.SetDefaultPartition(triShape);
		workNif.UpdateSkinPartitions(triShape);

		rootNode->GetChildren().AddBlockRef(shapeID);
		shapeResult = triShape;
	}

	SetTextures(shapeResult);
	return shapeResult;
}

std::string OutfitProject::SliderShapeDataName(int index, const std::string& shapeName) {
	if (!ValidSlider(index))
		return "";
	return activeSet.ShapeToDataName(index, shapeName);
}

bool OutfitProject::SliderClamp(int index) {
	if (!ValidSlider(index))
		return false;
	return activeSet[index].bClamp;
}

bool OutfitProject::SliderZap(int index) {
	if (!ValidSlider(index))
		return false;
	return activeSet[index].bZap;
}

bool OutfitProject::SliderUV(int index) {
	if (!ValidSlider(index))
		return false;
	return activeSet[index].bUV;
}

wxArrayString OutfitProject::SliderZapToggles(int index) {
	wxArrayString toggles;
	if (ValidSlider(index))
		for (auto &toggle : activeSet[index].zapToggles)
			toggles.Add(wxString::FromUTF8(toggle));

	return toggles;
}

bool OutfitProject::SliderInvert(int index) {
	if (!ValidSlider(index))
		return false;
	return activeSet[index].bInvert;
}

bool OutfitProject::SliderHidden(int index) {
	if (!ValidSlider(index))
		return false;
	return activeSet[index].bHidden;
}

void OutfitProject::SetSliderZap(int index, bool zap) {
	if (!ValidSlider(index))
		return;
	activeSet[index].bZap = zap;
}

void OutfitProject::SetSliderZapToggles(int index, const wxArrayString& toggles) {
	if (!ValidSlider(index))
		return;

	std::vector<std::string> zapToggles;
	for (auto &s : toggles)
		zapToggles.push_back(s.ToUTF8().data());

	activeSet[index].zapToggles = zapToggles;
}

void OutfitProject::SetSliderInvert(int index, bool inv) {
	if (!ValidSlider(index))
		return;
	activeSet[index].bInvert = inv;
}

void OutfitProject::SetSliderUV(int index, bool uv) {
	if (!ValidSlider(index))
		return;
	activeSet[index].bUV = uv;
}

void OutfitProject::SetSliderHidden(int index, bool hidden) {
	if (!ValidSlider(index))
		return;
	activeSet[index].bHidden = hidden;
}

void OutfitProject::SetSliderDefault(int index, int val, bool isHi) {
	if (!ValidSlider(index))
		return;

	if (!isHi)
		activeSet[index].defSmallValue = val;
	else
		activeSet[index].defBigValue = val;
}

void OutfitProject::SetSliderName(int index, const std::string& newName) {
	if (!ValidSlider(index))
		return;

	std::string oldName = activeSet[index].name;
	for (auto &s : workNif.GetShapes()) {
		std::string shapeName = s->GetName();
		std::string oldDT = shapeName + oldName;
		std::string newDT = shapeName + newName;

		if (IsBaseShape(s))
			baseDiffData.RenameSet(oldDT, newDT);
		else
			morpher.RenameResultDiffData(shapeName, oldName, newName);

		activeSet[index].RenameData(oldDT, newDT);
		activeSet[index].SetLocalData(newDT);
	}

	activeSet[index].name = newName;
}

float& OutfitProject::SliderValue(int index) {
	return activeSet[index].curValue;
}

float& OutfitProject::SliderValue(const std::string& name) {
	return activeSet[name].curValue;
}

float OutfitProject::SliderDefault(int index, bool hi) {
	if (hi)
		return activeSet[index].defBigValue;

	return activeSet[index].defSmallValue;
}

bool& OutfitProject::SliderShow(int index) {
	return activeSet[index].bShow;
}

bool& OutfitProject::SliderShow(const std::string& sliderName) {
	return activeSet[sliderName].bShow;
}

int OutfitProject::SliderIndexFromName(const std::string& sliderName) {
	for (int i = 0; i < activeSet.size(); i++)
		if (activeSet[i].name == sliderName)
			return i;

	return -1;
}

void OutfitProject::NegateSlider(const std::string& sliderName, NiShape* shape) {
	std::string target = ShapeToTarget(shape->GetName());

	if (IsBaseShape(shape)) {
		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.ScaleDiff(sliderData, target, -1.0f);
	}
	else
		morpher.ScaleResultDiff(target, sliderName, -1.0f);
}

void OutfitProject::MaskAffected(const std::string& sliderName, NiShape* shape) {
	mesh* m = owner->glView->GetMesh(shape->GetName());
	if (!m)
		return;

	m->ColorChannelFill(0, 0.0f);

	if (IsBaseShape(shape)) {
		std::vector<ushort> outIndices;
		std::string target = ShapeToTarget(shape->GetName());

		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.GetDiffIndices(sliderData, target, outIndices);

		for (auto &i : outIndices)
			m->vcolors[i].x = 1.0f;
	}
	else {
		std::unordered_map<ushort, Vector3> outDiff;
		morpher.GetRawResultDiff(shape->GetName(), sliderName, outDiff);

		for (auto &i : outDiff)
			m->vcolors[i.first].x = 1.0f;
	}

	m->QueueUpdate(mesh::UpdateType::VertexColors);
}

bool OutfitProject::WriteMorphTRI(const std::string& triPath) {
	DiffDataSets currentDiffs;
	activeSet.LoadSetDiffData(currentDiffs);

	TriFile tri;
	std::string triFilePath = triPath;

	for (auto &shape : workNif.GetShapes()) {
		int shapeVertCount = GetVertexCount(shape);
		if (shapeVertCount <= 0)
			continue;

		std::string shapeName = shape->GetName();
		bool bIsOutfit = true;
		if (IsBaseShape(shape))
			bIsOutfit = false;

		for (int s = 0; s < activeSet.size(); s++) {
			if (!activeSet[s].bClamp && !activeSet[s].bZap) {
				MorphDataPtr morph = std::make_shared<MorphData>();
				morph->name = activeSet[s].name;

				if (activeSet[s].bUV) {
					morph->type = MORPHTYPE_UV;

					std::vector<Vector2> uvs;
					uvs.resize(shapeVertCount);

					std::string target = ShapeToTarget(shapeName);
					if (!bIsOutfit) {
						std::string dn = activeSet[s].TargetDataName(target);
						if (dn.empty())
							continue;

						currentDiffs.ApplyUVDiff(dn, target, 1.0f, &uvs);
					}
					else
						morpher.ApplyResultToUVs(morph->name, target, &uvs);

					int i = 0;
					for (auto &uv : uvs) {
						Vector3 v(uv.u, uv.v, 0.0f);
						if (!v.IsZero(true))
							morph->offsets.emplace(i, v);
						i++;
					}
				}
				else {
					morph->type = MORPHTYPE_POSITION;

					std::vector<Vector3> verts;
					verts.resize(shapeVertCount);

					std::string target = ShapeToTarget(shapeName);
					if (!bIsOutfit) {
						std::string dn = activeSet[s].TargetDataName(target);
						if (dn.empty())
							continue;

						currentDiffs.ApplyDiff(dn, target, 1.0f, &verts);
					}
					else
						morpher.ApplyResultToVerts(morph->name, target, &verts);

					int i = 0;
					for (auto &v : verts) {
						if (!v.IsZero(true))
							morph->offsets.emplace(i, v);
						i++;
					}
				}

				if (morph->offsets.size() > 0)
					tri.AddMorph(shapeName, morph);
			}
		}
	}

	if (!tri.Write(triFilePath))
		return false;

	return true;
}

bool OutfitProject::WriteHeadTRI(NiShape* shape, const std::string& triPath) {
	if (!shape)
		return false;

	DiffDataSets currentDiffs;
	activeSet.LoadSetDiffData(currentDiffs);

	TriHeadFile tri;
	std::string triFilePath = triPath;

	int shapeVertCount = GetVertexCount(shape);
	if (shapeVertCount <= 0)
		return false;

	std::string shapeName = shape->GetName();
	bool bIsOutfit = true;
	if (IsBaseShape(shape))
		bIsOutfit = false;

	std::vector<Triangle> tris;
	if (!shape->GetTriangles(tris))
		return false;

	const std::vector<Vector3>* verts = workNif.GetRawVertsForShape(shape);
	if (!verts)
		return false;

	tri.SetVertices(*verts);
	tri.SetTriangles(tris);

	const std::vector<Vector2>* uvs = workNif.GetUvsForShape(shape);
	if (uvs)
		tri.SetUV(*uvs);

	for (int s = 0; s < activeSet.size(); s++) {
		if (!activeSet[s].bClamp && !activeSet[s].bZap && !activeSet[s].bUV) {
			TriHeadMorph morph;
			morph.morphName = activeSet[s].name;

			std::vector<Vector3> morphVerts;
			morphVerts.resize(shapeVertCount);

			std::string target = ShapeToTarget(shapeName);
			if (!bIsOutfit) {
				std::string dn = activeSet[s].TargetDataName(target);
				if (dn.empty())
					continue;

				currentDiffs.ApplyDiff(dn, target, 1.0f, &morphVerts);
			}
			else
				morpher.ApplyResultToVerts(morph.morphName, target, &morphVerts);

			morph.vertices = morphVerts;
			tri.AddMorph(morph);
		}
	}

	if (!tri.Write(triFilePath))
		return false;

	return true;
}

int OutfitProject::SaveSliderBSD(const std::string& sliderName, NiShape* shape, const std::string& fileName) {
	std::string target = ShapeToTarget(shape->GetName());

	if (IsBaseShape(shape)) {
		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.SaveSet(sliderData, target, fileName);
	}
	else
		morpher.SaveResultDiff(target, sliderName, fileName);

	return 0;
}

int OutfitProject::SaveSliderOBJ(const std::string& sliderName, NiShape* shape, const std::string& fileName, const bool onlyDiff) {
	if (!shape)
		return 1;

	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::string target = ShapeToTarget(shape->GetName());
	const std::vector<Vector3>* verts = workNif.GetRawVertsForShape(shape);
	if (!verts)
		return 2;

	const std::vector<Vector2>* uvs = workNif.GetUvsForShape(shape);
	const std::vector<Vector3> norms;

	std::vector<Vector3> outVerts = *verts;

	if (IsBaseShape(shape)) {
		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		bool foundDiff = baseDiffData.ApplyDiff(sliderData, target, 1.0f, &outVerts);
		if (onlyDiff && !foundDiff)
			return 0;
	}
	else {
		bool foundDiff = morpher.ApplyResultToVerts(sliderName, target, &outVerts);
		if (onlyDiff && !foundDiff)
			return 0;
	}

	ObjFile obj;
	obj.SetScale(Vector3(0.1f, 0.1f, 0.1f));
	obj.AddGroup(shape->GetName(), outVerts, tris, uvs ? *uvs : std::vector<Vector2>(), norms);
	if (obj.Save(fileName))
		return 3;

	return 0;
}

void OutfitProject::SetSliderFromBSD(const std::string& sliderName, NiShape* shape, const std::string& fileName) {
	std::string target = ShapeToTarget(shape->GetName());
	if (IsBaseShape(shape)) {
		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.LoadSet(sliderData, target, fileName);
	}
	else {
		DiffDataSets tmpSet;
		tmpSet.LoadSet(sliderName, target, fileName);
		std::unordered_map<ushort, Vector3>* diff = tmpSet.GetDiffSet(sliderName);
		morpher.SetResultDiff(target, sliderName, (*diff));
	}
}

bool OutfitProject::SetSliderFromOBJ(const std::string& sliderName, NiShape* shape, const std::string& fileName) {
	std::string target = ShapeToTarget(shape->GetName());

	ObjOptionsImport options;
	options.noFaces = true;

	ObjFile obj;
	obj.LoadForNif(fileName, options);

	// File needs at least one group
	std::vector<std::string> groupNames = obj.GetGroupList();
	if (groupNames.empty())
		return false;

	// Use first shape or shape with matching name
	std::string sourceShape = groupNames.front();
	if (std::find(groupNames.begin(), groupNames.end(), shape->GetName()) != groupNames.end())
		sourceShape = shape->GetName();

	std::vector<Vector3> objVerts;
	std::vector<Vector2> objUVs;
	if (!obj.CopyDataForGroup(sourceShape, &objVerts, nullptr, &objUVs, nullptr))
		return false;

	std::unordered_map<ushort, Vector3> diff;
	if (activeSet[sliderName].bUV) {
		if (workNif.CalcUVDiff(shape, &objUVs, diff))
			return false;
	}
	else {
		if (workNif.CalcShapeDiff(shape, &objVerts, diff, 10.0f))
			return false;
	}

	if (IsBaseShape(shape)) {
		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.LoadSet(sliderData, target, diff);
	}
	else
		morpher.SetResultDiff(target, sliderName, diff);

	return true;
}

bool OutfitProject::SetSliderFromFBX(const std::string& sliderName, NiShape* shape, const std::string& fileName) {
	std::string target = ShapeToTarget(shape->GetName());

	FBXWrangler fbxw;
	bool result = fbxw.ImportScene(fileName);
	if (!result)
		return 1;

	std::vector<std::string>shapes;
	fbxw.GetShapeNames(shapes);
	bool found = false;
	for (auto &s : shapes)
		if (s == shape->GetName())
			found = true;

	if (!found)
		return false;

	FBXShape* fbxShape = fbxw.GetShape(shape->GetName());

	std::unordered_map<ushort, Vector3> diff;
	if (IsBaseShape(shape)) {
		if (workNif.CalcShapeDiff(shape, &fbxShape->verts, diff, 1.0f))
			return false;

		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.LoadSet(sliderData, target, diff);
	}
	else {
		if (workNif.CalcShapeDiff(shape, &fbxShape->verts, diff, 1.0f))
			return false;

		morpher.SetResultDiff(target, sliderName, diff);
	}

	return true;
}

void OutfitProject::SetSliderFromDiff(const std::string& sliderName, NiShape* shape, std::unordered_map<ushort, Vector3>& diff) {
	std::string target = ShapeToTarget(shape->GetName());
	if (IsBaseShape(shape)) {
		std::string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.LoadSet(sliderData, target, diff);
	}
	else {
		morpher.EmptyResultDiff(target, sliderName);
		morpher.SetResultDiff(target, sliderName, diff);
	}
}

int OutfitProject::GetVertexCount(NiShape* shape) {
	if (workNif.IsValid()) {
		if (shape)
			return shape->GetNumVertices();
	}

	return 0;
}

void OutfitProject::GetLiveVerts(NiShape* shape, std::vector<Vector3>& outVerts, std::vector<Vector2>* outUVs) {
	workNif.GetVertsForShape(shape, outVerts);
	if (outUVs)
		workNif.GetUvsForShape(shape, *outUVs);

	std::string target = ShapeToTarget(shape->GetName());
	if (IsBaseShape(shape)) {
		for (int i = 0; i < activeSet.size(); i++) {
			if (activeSet[i].bShow && activeSet[i].curValue != 0.0f) {
				std::string targetData = activeSet.ShapeToDataName(i, shape->GetName());
				if (targetData.empty())
					continue;

				if (activeSet[i].bUV) {
					if (outUVs)
						baseDiffData.ApplyUVDiff(targetData, target, activeSet[i].curValue, outUVs);
				}
				else
					baseDiffData.ApplyDiff(targetData, target, activeSet[i].curValue, &outVerts);
			}
		}
	}
	else {
		for (int i = 0; i < activeSet.size(); i++) {
			if (activeSet[i].bShow && activeSet[i].curValue != 0.0f) {
				if (activeSet[i].bUV) {
					if (outUVs)
						morpher.ApplyResultToUVs(activeSet[i].name, target, outUVs, activeSet[i].curValue);
				}
				else
					morpher.ApplyResultToVerts(activeSet[i].name, target, &outVerts, activeSet[i].curValue);
			}
		}
	}
}

void OutfitProject::GetSliderDiff(NiShape* shape, const std::string& sliderName, std::vector<Vector3>& outVerts) {
	int sliderIndex = SliderIndexFromName(sliderName);
	if (sliderIndex < 0)
		return;

	std::string target = ShapeToTarget(shape->GetName());
	if (IsBaseShape(shape)) {
		std::string targetData = activeSet.ShapeToDataName(sliderIndex, shape->GetName());
		if (targetData.empty())
			return;

		if (!activeSet[sliderIndex].bUV)
			baseDiffData.ApplyDiff(targetData, target, 1.0f, &outVerts);
	}
	else {
		if (!activeSet[sliderIndex].bUV)
			morpher.ApplyResultToVerts(activeSet[sliderIndex].name, target, &outVerts);
	}
}

void OutfitProject::GetSliderDiffUV(NiShape* shape, const std::string& sliderName, std::vector<Vector2>& outUVs) {
	int sliderIndex = SliderIndexFromName(sliderName);
	if (sliderIndex < 0)
		return;

	std::string target = ShapeToTarget(shape->GetName());
	if (IsBaseShape(shape)) {
		std::string targetData = activeSet.ShapeToDataName(sliderIndex, shape->GetName());
		if (targetData.empty())
			return;

		if (activeSet[sliderIndex].bUV)
			baseDiffData.ApplyUVDiff(targetData, target, 1.0f, &outUVs);
	}
	else {
		if (activeSet[sliderIndex].bUV)
			morpher.ApplyResultToUVs(activeSet[sliderIndex].name, target, &outUVs);
	}
}

const std::string& OutfitProject::ShapeToTarget(const std::string& shapeName) {
	for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it)
		if (it->first == shapeName)
			return it->second.targetShape;

	return shapeName;
}

void OutfitProject::GetActiveBones(std::vector<std::string>& outBoneNames) {
	AnimSkeleton::getInstance().GetActiveBoneNames(outBoneNames);
}

std::vector<std::string> OutfitProject::GetShapeTextures(NiShape* shape) {
	std::string shapeName = shape->GetName();

	if (shapeTextures.find(shapeName) != shapeTextures.end())
		return shapeTextures[shapeName];

	return std::vector<std::string>();
}

bool OutfitProject::GetShapeMaterialFile(NiShape* shape, MaterialFile& outMatFile) {
	std::string shapeName = shape->GetName();

	if (shapeMaterialFiles.find(shapeName) != shapeMaterialFiles.end()) {
		outMatFile = shapeMaterialFiles[shapeName];
		return true;
	}

	return false;
}

void OutfitProject::SetTextures() {
	for (auto &s : workNif.GetShapes())
		SetTextures(s);
}

void OutfitProject::SetTextures(const std::vector<std::string>& textureFiles) {
	for (auto &s : workNif.GetShapes())
		SetTextures(s, textureFiles);
}

void OutfitProject::SetTextures(NiShape* shape, const std::vector<std::string>& textureFiles) {
	if (!shape)
		return;

	std::string shapeName = shape->GetName();
	if (shapeName.empty())
		return;

	if (textureFiles.empty()) {
		std::string texturesDir = Config["GameDataPath"];
		bool hasMat = false;
		std::string matFile;

		const byte MAX_TEXTURE_PATHS = 10;
		std::vector<std::string> texFiles(MAX_TEXTURE_PATHS);

		NiShader* shader = workNif.GetShader(shape);
		if (shader) {
			// Find material file
			if (workNif.GetHeader().GetVersion().User() == 12 && workNif.GetHeader().GetVersion().Stream() >= 130) {
				matFile = shader->GetName();
				if (!matFile.empty())
					hasMat = true;
			}
		}

		shapeMaterialFiles.erase(shapeName);

		MaterialFile mat(MaterialFile::BGSM);
		if (hasMat) {
			matFile = std::regex_replace(matFile, std::regex("\\\\+"), "/");													// Replace all backward slashes with one forward slash
			matFile = std::regex_replace(matFile, std::regex("^(.*?)/materials/", std::regex_constants::icase), "");			// Remove everything before the first occurence of "/materials/"
			matFile = std::regex_replace(matFile, std::regex("^/+"), "");														// Remove all slashes from the front
			matFile = std::regex_replace(matFile, std::regex("^(?!^materials/)", std::regex_constants::icase), "materials/");	// If the path doesn't start with "materials/", add it to the front

			// Attempt to read loose material file
			mat = MaterialFile(texturesDir + matFile);

			if (mat.Failed()) {
				// Search for material file in archives
				wxMemoryBuffer data;
				for (FSArchiveFile *archive : FSManager::archiveList()) {
					if (archive) {
						if (archive->hasFile(matFile)) {
							wxMemoryBuffer outData;
							archive->fileContents(matFile, outData);

							if (!outData.IsEmpty()) {
								data = std::move(outData);
								break;
							}
						}
					}
				}

				if (!data.IsEmpty()) {
					std::string content((char*)data.GetData(), data.GetDataLen());
					std::istringstream contentStream(content, std::istringstream::binary);

					mat = MaterialFile(contentStream);
				}
			}

			if (!mat.Failed()) {
				if (mat.signature == MaterialFile::BGSM) {
					texFiles[0] = mat.diffuseTexture.c_str();
					texFiles[1] = mat.normalTexture.c_str();
					texFiles[2] = mat.glowTexture.c_str();
					texFiles[3] = mat.greyscaleTexture.c_str();
					texFiles[4] = mat.envmapTexture.c_str();
					texFiles[7] = mat.smoothSpecTexture.c_str();
				}
				else if (mat.signature == MaterialFile::BGEM) {
					texFiles[0] = mat.baseTexture.c_str();
					texFiles[1] = mat.fxNormalTexture.c_str();
					texFiles[3] = mat.grayscaleTexture.c_str();
					texFiles[4] = mat.fxEnvmapTexture.c_str();
					texFiles[5] = mat.envmapMaskTexture.c_str();
				}

				shapeMaterialFiles[shapeName] = std::move(mat);
			}
			else if (shader) {
				for (int i = 0; i < MAX_TEXTURE_PATHS; i++)
					workNif.GetTextureSlot(shader, texFiles[i], i);
			}
		}
		else if (shader) {
			for (int i = 0; i < MAX_TEXTURE_PATHS; i++)
				workNif.GetTextureSlot(shader, texFiles[i], i);
		}

		for (int i = 0; i < MAX_TEXTURE_PATHS; i++) {
			if (!texFiles[i].empty()) {
				texFiles[i] = std::regex_replace(texFiles[i], std::regex("\\\\+"), "/");													// Replace all backward slashes with one forward slash
				texFiles[i] = std::regex_replace(texFiles[i], std::regex("^(.*?)/textures/", std::regex_constants::icase), "");				// Remove everything before the first occurence of "/textures/"
				texFiles[i] = std::regex_replace(texFiles[i], std::regex("^/+"), "");														// Remove all slashes from the front
				texFiles[i] = std::regex_replace(texFiles[i], std::regex("^(?!^textures/)", std::regex_constants::icase), "textures/");		// If the path doesn't start with "textures/", add it to the front

				texFiles[i] = texturesDir + texFiles[i];
			}
		}

		shapeTextures[shapeName] = texFiles;
	}
	else
		shapeTextures[shapeName] = textureFiles;
}

bool OutfitProject::IsValidShape(const std::string& shapeName) {
	for (auto &s : workNif.GetShapeNames())
		if (s == shapeName)
			return true;

	return false;
}

void OutfitProject::RefreshMorphShape(NiShape* shape) {
	morpher.UpdateMeshFromNif(workNif, shape->GetName());
}

void OutfitProject::UpdateShapeFromMesh(NiShape* shape, const mesh* m) {
	std::vector<Vector3> liveVerts(m->nVerts);

	for (int i = 0; i < m->nVerts; i++) {
		auto& vertex = m->verts[i];
		liveVerts[i] = std::move(Vector3(vertex.x * -10.0f, vertex.z * 10.0f, vertex.y * 10.0f));
	}

	workNif.SetVertsForShape(shape, liveVerts);
}

void OutfitProject::UpdateMorphResult(NiShape* shape, const std::string& sliderName, std::unordered_map<ushort, Vector3>& vertUpdates) {
	// Morph results are stored in two different places depending on whether it's an outfit or the base shape.
	// The outfit morphs are stored in the automorpher, whereas the base shape diff info is stored in directly in basediffdata.
	
	std::string target = ShapeToTarget(shape->GetName());
	std::string dataName = activeSet[sliderName].TargetDataName(target);
	if (!vertUpdates.empty()) {
		if (dataName.empty())
			activeSet[sliderName].AddDataFile(target, target + sliderName, target + sliderName);
		else
			activeSet[sliderName].SetLocalData(dataName);
	}

	if (IsBaseShape(shape)) {
		for (auto &i : vertUpdates) {
			Vector3 diffscale = Vector3(i.second.x * -10, i.second.z * 10, i.second.y * 10);
			baseDiffData.SumDiff(dataName, target, i.first, diffscale);
		}
	}
	else
		morpher.UpdateResultDiff(shape->GetName(), sliderName, vertUpdates);
}

void OutfitProject::ScaleMorphResult(NiShape* shape, const std::string& sliderName, float scaleValue) {
	if (IsBaseShape(shape)) {
		std::string target = ShapeToTarget(shape->GetName());
		std::string dataName = activeSet[sliderName].TargetDataName(target);
		baseDiffData.ScaleDiff(dataName, target, scaleValue);
	}
	else
		morpher.ScaleResultDiff(shape->GetName(), sliderName, scaleValue);
}

void OutfitProject::MoveVertex(NiShape* shape, const Vector3& pos, const int& id) {
	workNif.MoveVertex(shape, pos, id);
}

void OutfitProject::OffsetShape(NiShape* shape, const Vector3& xlate, std::unordered_map<ushort, float>* mask) {
	workNif.OffsetShape(shape, xlate, mask);
}

void OutfitProject::ScaleShape(NiShape* shape, const Vector3& scale, std::unordered_map<ushort, float>* mask) {
	workNif.ScaleShape(shape, scale, mask);
}

void OutfitProject::RotateShape(NiShape* shape, const Vector3& angle, std::unordered_map<ushort, float>* mask) {
	workNif.RotateShape(shape, angle, mask);
}

bool OutfitProject::AddShapeBoneAndXForm(const std::string &shapeName, const std::string &boneName) {
	if (!workAnim.AddShapeBone(shapeName, boneName))
		return false;
	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");
	if (targetGame == FO4 || targetGame == FO4VR) {
		// Fallout 4 bone transforms are stored in a bonedata structure per shape versus the node transform in the skeleton data.
		MatTransform xForm;
		workNif.GetShapeBoneTransform(baseShape, boneName, xForm);
		workAnim.SetShapeBoneXForm(shapeName, boneName, xForm);
	}
	else {
		MatTransform xForm;
		workAnim.GetBoneXForm(boneName, xForm);
		workAnim.SetShapeBoneXForm(shapeName, boneName, xForm);
	}
	return true;
}

void OutfitProject::CopyBoneWeights(NiShape* shape, const float proximityRadius, const int maxResults, std::unordered_map<ushort, float>& mask, const std::vector<std::string>& boneList, int nCopyBones, const std::vector<std::string> &lockedBones, UndoStateShape &uss, bool bSpreadWeight) {
	if (!shape || !baseShape)
		return;

	std::string shapeName = shape->GetName();
	std::string baseShapeName = baseShape->GetName();

	owner->UpdateProgress(1, _("Gathering bones..."));

	int nBones = boneList.size();
	if (nBones <= 0 || nCopyBones <= 0) {
		owner->UpdateProgress(90);
		return;
	}

	DiffDataSets dds;
	for (int bi = 0; bi < nCopyBones; ++bi) {
		const std::string &bone = boneList[bi];
		std::string wtSet = bone + "_WT_";
		dds.AddEmptySet(wtSet, "Weight");

		auto weights = workAnim.GetWeightsPtr(baseShapeName, bone);
		if (weights) {
			for (auto &w : *weights) {
				Vector3 tmp;
				tmp.y = w.second;
				dds.UpdateDiff(wtSet, "Weight", w.first, tmp);
			}
		}
	}

	BoneWeightAutoNormalizer nzer;
	nzer.SetUp(&uss, &workAnim, shapeName, boneList, lockedBones, nCopyBones, bSpreadWeight);
	std::unordered_set<int> vertList;

	owner->UpdateProgress(10, _("Initializing proximity data..."));

	InitConform();
	morpher.LinkRefDiffData(&dds);
	morpher.BuildProximityCache(shapeName, proximityRadius);
	workNif.CreateSkinning(shape);

	int step = 40 / nCopyBones;
	int prog = 40;
	owner->UpdateProgress(prog);

	for (unsigned int bi = 0; bi < nCopyBones; ++bi) {
		const std::string &boneName = boneList[bi];
		auto &ubw = uss.boneWeights[bi].weights;
		// Zero out unmasked weights
		auto weights = workAnim.GetWeightsPtr(shapeName, boneName);
		if (weights) {
			for (auto &pi : *weights) {
				if (mask[pi.first] > 0.0f) continue;
				if (vertList.find(pi.first) == vertList.end()) {
					vertList.insert(pi.first);
					nzer.GrabOneVertexStartingWeights(pi.first);
				}
				ubw[pi.first].endVal = 0.0;
			}
		}

		// Calculate new values for bone's weights
		std::string wtSet = boneName + "_WT_";
		morpher.GenerateResultDiff(shapeName, wtSet, wtSet, maxResults);

		std::unordered_map<ushort, Vector3> diffResult;
		morpher.GetRawResultDiff(shapeName, wtSet, diffResult);

		// Copy unmasked weights from diffResult into uss
		for (auto &dr : diffResult) {
			if (mask[dr.first] > 0.0f) continue;
			if (vertList.find(dr.first) == vertList.end()) {
				vertList.insert(dr.first);
				nzer.GrabOneVertexStartingWeights(dr.first);
			}
			ubw[dr.first].endVal = dr.second.y;
		}

		owner->UpdateProgress(prog += step, _("Copying bone weights..."));
	}
	morpher.UnlinkRefDiffData();

	// Normalize
	for (auto vInd : vertList)
		nzer.AdjustWeights(vInd);

	owner->UpdateProgress(90);
}

void OutfitProject::TransferSelectedWeights(NiShape* shape, std::unordered_map<ushort, float>* mask, std::vector<std::string>* inBoneList) {
	if (!shape || !baseShape)
		return;

	std::string shapeName = shape->GetName();
	std::string baseShapeName = baseShape->GetName();

	owner->UpdateProgress(10, _("Gathering bones..."));

	std::vector<std::string>* boneList;
	std::vector<std::string> allBoneList;
	if (!inBoneList) {
		for (auto &boneName : workAnim.shapeBones[baseShapeName])
			allBoneList.push_back(boneName);

		boneList = &allBoneList;
	}
	else
		boneList = inBoneList;

	if (boneList->size() <= 0) {
		owner->UpdateProgress(100, _("Finished"));
		return;
	}

	int step = 50 / boneList->size();
	int prog = 40;
	owner->UpdateProgress(prog, _("Transferring bone weights..."));

	for (auto &boneName : *boneList) {
		std::unordered_map<ushort, float> weights;
		std::unordered_map<ushort, float> oldWeights;
		workAnim.GetWeights(baseShapeName, boneName, weights);
		workAnim.GetWeights(shapeName, boneName, oldWeights);

		for (auto &w : weights) {
			if (mask) {
				if (1.0f - (*mask)[w.first] > 0.0f)
					weights[w.first] = w.second * (1.0f - (*mask)[w.first]);
				else
					weights[w.first] = oldWeights[w.first];
			}
			else
				weights[w.first] = w.second;
		}

		AddShapeBoneAndXForm(shapeName, boneName);
		workAnim.SetWeights(shapeName, boneName, weights);
		owner->UpdateProgress(prog += step, "");
	}

	owner->UpdateProgress(100, _("Finished"));
}

bool OutfitProject::HasUnweighted(std::vector<std::string>* shapeNames) {
	bool hasUnweighted = false;

	for (auto &shape : workNif.GetShapes()) {
		if (!shape || !shape->IsSkinned())
			continue;

		std::string shapeName = shape->GetName();
		std::vector<Vector3> verts;
		workNif.GetVertsForShape(shape, verts);

		std::unordered_map<int, int> influences;
		for (int i = 0; i < verts.size(); i++)
			influences.emplace(i, 0);

		if (workAnim.shapeBones.find(shapeName) != workAnim.shapeBones.end()) {
			for (auto &b : workAnim.shapeBones[shapeName]) {
				auto weights = workAnim.GetWeightsPtr(shapeName, b);
				if (weights) {
					for (int i = 0; i < verts.size(); i++) {
						auto id = weights->find(i);
						if (id != weights->end() && id->second > 0.0f)
							influences.at(i)++;
					}
				}
			}
		}

		bool shapeUnweighted = false;
		mesh* m = owner->glView->GetMesh(shapeName);
		if (m) {
			for (auto &i : influences) {
				if (i.second == 0) {
					if (!shapeUnweighted)
						m->ColorChannelFill(0, 0.0f);

					m->vcolors[i.first].x = 1.0f;
					shapeUnweighted = true;
				}
			}

			if (shapeUnweighted) {
				hasUnweighted = true;

				if (shapeNames)
					shapeNames->push_back(shapeName);
			}

			m->QueueUpdate(mesh::UpdateType::VertexColors);
		}
	}

	return hasUnweighted;
}

void OutfitProject::ApplyBoneScale(const std::string& bone, int sliderPos, bool clear) {
	ClearBoneScale(false);

	for (auto &s : workNif.GetShapeNames()) {
		auto it = boneScaleVerts.find(s);
		if (it == boneScaleVerts.end()) {
			mesh* m = owner->glView->GetMesh(s);
			boneScaleVerts.emplace(s, std::vector<Vector3>(m->nVerts));
			it = boneScaleVerts.find(s);
			for (int i = 0; i < m->nVerts; i++) {
				auto& vertex = m->verts[i];
				it->second[i] = std::move(Vector3(vertex.x * -10.0f, vertex.z * 10.0f, vertex.y * 10.0f));
			}
		}

		std::vector<Vector3>* verts = &it->second;

		it = boneScaleOffsets.find(s);
		if (it == boneScaleOffsets.end())
			boneScaleOffsets.emplace(s, std::vector<Vector3>(verts->size()));
		it = boneScaleOffsets.find(s);

		for (auto &b : workAnim.shapeBones[s]) {
			if (b == bone) {
				MatTransform xform;
				workNif.GetNodeTransform(b, xform);

				auto weights = workAnim.GetWeightsPtr(s, b);
				if (weights) {
					for (auto &w : *weights) {
						Vector3 dir = (*verts)[w.first] - xform.translation;
						dir.Normalize();
						Vector3 offset = dir * w.second * sliderPos / 5.0f;
						(*verts)[w.first] += offset;
						it->second[w.first] += offset;
					}
				}
				break;
			}
		}

		if (clear)
			owner->glView->UpdateMeshVertices(s, verts, true, true, false);
		else
			owner->glView->UpdateMeshVertices(s, verts, true, false, false);
	}
}

void OutfitProject::ClearBoneScale(bool clear) {
	if (boneScaleOffsets.empty())
		return;

	for (auto &s : workNif.GetShapeNames()) {
		auto it = boneScaleVerts.find(s);
		std::vector<Vector3>* verts = &it->second;

		it = boneScaleOffsets.find(s);
		if (it != boneScaleOffsets.end()) {
			if (verts->size() == it->second.size()) {
				for (int i = 0; i < verts->size(); i++)
					(*verts)[i] -= it->second[i];

				if (clear)
					owner->glView->UpdateMeshVertices(s, verts, true, true, false);
				else
					owner->glView->UpdateMeshVertices(s, verts, false, false, false);
			}
		}
	}

	boneScaleVerts.clear();
	boneScaleOffsets.clear();
}

void OutfitProject::AddBoneRef(const std::string& boneName) {
	MatTransform xForm;
	if (!AnimSkeleton::getInstance().GetSkinTransform(boneName, xForm, xForm))
		return;

	for (auto &s : workNif.GetShapeNames())
		if (workAnim.AddShapeBone(s, boneName))
			workAnim.SetShapeBoneXForm(s, boneName, xForm);
}

void OutfitProject::AddCustomBoneRef(const std::string& boneName, const Vector3& translation) {
	AnimBone& customBone = AnimSkeleton::getInstance().AddBone(boneName, true);

	MatTransform xForm;
	xForm.translation = translation;

	customBone.trans = xForm.translation;
	customBone.scale = xForm.scale;

	for (auto &s : workNif.GetShapeNames())
		if (workAnim.AddShapeBone(s, boneName))
			workAnim.SetShapeBoneXForm(s, boneName, xForm);
}

void OutfitProject::ClearWorkSliders() {
	morpher.ClearResultDiff();
}

void OutfitProject::ClearReference() {
	DeleteShape(baseShape);

	if (activeSet.size() > 0)
		activeSet.Clear();
}

void OutfitProject::ClearOutfit() {
	for (auto &s : workNif.GetShapes()) {
		if (IsBaseShape(s))
			continue;

		DeleteShape(s);
	}
	ClearWorkSliders();
}

void OutfitProject::ClearSlider(NiShape* shape, const std::string& sliderName) {
	std::string target = ShapeToTarget(shape->GetName());

	if (IsBaseShape(shape)) {
		std::string data = activeSet[sliderName].TargetDataName(target);
		baseDiffData.EmptySet(data, target);
	}
	else
		morpher.EmptyResultDiff(target, sliderName);
}

void OutfitProject::ClearUnmaskedDiff(NiShape* shape, const std::string& sliderName, std::unordered_map<ushort, float>* mask) {
	std::string target = ShapeToTarget(shape->GetName());

	if (IsBaseShape(shape)) {
		std::string data = activeSet[sliderName].TargetDataName(target);
		baseDiffData.ZeroVertDiff(data, target, nullptr, mask);
	}
	else
		morpher.ZeroVertDiff(target, sliderName, nullptr, mask);
}

void OutfitProject::DeleteSlider(const std::string& sliderName) {
	for (auto &s : workNif.GetShapes()) {
		std::string target = ShapeToTarget(s->GetName());
		std::string data = activeSet[sliderName].TargetDataName(target);

		if (IsBaseShape(s))
			baseDiffData.ClearSet(data);
		else
			morpher.ClearResultSet(data);
	}

	activeSet.DeleteSlider(sliderName);
}

int OutfitProject::LoadSkeletonReference(const std::string& skeletonFileName) {
	return AnimSkeleton::getInstance().LoadFromNif(skeletonFileName);
}

int OutfitProject::LoadReferenceTemplate(const std::string& sourceFile, const std::string& set, const std::string& shape, bool loadAll, bool mergeSliders) {
	if (sourceFile.empty() || set.empty()) {
		wxLogError("Template source entries are invalid.");
		wxMessageBox(_("Template source entries are invalid."), _("Reference Error"), wxICON_ERROR, owner);
		return 1;
	}

	if (loadAll) {
		owner->StartSubProgress(10, 20);
		return AddFromSliderSet(sourceFile, set);
	}
	else
		return LoadReference(sourceFile, set, mergeSliders, shape);
}

int OutfitProject::LoadReferenceNif(const std::string& fileName, const std::string& shapeName, bool mergeSliders) {
	if (mergeSliders)
		DeleteShape(baseShape);
	else
		ClearReference();

	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::in | std::ios::binary);

	NifFile refNif;
	int error = refNif.Load(file);
	if (error) {
		if (error == 2) {
			wxString errorText = wxString::Format(_("NIF version not supported!\n\nFile: %s\n%s"),
				fileName, refNif.GetHeader().GetVersion().GetVersionInfo());

			wxLogError(errorText);
			wxMessageBox(errorText, _("Reference Error"), wxICON_ERROR, owner);
			return 3;
		}

		wxLogError("Could not load reference NIF file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Could not load reference NIF file '%s'!"), fileName), _("Reference Error"), wxICON_ERROR, owner);
		return 2;
	}

	ValidateNIF(refNif);

	auto refShapeDup = workNif.FindBlockByName<NiShape>(shapeName);
	auto refShape = refNif.FindBlockByName<NiShape>(shapeName);
	if (refShapeDup && refShape) {
		std::string newName = shapeName + "_ref";
		refNif.RenameShape(refShape, newName);
	}

	if (workNif.IsValid()) {
		// Copy only reference shape
		auto clonedShape = workNif.CloneShape(refShape, shapeName, &refNif);
		workAnim.LoadFromNif(&workNif, clonedShape);
	}
	else {
		// Copy the full file
		workNif.CopyFrom(refNif);
		workAnim.LoadFromNif(&workNif);

		// Delete all except for reference
		for (auto &s : workNif.GetShapes())
			if (s->GetName() != shapeName)
				DeleteShape(s);
	}

	baseShape = workNif.FindBlockByName<NiShape>(shapeName);
	activeSet.LoadSetDiffData(baseDiffData);
	return 0;
}

int OutfitProject::LoadReference(const std::string& fileName, const std::string& setName, bool mergeSliders, const std::string& shapeName) {
	if (mergeSliders)
		DeleteShape(baseShape);
	else
		ClearReference();

	SliderSetFile sset(fileName);
	if (sset.fail()) {
		wxLogError("Could not load slider set file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Could not load slider set file '%s'!"), fileName), _("Reference Error"), wxICON_ERROR, owner);
		return 1;
	}

	std::string dataFolder = activeSet.GetDefaultDataFolder();
	std::vector<std::string> dataNames = activeSet.GetLocalData(shapeName);

	sset.GetSet(setName, activeSet);

	activeSet.SetBaseDataPath(Config["AppDir"] + PathSepStr + "ShapeData");
	std::string refFile = activeSet.GetInputFileName();

	std::fstream file;
	PlatformUtil::OpenFileStream(file, refFile, std::ios::in | std::ios::binary);

	NifFile refNif;
	int error = refNif.Load(file);
	if (error) {
		if (error == 2) {
			wxString errorText = wxString::Format(_("NIF version not supported!\n\nFile: %s\n%s"),
				refFile, refNif.GetHeader().GetVersion().GetVersionInfo());

			wxLogError(errorText);
			wxMessageBox(errorText, _("Reference Error"), wxICON_ERROR, owner);
			ClearReference();
			return 5;
		}

		ClearReference();
		wxLogError("Could not load reference NIF file '%s'!", refFile);
		wxMessageBox(wxString::Format(_("Could not load reference NIF file '%s'!"), refFile), _("Reference Error"), wxICON_ERROR, owner);
		return 2;
	}

	ValidateNIF(refNif);

	std::vector<std::string> shapes = refNif.GetShapeNames();
	if (shapes.empty()) {
		ClearReference();
		wxLogError("Reference NIF file '%s' does not contain any shapes.", refFile);
		wxMessageBox(wxString::Format(_("Reference NIF file '%s' does not contain any shapes."), refFile), _("Reference Error"), wxICON_ERROR, owner);
		return 3;
	}

	std::string shape = shapeName;
	if (shape.empty())
		shape = shapes[0];

	auto refShape = refNif.FindBlockByName<NiShape>(shape);
	if (!refShape) {
		ClearReference();
		wxLogError("Shape '%s' not found in reference NIF file '%s'!", shape, refFile);
		wxMessageBox(wxString::Format(_("Shape '%s' not found in reference NIF file '%s'!"), shape, refFile), _("Reference Error"), wxICON_ERROR, owner);
		return 4;
	}

	auto refShapeDup = workNif.FindBlockByName<NiShape>(shape);
	if (refShapeDup) {
		std::string newName = shape + "_ref";
		refNif.RenameShape(refShape, newName);
	}

	// Add cloth data block of NIF to the list
	std::vector<BSClothExtraData*> clothDataBlocks = refNif.GetChildren<BSClothExtraData>(nullptr, true);
	for (auto &cloth : clothDataBlocks)
		clothData[refFile] = cloth->Clone();

	refNif.GetHeader().DeleteBlockByType("BSClothExtraData");

	if (workNif.IsValid()) {
		// Copy only reference shape
		auto clonedShape = workNif.CloneShape(refShape, shape, &refNif);
		workAnim.LoadFromNif(&workNif, clonedShape);
	}
	else {
		// Copy the full file
		workNif.CopyFrom(refNif);
		workAnim.LoadFromNif(&workNif);

		// Delete all except for reference
		for (auto &s : workNif.GetShapes())
			if (s->GetName() != shape)
				DeleteShape(s);
	}

	baseShape = workNif.FindBlockByName<NiShape>(shape);

	if (mergeSliders)
		activeSet.LoadSetDiffData(baseDiffData, shape);
	else
		activeSet.LoadSetDiffData(baseDiffData);

	activeSet.SetReferencedData(shape);
	for (auto &dn : dataNames)
		activeSet.SetReferencedDataByName(shape, dn, true);

	// Keep default data folder from current project if existing
	if (!dataFolder.empty())
		activeSet.SetDataFolder(dataFolder);

	return 0;
}

int OutfitProject::LoadFromSliderSet(const std::string& fileName, const std::string& sliderSetName, std::vector<std::string>* origShapeOrder) {
	owner->StartProgress(_("Loading slider set..."));
	SliderSetFile InSS(fileName);
	if (InSS.fail()) {
		owner->EndProgress();
		return 1;
	}

	owner->UpdateProgress(20, _("Retrieving sliders..."));
	if (InSS.GetSet(sliderSetName, activeSet)) {
		owner->EndProgress();
		return 3;
	}

	activeSet.SetBaseDataPath(Config["AppDir"] + PathSepStr + "ShapeData");

	std::string inputNif = activeSet.GetInputFileName();

	owner->UpdateProgress(30, _("Loading outfit shapes..."));
	if (ImportNIF(inputNif, true, sliderSetName)) {
		owner->EndProgress();
		return 4;
	}

	if (origShapeOrder)
		*origShapeOrder = workNif.GetShapeNames();

	NiShape* newBaseShape = nullptr;

	// First external target with skin shader becomes reference
	std::vector<std::string> refTargets;
	activeSet.GetReferencedTargets(refTargets);
	for (auto &target : refTargets) {
		std::string shapeName = activeSet.TargetToShape(target);
		auto shape = workNif.FindBlockByName<NiShape>(shapeName);
		if (shape) {
			NiShader* shader = workNif.GetShader(shape);
			if (shader && shader->IsSkinTinted()) {
				newBaseShape = shape;
				break;
			}
		}
	}

	// No external target found, first skin shaded shape becomes reference
	if (refTargets.empty()) {
		for (auto shapeTarget = activeSet.ShapesBegin(); shapeTarget != activeSet.ShapesEnd(); ++shapeTarget) {
			auto shape = workNif.FindBlockByName<NiShape>(shapeTarget->first);
			if (shape) {
				NiShader* shader = workNif.GetShader(shape);
				if (shader && shader->IsSkinTinted()) {
					newBaseShape = shape;
					break;
				}
			}
		}
	}

	// Store base shape for later deletion
	baseShape = newBaseShape;

	owner->UpdateProgress(90, _("Updating slider data..."));
	morpher.LoadResultDiffs(activeSet);

	wxString rest;
	mFileName = wxString::FromUTF8(fileName);
	if (mFileName.EndsWith(".xml", &rest))
		mFileName = rest.Append(".osp");

	mOutfitName = wxString::FromUTF8(sliderSetName);
	mDataDir = wxString::FromUTF8(activeSet.GetDefaultDataFolder());
	mBaseFile = wxString::FromUTF8(activeSet.GetInputFileName());
	mBaseFile = mBaseFile.AfterLast('/').AfterLast('\\');

	mGamePath = wxString::FromUTF8(activeSet.GetOutputPath());
	mGameFile = wxString::FromUTF8(activeSet.GetOutputFile());
	mCopyRef = true;
	mGenWeights = activeSet.GenWeights();

	owner->UpdateProgress(100, _("Finished"));
	owner->EndProgress();
	return 0;
}

int OutfitProject::AddFromSliderSet(const std::string& fileName, const std::string& sliderSetName) {
	owner->StartProgress(_("Adding slider set..."));
	SliderSetFile InSS(fileName);
	if (InSS.fail()) {
		owner->EndProgress();
		return 1;
	}

	SliderSet addSet;
	owner->UpdateProgress(20, _("Retrieving sliders..."));
	if (InSS.GetSet(sliderSetName, addSet)) {
		owner->EndProgress();
		return 2;
	}

	addSet.SetBaseDataPath(Config["AppDir"] + PathSepStr + "ShapeData");
	std::string inputNif = addSet.GetInputFileName();

	std::map<std::string, std::string> renamedShapes;
	owner->UpdateProgress(30, _("Adding outfit shapes..."));
	if (ImportNIF(inputNif, false, "", &renamedShapes)) {
		owner->EndProgress();
		return 3;
	}

	if (!baseShape) {
		NiShape* newBaseShape = nullptr;

		// First external target with skin shader becomes reference
		std::vector<std::string> refTargets;
		addSet.GetReferencedTargets(refTargets);
		for (auto &target : refTargets) {
			std::string shapeName = addSet.TargetToShape(target);
			auto shape = workNif.FindBlockByName<NiShape>(shapeName);
			if (shape) {
				NiShader* shader = workNif.GetShader(shape);
				if (shader && shader->IsSkinTinted()) {
					newBaseShape = shape;
					break;
				}
			}
		}

		// No external target found, first skin shaded shape becomes reference
		if (refTargets.empty()) {
			for (auto shapeTarget = addSet.ShapesBegin(); shapeTarget != addSet.ShapesEnd(); ++shapeTarget) {
				auto shape = workNif.FindBlockByName<NiShape>(shapeTarget->first);
				if (shape) {
					NiShader* shader = workNif.GetShader(shape);
					if (shader && shader->IsSkinTinted()) {
						newBaseShape = shape;
						break;
					}
				}
			}
		}

		baseShape = newBaseShape;
	}

	if (!renamedShapes.empty()) {
		std::vector<std::string> renamedShapesOrig;
		renamedShapesOrig.reserve(renamedShapes.size());
		for (auto &rs : renamedShapes)
			renamedShapesOrig.push_back(rs.first);

		std::string shapesJoin = JoinStrings(renamedShapesOrig, "; ");
		wxMessageBox(wxString::Format("%s\n \n%s", _("The following shapes were renamed and won't have slider data attached. Rename the duplicates yourself beforehand."), shapesJoin), _("Renamed Shapes"), wxOK | wxICON_WARNING, owner);
	}

	owner->UpdateProgress(70, _("Updating slider data..."));
	morpher.MergeResultDiffs(activeSet, addSet, baseDiffData, baseShape ? baseShape->GetName() : "");

	owner->UpdateProgress(100, _("Finished"));
	owner->EndProgress();
	return 0;
}

void OutfitProject::InitConform() {
	if (baseShape) {
		morpher.SetRef(workNif, baseShape);
		morpher.LinkRefDiffData(&baseDiffData);
		morpher.SourceShapesFromNif(workNif);
	}
}

void OutfitProject::ConformShape(NiShape* shape, const float proximityRadius, const int maxResults) {
	if (!workNif.IsValid() || !baseShape)
		return;

	morpher.BuildProximityCache(shape->GetName(), proximityRadius);

	std::string refTarget = ShapeToTarget(baseShape->GetName());
	for (int i = 0; i < activeSet.size(); i++)
		if (SliderShow(i) && !SliderZap(i) && !SliderUV(i))
			morpher.GenerateResultDiff(shape->GetName(), activeSet[i].name, activeSet[i].TargetDataName(refTarget), maxResults);
}

bool OutfitProject::DeleteVerts(NiShape* shape, const std::unordered_map<ushort, float>& mask) {
	std::vector<ushort> indices;
	indices.reserve(mask.size());

	for (auto &m : mask)
		indices.push_back(m.first);

	std::sort(indices.begin(), indices.end());
	indices.erase(std::unique(indices.begin(), indices.end()), indices.end());

	bool deleteShape = workNif.DeleteVertsForShape(shape, indices);
	if (!deleteShape) {
		workAnim.DeleteVertsForShape(shape->GetName(), indices);

		std::string target = ShapeToTarget(shape->GetName());
		if (IsBaseShape(shape))
			baseDiffData.DeleteVerts(target, indices);
		else
			morpher.DeleteVerts(target, indices);
		
		activeSet.SetReferencedData(shape->GetName(), true);
	}
	else
		DeleteShape(shape);

	return deleteShape;
}

NiShape* OutfitProject::DuplicateShape(NiShape* sourceShape, const std::string& destShapeName) {
	if (!sourceShape)
		return nullptr;

	auto newShape = workNif.CloneShape(sourceShape, destShapeName);
	workAnim.LoadFromNif(&workNif, newShape);

	std::string shapeName = sourceShape->GetName();
	std::string srcTarget = ShapeToTarget(shapeName);

	if (IsBaseShape(sourceShape)) {
		for (int i = 0; i < activeSet.size(); i++) {
			std::string srcTargetData = activeSet[i].TargetDataName(srcTarget);

			auto diff = baseDiffData.GetDiffSet(srcTargetData);
			if (diff)
				morpher.SetResultDiff(destShapeName, activeSet[i].name, *diff);
		}
	}
	else
		morpher.CopyShape(shapeName, srcTarget, destShapeName);

	return newShape;
}

void OutfitProject::DeleteShape(NiShape* shape) {
	if (!shape)
		return;

	std::string shapeName = shape->GetName();
	workAnim.ClearShape(shapeName);
	owner->glView->DeleteMesh(shapeName);
	shapeTextures.erase(shapeName);
	shapeMaterialFiles.erase(shapeName);

	if (IsBaseShape(shape)) {
		morpher.UnlinkRefDiffData();
		baseShape = nullptr;
	}

	owner->ClearSelected(shape);
	workNif.DeleteShape(shape);
}

void OutfitProject::RenameShape(NiShape* shape, const std::string& newShapeName) {
	std::string shapeName = shape->GetName();
	std::string oldTarget = ShapeToTarget(shapeName);
	workNif.RenameShape(shape, newShapeName);
	workAnim.RenameShape(shapeName, newShapeName);
	activeSet.RenameShape(shapeName, newShapeName);
	
	auto tex = shapeTextures.find(shapeName);
	if (tex != shapeTextures.end()) {
		auto value = tex->second;
		shapeTextures.erase(tex);
		shapeTextures[newShapeName] = value;
	}

	auto mat = shapeMaterialFiles.find(shapeName);
	if (mat != shapeMaterialFiles.end()) {
		auto value = mat->second;
		shapeMaterialFiles.erase(mat);
		shapeMaterialFiles[newShapeName] = value;
	}

	if (IsBaseShape(shape)) {
		activeSet.SetReferencedData(newShapeName, true);
		baseDiffData.DeepRename(oldTarget, newShapeName);
	}
	else
		morpher.RenameShape(shapeName, oldTarget, newShapeName);

	wxLogMessage("Renamed shape '%s' to '%s'.", shapeName, newShapeName);
}

void OutfitProject::UpdateNifNormals(NifFile* nif, const std::vector<mesh*>& shapeMeshes) {
	std::vector<Vector3> liveNorms;
	for (auto &m : shapeMeshes) {
		auto shape = nif->FindBlockByName<NiShape>(m->shapeName);
		if (shape) {
			if (nif->GetHeader().GetVersion().IsSK() || nif->GetHeader().GetVersion().IsSSE()) {
				NiShader* shader = nif->GetShader(shape);
				if (shader && shader->IsModelSpace())
					continue;
			}

			liveNorms.clear();
			for (int i = 0; i < m->nVerts; i++)
				liveNorms.emplace_back(std::move(Vector3(m->norms[i].x* -1, m->norms[i].z, m->norms[i].y)));

			nif->SetNormalsForShape(shape, liveNorms);
			nif->CalcTangentsForShape(shape);
		}
	}
}

int OutfitProject::ImportNIF(const std::string& fileName, bool clear, const std::string& inOutfitName, std::map<std::string, std::string>* renamedShapes) {
	if (clear)
		ClearOutfit();

	if (fileName.empty()) {
		wxLogMessage("No outfit selected.");
		return 0;
	}

	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");

	if (!inOutfitName.empty())
		outfitName = inOutfitName;
	else if (outfitName.empty())
		outfitName = "New Outfit";

	if (clear) {
		wxFileName file(fileName);
		mGameFile = file.GetName();
		mGamePath = file.GetPath();

		int pos = mGamePath.Lower().Find("meshes");
		if (pos != wxNOT_FOUND)
			mGamePath = mGamePath.Mid(pos);
		else
			mGamePath.Clear();

		if (targetGame == SKYRIM || targetGame == SKYRIMSE || targetGame == SKYRIMVR) {
			wxString fileRest;
			if (mGameFile.EndsWith("_0", &fileRest) || mGameFile.EndsWith("_1", &fileRest))
				mGameFile = fileRest;
		}
	}

	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::in | std::ios::binary);

	NifFile nif;
	int error = nif.Load(file);
	if (error) {
		if (error == 2) {
			wxString errorText = wxString::Format(_("NIF version not supported!\n\nFile: %s\n%s"),
				fileName, nif.GetHeader().GetVersion().GetVersionInfo());

			wxLogError(errorText);
			wxMessageBox(errorText, _("NIF Error"), wxICON_ERROR, owner);
			return 4;
		}

		wxLogError("Could not load NIF file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Could not load NIF file '%s'!"), fileName), _("NIF Error"), wxICON_ERROR, owner);
		return 1;
	}

	ValidateNIF(nif);

	nif.SetNodeName(0, "Scene Root");
	nif.RenameDuplicateShapes();

	if (baseShape) {
		std::string baseShapeName = baseShape->GetName();
		auto bshape = nif.FindBlockByName<NiShape>(baseShapeName);
		if (nif.RenameShape(bshape, baseShapeName + "_outfit")) {
			if (renamedShapes)
				(*renamedShapes)[baseShapeName] = baseShapeName + "_outfit";
		}
	}

	std::vector<std::string> shapes = workNif.GetShapeNames();
	auto nifShapes = nif.GetShapes();

	for (auto &s : nifShapes) {
		std::vector<std::string> uniqueShapes = nif.GetShapeNames();
		uniqueShapes.insert(uniqueShapes.end(), shapes.begin(), shapes.end());

		std::string shapeName = s->GetName();
		std::string newName = shapeName;
		int uniqueCount = 0;
		for (;;) {
			auto foundShape = find(uniqueShapes.begin(), uniqueShapes.end(), newName);
			if (foundShape != uniqueShapes.end()) {
				uniqueShapes.erase(foundShape);
				uniqueCount++;
				if (uniqueCount > 1)
					newName = shapeName + wxString::Format("_%d", uniqueCount).ToStdString();
			}
			else {
				if (uniqueCount > 1) {
					if (nif.RenameShape(s, newName)) {
						if (renamedShapes)
							(*renamedShapes)[shapeName] = newName;
					}
				}
				break;
			}
		}
	}

	// Add cloth data block of NIF to the list
	std::vector<BSClothExtraData*> clothDataBlocks = nif.GetChildren<BSClothExtraData>(nullptr, true);
	for (auto &cloth : clothDataBlocks)
		clothData[fileName] = cloth->Clone();

	nif.GetHeader().DeleteBlockByType("BSClothExtraData");

	if (workNif.IsValid()) {
		for (auto &s : nif.GetShapes()) {
			std::string shapeName = s->GetName();
			auto clonedShape = workNif.CloneShape(s, shapeName, &nif);
			workAnim.LoadFromNif(&workNif, clonedShape);
		}
	}
	else {
		workNif.CopyFrom(nif);
		workAnim.LoadFromNif(&workNif);
	}

	return 0;
}

int OutfitProject::ExportNIF(const std::string& fileName, const std::vector<mesh*>& modMeshes, bool withRef) {
	workAnim.CleanupBones();
	owner->AnimationGUIFromProj();

	NifFile clone(workNif);
	ChooseClothData(clone);

	std::vector<Vector3> liveVerts;
	std::vector<Vector3> liveNorms;
	for (auto &m : modMeshes) {
		auto shape = clone.FindBlockByName<NiShape>(m->shapeName);
		if (shape) {
			liveVerts.clear();
			liveNorms.clear();

			for (int i = 0; i < m->nVerts; i++) {
				liveVerts.emplace_back(std::move(Vector3(m->verts[i].x * -10, m->verts[i].z * 10, m->verts[i].y * 10)));
				liveNorms.emplace_back(std::move(Vector3(m->norms[i].x * -1, m->norms[i].z, m->norms[i].y)));
			}

			clone.SetVertsForShape(shape, liveVerts);

			if (clone.GetHeader().GetVersion().IsSK() || clone.GetHeader().GetVersion().IsSSE()) {
				NiShader* shader = clone.GetShader(shape);
				if (shader && shader->IsModelSpace())
					continue;
			}

			clone.SetNormalsForShape(shape, liveNorms);
			clone.CalcTangentsForShape(shape);
		}
	}

	if (!withRef && baseShape) {
		std::string baseShapeName = baseShape->GetName();
		auto bshape = clone.FindBlockByName<NiShape>(baseShapeName);
		clone.DeleteShape(bshape);
		workAnim.WriteToNif(&clone, baseShapeName);
	}
	else
		workAnim.WriteToNif(&clone);

	for (auto &s : clone.GetShapes())
		clone.UpdateSkinPartitions(s);

	clone.SetShapeOrder(owner->GetShapeList());
	clone.GetHeader().SetExportInfo("Exported using Outfit Studio.");

	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::out | std::ios::binary);

	return clone.Save(file);
}


void OutfitProject::ChooseClothData(NifFile& nif) {
	if (!clothData.empty()) {
		wxArrayString clothFileNames;
		for (auto &cloth : clothData)
			clothFileNames.Add(wxString::FromUTF8(cloth.first));

		wxMultiChoiceDialog clothDataChoice(owner, _("There was cloth physics data loaded at some point (BSClothExtraData). Please choose all the origins to use in the output."), _("Choose cloth data"), clothFileNames);
		if (clothDataChoice.ShowModal() == wxID_CANCEL)
			return;

		wxArrayInt sel = clothDataChoice.GetSelections();
		for (int i = 0; i < sel.Count(); i++) {
			std::string selString{clothFileNames[sel[i]].ToUTF8()};
			if (!selString.empty()) {
				auto clothBlock = clothData[selString]->Clone();
				int id = nif.GetHeader().AddBlock(clothBlock);
				if (id != 0xFFFFFFFF) {
					auto root = nif.GetRootNode();
					if (root)
						root->GetExtraData().AddBlockRef(id);
				}
			}
		}
	}
}

int OutfitProject::ExportShapeNIF(const std::string& fileName, const std::vector<std::string>& exportShapes) {
	if (exportShapes.empty())
		return 1;

	if (!workNif.IsValid())
		return 2;

	NifFile clone(workNif);
	ChooseClothData(clone);

	for (auto &s : clone.GetShapes())
		if (find(exportShapes.begin(), exportShapes.end(), s->GetName()) == exportShapes.end())
			clone.DeleteShape(s);

	for (auto &s : clone.GetShapes())
		clone.UpdateSkinPartitions(s);

	clone.GetHeader().SetExportInfo("Exported using Outfit Studio.");

	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::out | std::ios::binary);

	return clone.Save(file);
}

int OutfitProject::ImportOBJ(const std::string& fileName, const std::string& shapeName, NiShape* mergeShape) {
	ObjFile obj;
	obj.SetScale(Vector3(10.0f, 10.0f, 10.0f));

	if (!shapeName.empty())
		outfitName = shapeName;
	else if (outfitName.empty())
		outfitName = "New Outfit";

	if (obj.LoadForNif(fileName)) {
		wxLogError("Could not load OBJ file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Could not load OBJ file '%s'!"), fileName), _("OBJ Error"), wxICON_ERROR, owner);
		return 1;
	}

	std::vector<std::string> groupNames = obj.GetGroupList();
	for (const auto& group : groupNames) {
		std::vector<Vector3> v;
		std::vector<Triangle> t;
		std::vector<Vector2> uv;
		std::vector<Vector3> n;
		if (!obj.CopyDataForGroup(group, &v, &t, &uv, &n)) {
			wxLogError("Could not copy data from OBJ file '%s'!", fileName);
			wxMessageBox(wxString::Format(_("Could not copy data from OBJ file '%s'!"), fileName), _("OBJ Error"), wxICON_ERROR, owner);
			return 3;
		}

		// Skip zero size groups.  
		if (v.size() == 0)
			continue;

		std::string useShapeName = group;

		if (mergeShape) {
			std::vector<Vector3> shapeVerts;
			workNif.GetVertsForShape(mergeShape, shapeVerts);
			if (shapeVerts.size() == v.size()) {
				int ret = wxMessageBox(_("The vertex count of the selected .obj file matches the currently selected outfit shape.  Do you wish to update the current shape?  (click No to create a new shape)"), _("Merge or New"), wxYES_NO | wxICON_QUESTION, owner);
				if (ret == wxYES) {
					ret = wxMessageBox(_("Update Vertex Positions?"), _("Vertex Position Update"), wxYES_NO | wxICON_QUESTION, owner);
					if (ret == wxYES)
						workNif.SetVertsForShape(mergeShape, v);

					ret = wxMessageBox(_("Update Texture Coordinates?"), _("UV Update"), wxYES_NO | wxICON_QUESTION, owner);
					if (ret == wxYES)
						workNif.SetUvsForShape(mergeShape, uv);

					return 101;
				}
			}
			useShapeName = wxGetTextFromUser(_("Please specify a name for the new shape"), _("New Shape Name"), useShapeName, owner).ToUTF8();
			if (useShapeName.empty())
				return 100;
		}

		CreateNifShapeFromData(useShapeName, v, t, uv, &n);
	}

	return 0;
}

int OutfitProject::ExportOBJ(const std::string& fileName, const std::vector<NiShape*>& shapes, const Vector3& scale, const Vector3& offset) {
	ObjFile obj;
	obj.SetScale(scale);
	obj.SetOffset(offset);

	for (auto &shape : shapes) {
		if (!shape)
			return 1;

		std::vector<Triangle> tris;
		if (!shape->GetTriangles(tris))
			return 2;

		const std::vector<Vector3>* verts = workNif.GetRawVertsForShape(shape);
		if (!verts)
			return 3;

		const std::vector<Vector2>* uvs = workNif.GetUvsForShape(shape);
		const std::vector<Vector3>* norms = workNif.GetNormalsForShape(shape, false);

		obj.AddGroup(shape->GetName(), *verts, tris, uvs ? *uvs : std::vector<Vector2>(), norms ? *norms : std::vector<Vector3>());
	}

	if (obj.Save(fileName))
		return 4;

	return 0;
}

int OutfitProject::ImportFBX(const std::string& fileName, const std::string& shapeName, NiShape* mergeShape) {
	FBXWrangler fbxw;
	std::string nonRefBones;

	FBXImportDialog import(owner);
	if (import.ShowModal() != wxID_OK)
		return 1;

	bool result = fbxw.ImportScene(fileName, import.GetOptions());
	if (!result)
		return 2;

	if (!shapeName.empty())
		outfitName = shapeName;
	else if (outfitName.empty())
		outfitName = "New Outfit";

	std::vector<std::string>shapes;
	fbxw.GetShapeNames(shapes);
	for (auto &s : shapes) {
		FBXShape* fbxShape = fbxw.GetShape(s);
		std::string useShapeName = s;

		if (mergeShape) {
			if (mergeShape->GetNumVertices() == fbxShape->verts.size()) {
				int ret = wxMessageBox(_("The vertex count of the selected .fbx file matches the currently selected outfit shape.  Do you wish to update the current shape?  (click No to create a new shape)"), _("Merge or New"), wxYES_NO | wxICON_QUESTION, owner);
				if (ret == wxYES) {
					ret = wxMessageBox(_("Update Vertex Positions?"), _("Vertex Position Update"), wxYES_NO | wxICON_QUESTION, owner);
					if (ret == wxYES)
						workNif.SetVertsForShape(mergeShape, fbxShape->verts);

					ret = wxMessageBox(_("Update Texture Coordinates?"), _("UV Update"), wxYES_NO | wxICON_QUESTION, owner);
					if (ret == wxYES)
						workNif.SetUvsForShape(mergeShape, fbxShape->uvs);

					ret = wxMessageBox(_("Update Animation Weighting?"), _("Animation Weight Update"), wxYES_NO | wxICON_QUESTION, owner);
					if (ret == wxYES)
						for (auto &bn : fbxShape->boneNames)
							workAnim.SetWeights(mergeShape->GetName(), bn, fbxShape->boneSkin[bn].GetWeights());

					return 101;
				}
			}

			useShapeName = wxGetTextFromUser(_("Please specify a name for the new shape"), _("New Shape Name"), useShapeName, owner).ToUTF8();
			if (useShapeName.empty())
				return 100;
		}

		CreateNifShapeFromData(s, fbxShape->verts, fbxShape->tris, fbxShape->uvs, &fbxShape->normals);

		int slot = 0;
		std::vector<int> boneIndices;
		for (auto &bn : fbxShape->boneNames) {
			if (!AnimSkeleton::getInstance().RefBone(bn)) {
				// Not found in reference skeleton, use default values
				AnimBone& cstm = AnimSkeleton::getInstance().AddBone(bn, true);
				if (!cstm.isValidBone)
					nonRefBones += bn + "\n";

				AnimSkeleton::getInstance().RefBone(bn);
			}

			workAnim.shapeBones[useShapeName].push_back(bn);
			workAnim.shapeSkinning[useShapeName].boneNames[bn] = slot;
			workAnim.SetWeights(useShapeName, bn, fbxShape->boneSkin[bn].GetWeights());

			if (baseShape) {
				MatTransform xform;
				if (workAnim.GetShapeBoneXForm(baseShape->GetName(), bn, xform))
					workAnim.SetShapeBoneXForm(useShapeName, bn, xform);
			}

			boneIndices.push_back(slot++);
		}

		auto shape = workNif.FindBlockByName<NiShape>(useShapeName);
		workNif.SetShapeBoneIDList(shape, boneIndices);

		if (!nonRefBones.empty())
			wxLogMessage("Bones in shape '%s' not found in reference skeleton:\n%s", useShapeName, nonRefBones);
	}

	return 0;
}

int OutfitProject::ExportFBX(const std::string& fileName, const std::vector<NiShape*>& shapes) {
	FBXWrangler fbxw;
	fbxw.AddSkeleton(&AnimSkeleton::getInstance().refSkeletonNif);

	for (auto &s : shapes) {
		fbxw.AddNif(&workNif, s);
		fbxw.AddSkinning(&workAnim, s);
	}

	return fbxw.ExportScene(fileName);
}


void OutfitProject::ValidateNIF(NifFile& nif) {
	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");
	bool match = false;

	switch (targetGame) {
	case FO3:
	case FONV:
		match = nif.GetHeader().GetVersion().IsFO3();
		break;
	case SKYRIM:
		match = nif.GetHeader().GetVersion().IsSK();
		break;
	case FO4:
	case FO4VR:
		match = nif.GetHeader().GetVersion().IsFO4();
		break;
	case SKYRIMSE:
	case SKYRIMVR:
		match = nif.GetHeader().GetVersion().IsSSE();
		break;
	}

	if (!match) {
		if ((targetGame == SKYRIMSE || targetGame == SKYRIMVR) && nif.GetHeader().GetVersion().IsSK()) {
			if (!Config.Exists("OptimizeForSSE")) {
				int res = wxMessageBox(_("Would you like Skyrim NIFs to be optimized for SSE during this session?"), _("Target Game"), wxYES_NO | wxICON_INFORMATION, owner);
				if (res == wxYES)
					Config.SetDefaultBoolValue("OptimizeForSSE", true);
				else
					Config.SetDefaultBoolValue("OptimizeForSSE", false);
			}

			if (Config["OptimizeForSSE"] == "true") {
				OptOptions options;
				options.targetVersion.SetFile(V20_2_0_7);
				options.targetVersion.SetUser(12);
				options.targetVersion.SetStream(100);
				nif.OptimizeFor(options);
			}
		}
		else {
			wxLogWarning("Version of NIF file doesn't match current target game. To use the meshes for the target game, export to OBJ/FBX and reload them again.");
			wxMessageBox(wxString::Format(_("Version of NIF file doesn't match current target game. To use the meshes for the target game, export to OBJ/FBX and reload them again.")), _("Version"), wxICON_WARNING, owner);
		}
	}

	for (auto &s : nif.GetShapes())
		nif.TriangulateShape(s);
}

void OutfitProject::ResetTransforms() {
	bool clearRoot = false;
	bool unskinnedFound = false;

	for (auto &s : workNif.GetShapes()) {
		if (s->IsSkinned()) {
			/*
			 * Root node, shape and overall skin transform aren't rendered for skinned meshes.
			 * They only affect different things, e.g. bounds.
			 *
			 * By clearing these and recalculating bounds on export we make sure that
			 * nothing but the individual bone transforms affect visuals.
			 */

			if (!unskinnedFound)
				clearRoot = true;

			 // Clear shape transform
			s->transform.Clear();

			// Clear overall skin transform
			MatTransform xForm;
			workNif.SetShapeBoneTransform(s, 0xFFFFFFFF, xForm);

			//workAnim.GetBoneXForm("", );
		}
		else {
			clearRoot = false;
			unskinnedFound = true;
		}
	}

	if (clearRoot) {
		// Clear root node transform
		auto rootNode = workNif.GetRootNode();
		if (rootNode)
			rootNode->transform.Clear();
	}
}
