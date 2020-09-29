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
#include "../NIF/NifUtil.h"

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

void OutfitProject::SetBaseShape(NiShape* shape, const bool moveData) {
	if (moveData) {
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

	if (bPose) {
		int nv = outVerts.size();
		std::vector<Vector3> pv(nv);
		std::vector<float> wv(nv, 0.0f);
		AnimSkin &animSkin = workAnim.shapeSkinning[shape->GetName()];

		for (auto &boneNamesIt : animSkin.boneNames) {
			AnimBone *animB = AnimSkeleton::getInstance().GetBonePtr(boneNamesIt.first);
			if (animB) {
				AnimWeight &animW = animSkin.boneWeights[boneNamesIt.second];
				// Compose transform: skin -> (posed) bone -> global -> skin
				MatTransform t = animSkin.xformGlobalToSkin.ComposeTransforms(animB->xformPoseToGlobal.ComposeTransforms(animW.xformSkinToBone));
				// Add weighted contributions to vertex for this bone
				for (auto &wIt : animW.weights) {
					int ind = wIt.first;
					float w = wIt.second;
					pv[ind] += w * t.ApplyTransform(outVerts[ind]);
					wv[ind] += w;
				}
			}
		}

		// Check if total weight for each vertex was 1
		for (int ind = 0; ind < nv; ++ind) {
			if (wv[ind] < EPSILON) // If weights are missing for this vertex
				pv[ind] = outVerts[ind];
			else if (std::fabs(wv[ind] - 1.0f) >= EPSILON) // If weights are bad for this vertex
				pv[ind] /= wv[ind];
			// else do nothing because weights totaled 1.
		}

		outVerts.swap(pv);
	}

	InvalidateBoneScaleCache();
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

void OutfitProject::ApplyTransformToShapeGeometry(NiShape* shape, const MatTransform &t) {
	if (!shape)
		return;

	// Vertices
	const std::vector<Vector3>* oldVerts = workNif.GetRawVertsForShape(shape);
	if (!oldVerts || oldVerts->empty())
		return;

	int nVerts = oldVerts->size();
	std::vector<Vector3> verts(nVerts);
	for (int i = 0; i < nVerts; ++i)
		verts[i] = t.ApplyTransform((*oldVerts)[i]);

	workNif.SetVertsForShape(shape, verts);

	// Normals
	if (t.rotation.IsNearlyEqualTo(Matrix3()))
		return;

	const std::vector<Vector3>* oldNorms = workNif.GetNormalsForShape(shape, false);
	if (!oldNorms || oldNorms->size() != nVerts)
		return;

	std::vector<Vector3> norms(nVerts);
	for (int i = 0; i < nVerts; ++i)
		norms[i] = t.rotation * (*oldNorms)[i];

	workNif.SetNormalsForShape(shape, norms);
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

		workAnim.AddShapeBone(shapeName, boneName);
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

void OutfitProject::InvalidateBoneScaleCache() {
	boneScaleVerts.clear();
	boneScaleOffsets.clear();
}

void OutfitProject::ApplyBoneScale(const std::string& bone, int sliderPos, bool clear) {
	ClearBoneScale(false);

	AnimBone *bptr = AnimSkeleton::getInstance().GetBonePtr(bone);
	if (!bptr) return;
	MatTransform xform = bPose ? bptr->xformPoseToGlobal : bptr->xformToGlobal;

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
	for (auto &s : workNif.GetShapeNames())
		workAnim.AddShapeBone(s, boneName);
}

void OutfitProject::AddCustomBoneRef(const std::string& boneName, const std::string& parentBone, const MatTransform &xformToParent) {
	AnimBone& customBone = AnimSkeleton::getInstance().AddCustomBone(boneName);
	customBone.SetTransformBoneToParent(xformToParent);
	customBone.SetParentBone(AnimSkeleton::getInstance().GetBonePtr(parentBone));

	for (auto &s : workNif.GetShapeNames())
		workAnim.AddShapeBone(s, boneName);
}

void OutfitProject::ModifyCustomBone(AnimBone *bPtr, const std::string& parentBone, const MatTransform &xformToParent) {
	bPtr->SetTransformBoneToParent(xformToParent);
	bPtr->SetParentBone(AnimSkeleton::getInstance().GetBonePtr(parentBone));

	for (auto &s : workNif.GetShapeNames())
		workAnim.RecursiveRecalcXFormSkinToBone(s, bPtr);
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
		return AddFromSliderSet(sourceFile, set, false);
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

int OutfitProject::AddFromSliderSet(const std::string& fileName, const std::string& sliderSetName, const bool newDataLocal) {
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
	morpher.MergeResultDiffs(activeSet, addSet, baseDiffData, baseShape ? baseShape->GetName() : "", newDataLocal);

	owner->UpdateProgress(100, _("Finished"));
	owner->EndProgress();
	return 0;
}

void OutfitProject::InitConform() {
	if (baseShape) {
		morpher.SetRef(workNif, baseShape, &workAnim);
		morpher.LinkRefDiffData(&baseDiffData);
		morpher.SourceShapesFromNif(workNif, &workAnim);
	}
}

void OutfitProject::ConformShape(NiShape* shape, const ConformOptions& options) {
	if (!workNif.IsValid() || !baseShape)
		return;

	morpher.BuildProximityCache(shape->GetName(), options.proximityRadius);

	std::string refTarget = ShapeToTarget(baseShape->GetName());
	for (int i = 0; i < activeSet.size(); i++)
		if (SliderShow(i) && !SliderZap(i) && !SliderUV(i))
			morpher.GenerateResultDiff(shape->GetName(), activeSet[i].name, activeSet[i].TargetDataName(refTarget), options.maxResults, options.axisX, options.axisY, options.axisZ);
}

void OutfitProject::CollectVertexData(NiShape* shape, UndoStateShape &uss, const std::vector<int> &indices) {
	uss.delVerts.resize(indices.size());

	const std::vector<Vector3> *verts = workNif.GetRawVertsForShape(shape);
	const std::vector<Vector2> *uvs = workNif.GetUvsForShape(shape);
	const std::vector<Color4> *colors = workNif.GetColorsForShape(shape->GetName());
	const std::vector<Vector3> *normals = workNif.GetNormalsForShape(shape, false);
	const std::vector<Vector3> *tangents = workNif.GetTangentsForShape(shape, false);
	const std::vector<Vector3> *bitangents = workNif.GetBitangentsForShape(shape, false);
	std::vector<float> *eyeData = workNif.GetEyeDataForShape(shape);
	AnimSkin &skin = workAnim.shapeSkinning[shape->GetName()];
	std::string target = ShapeToTarget(shape->GetName());

	for (int di = 0; di < indices.size(); ++di) {
		UndoStateVertex &usv = uss.delVerts[di];
		int vi = indices[di];
		usv.index = vi;

		if (verts && verts->size() > vi)
			usv.pos = (*verts)[vi];
		if (uvs && uvs->size() > vi)
			usv.uv = (*uvs)[vi];
		if (colors && colors->size() > vi)
			usv.color = (*colors)[vi];
		if (normals && normals->size() > vi)
			usv.normal = (*normals)[vi];
		if (tangents && tangents->size() > vi)
			usv.tangent = (*tangents)[vi];
		if (bitangents && bitangents->size() > vi)
			usv.bitangent = (*bitangents)[vi];
		if (eyeData && eyeData->size() > vi)
			usv.eyeData = (*eyeData)[vi];

		for (auto bnp : skin.boneNames) {
			AnimWeight &aw = skin.boneWeights[bnp.second];
			auto wit = aw.weights.find(vi);
			if (wit != aw.weights.end())
				usv.weights.emplace_back(UndoStateVertexBoneWeight{ bnp.first, wit->second });
		}
	}

	// For diffs, it's more efficient to reverse the loop nesting.
	for (int si = 0; si < activeSet.size(); ++si) {
		std::string targetDataName = activeSet[si].TargetDataName(target);
		if (targetDataName.empty())
			targetDataName = target + activeSet[si].name;
		std::unordered_map<ushort, Vector3>* diffSet;
		if (IsBaseShape(shape))
			diffSet = baseDiffData.GetDiffSet(targetDataName);
		else
			diffSet = morpher.GetDiffSet(targetDataName);

		if (!diffSet)
			continue;

		for (UndoStateVertex &usv : uss.delVerts) {
			auto dit = diffSet->find(usv.index);
			if (dit == diffSet->end())
				continue;

			usv.diffs.push_back(UndoStateVertexSliderDiff{ activeSet[si].name, dit->second });
		}
	}
}

void OutfitProject::CollectTriangleData(NiShape* shape, UndoStateShape &uss, const std::vector<int> &indices) {
	uss.delTris.resize(indices.size());
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);
	std::vector<int> triParts;
	NifSegmentationInfo inf;
	if (!workNif.GetShapeSegments(shape, inf, triParts)) {
		std::vector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
		workNif.GetShapePartitions(shape, partitionInfo, triParts);
	}

	for (int di = 0; di < indices.size(); ++di) {
		UndoStateTriangle &ust = uss.delTris[di];
		int ti = indices[di];
		ust.index = ti;
		ust.t = tris[ti];
		if (ti < triParts.size())
			ust.partID = triParts[ti];
		else
			ust.partID = -1;
	}
}

bool OutfitProject::PrepareDeleteVerts(NiShape* shape, const std::unordered_map<ushort, float>& mask, UndoStateShape &uss) {
	int numVerts = shape->GetNumVertices();

	// Set flag for every vertex index in mask
	std::vector<bool> delVertFlags(numVerts, false);
	for (auto &m : mask)
		delVertFlags[m.first] = true;

	// Generate list of triangles to delete.  Also count how many
	// non-deleted triangles each vertex belongs to.
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<int> delTriInds;
	std::vector<int> vertTriCounts(numVerts, 0);

	for (int ti = 0; ti < tris.size(); ++ti) {
		if (delVertFlags[tris[ti].p1] ||
			delVertFlags[tris[ti].p2] ||
			delVertFlags[tris[ti].p3])
			delTriInds.push_back(ti);
		else {
			++vertTriCounts[tris[ti].p1];
			++vertTriCounts[tris[ti].p2];
			++vertTriCounts[tris[ti].p3];
		}
	}

	// If all triangles are deleted, then delete the whole shape.
	if (delTriInds.size() >= tris.size())
		return true;

	// Generate new list of vertices to delete: vertices that are not
	// used by any non-deleted triangle.
	std::vector<int> delVertInds;
	for (int vi = 0; vi < numVerts; ++vi)
		if (vertTriCounts[vi] <= 0)
			delVertInds.push_back(vi);

	// If all vertices are deleted, then delete the whole shape.
	if (delVertInds.size() >= numVerts)
		return true;

	// Now collect the vertex and triangle data.
	CollectVertexData(shape, uss, delVertInds);
	CollectTriangleData(shape, uss, delTriInds);

	return false;
}

void OutfitProject::ApplyShapeMeshUndo(NiShape* shape, const UndoStateShape &uss, bool bUndo) {
	const std::vector<UndoStateVertex> &delVerts = bUndo ? uss.addVerts : uss.delVerts;
	const std::vector<UndoStateVertex> &addVerts = bUndo ? uss.delVerts : uss.addVerts;
	const std::vector<UndoStateTriangle> &delTris = bUndo ? uss.addTris : uss.delTris;
	const std::vector<UndoStateTriangle> &addTris = bUndo ? uss.delTris : uss.addTris;

	// Gather data
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<int> triParts;
	NifSegmentationInfo inf;
	bool gotsegs = workNif.GetShapeSegments(shape, inf, triParts);
	std::vector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
	bool gotparts = false;
	if (!gotsegs)
		gotparts = workNif.GetShapePartitions(shape, partitionInfo, triParts);

	std::vector<Vector3> verts;
	workNif.GetVertsForShape(shape, verts);

	const std::vector<Vector2> *uvsp = workNif.GetUvsForShape(shape);
	const std::vector<Color4> *colorsp = workNif.GetColorsForShape(shape->GetName());
	const std::vector<Vector3> *normalsp = workNif.GetNormalsForShape(shape, false);
	const std::vector<Vector3> *tangentsp = workNif.GetTangentsForShape(shape, false);
	const std::vector<Vector3> *bitangentsp = workNif.GetBitangentsForShape(shape, false);
	std::vector<float> *eyeDatap = workNif.GetEyeDataForShape(shape);

	std::vector<Vector2> uvs;
	std::vector<Color4> colors;
	std::vector<Vector3> normals;
	std::vector<Vector3> tangents;
	std::vector<Vector3> bitangents;
	std::vector<float> eyeData;
	if (uvsp)
		uvs = *uvsp;
	if (colorsp)
		colors = *colorsp;
	if (normalsp)
		normals = *normalsp;
	if (tangentsp)
		tangents = *tangentsp;
	if (bitangentsp)
		bitangents = *bitangentsp;
	if (eyeDatap)
		eyeData = *eyeDatap;

	AnimSkin &skin = workAnim.shapeSkinning[shape->GetName()];
	std::string target = ShapeToTarget(shape->GetName());

	if (!delTris.empty()) {
		// Delete triangles
		std::vector<int> delTriInds(delTris.size());
		for (int di = 0; di < delTris.size(); ++di)
			delTriInds[di] = delTris[di].index;

		EraseVectorIndices(tris, delTriInds);

		if (gotsegs || gotparts)
			EraseVectorIndices(triParts, delTriInds);
	}

	if (!delVerts.empty()) {
		// Delete vertices...
		std::vector<ushort> delVertInds(delVerts.size());
		for (int di = 0; di < delVerts.size(); ++di)
			delVertInds[di] = delVerts[di].index;
		std::vector<int> vertCollapseMap = GenerateIndexCollapseMap(delVertInds, verts.size());

		// ...from triangles
		ApplyMapToTriangles(tris, vertCollapseMap);

		// ...from workAnim
		workAnim.DeleteVertsForShape(shape->GetName(), delVertInds);

		// ...from diff data
		if (IsBaseShape(shape))
			baseDiffData.DeleteVerts(target, delVertInds);
		else
			morpher.DeleteVerts(target, delVertInds);

		// ...from nif arrays
		EraseVectorIndices(verts, delVertInds);
		if (uvsp)
			EraseVectorIndices(uvs, delVertInds);
		if (colorsp)
			EraseVectorIndices(colors, delVertInds);
		if (normalsp)
			EraseVectorIndices(normals, delVertInds);
		if (tangentsp)
			EraseVectorIndices(tangents, delVertInds);
		if (bitangentsp)
			EraseVectorIndices(bitangents, delVertInds);
		if (eyeDatap)
			EraseVectorIndices(eyeData, delVertInds);
	}

	if (!addVerts.empty()) {
		// Insert new vertex indices...
		std::vector<ushort> insVertInds(addVerts.size());
		for (int di = 0; di < addVerts.size(); ++di)
			insVertInds[di] = addVerts[di].index;
		std::vector<int> vertExpandMap = GenerateIndexExpandMap(insVertInds, verts.size());

		// ...into triangles
		ApplyMapToTriangles(tris, vertExpandMap);

		// ...into workAnim
		skin.InsertVertexIndices(insVertInds);

		// ...into diff data
		if (IsBaseShape(shape))
			baseDiffData.InsertVertexIndices(target, insVertInds);
		else
			morpher.InsertVertexIndices(target, insVertInds);

		// ...into nif arrays
		InsertVectorIndices(verts, insVertInds);
		if (uvsp)
			InsertVectorIndices(uvs, insVertInds);
		if (colorsp)
			InsertVectorIndices(colors, insVertInds);
		if (normalsp)
			InsertVectorIndices(normals, insVertInds);
		if (tangentsp)
			InsertVectorIndices(tangents, insVertInds);
		if (bitangentsp)
			InsertVectorIndices(bitangents, insVertInds);
		if (eyeDatap)
			InsertVectorIndices(eyeData, insVertInds);

		// Store vertex data...
		for (const UndoStateVertex &usv : addVerts) {
			// ...in nif arrays
			verts[usv.index] = usv.pos;
			if (uvsp && uvs.size() > usv.index)
				uvs[usv.index] = usv.uv;
			if (colorsp && colors.size() > usv.index)
				colors[usv.index] = usv.color;
			if (normalsp && normals.size() > usv.index)
				normals[usv.index] = usv.normal;
			if (tangentsp && tangents.size() > usv.index)
				tangents[usv.index] = usv.tangent;
			if (bitangentsp && bitangents.size() > usv.index)
				bitangents[usv.index] = usv.bitangent;
			if (eyeDatap && eyeData.size() > usv.index)
				eyeData[usv.index] = usv.eyeData;

			// ...in workAnim
			for (const auto &usvbw : usv.weights) {
				auto bnit = skin.boneNames.find(usvbw.boneName);
				if (bnit == skin.boneNames.end()) {
					workAnim.AddShapeBone(shape->GetName(), usvbw.boneName);
					bnit = skin.boneNames.find(usvbw.boneName);
				}
				skin.boneWeights[bnit->second].weights[usv.index] = usvbw.w;
			}

			// ...in diff data
			for (const UndoStateVertexSliderDiff &diff : usv.diffs) {
				std::string targetDataName = activeSet[diff.sliderName].TargetDataName(target);
				if (targetDataName.empty())
					targetDataName = target + diff.sliderName;
				std::unordered_map<ushort, Vector3>* diffSet;
				if (IsBaseShape(shape))
					diffSet = baseDiffData.GetDiffSet(targetDataName);
				else
					diffSet = morpher.GetDiffSet(targetDataName);
				if (!diffSet)	// should be impossible
					continue;
				(*diffSet)[usv.index] = diff.diff;
			}
		}
	}

	if (!addTris.empty()) {
		// Insert new triangle indices
		std::vector<int> insTriInds(addTris.size());
		for (int di = 0; di < addTris.size(); ++di)
			insTriInds[di] = addTris[di].index;

		InsertVectorIndices(tris, insTriInds);

		if (gotsegs || gotparts)
			InsertVectorIndices(triParts, insTriInds);

		// Store triangle data
		for (const UndoStateTriangle &ust : addTris) {
			tris[ust.index] = ust.t;
			if (gotsegs || gotparts)
				triParts[ust.index] = ust.partID;
		}
	}

	// Put data back in nif
	workNif.SetVertsForShape(shape, verts);
	if (uvsp)
		workNif.SetUvsForShape(shape, uvs);
	if (colorsp)
		workNif.SetColorsForShape(shape->GetName(), colors);
	if (normalsp)
		workNif.SetNormalsForShape(shape, normals);
	if (tangentsp)
		workNif.SetTangentsForShape(shape, tangents);
	if (bitangentsp)
		workNif.SetBitangentsForShape(shape, bitangents);
	if (eyeDatap)
		workNif.SetEyeDataForShape(shape, eyeData);

	shape->SetTriangles(tris);

	if (gotsegs)
		workNif.SetShapeSegments(shape, inf, triParts);
	if (gotparts)
		workNif.SetShapePartitions(shape, partitionInfo, triParts);

	// Note that we do not restore the nif's vertex bone weights.
	// That should happen when the file is saved.
}

bool OutfitProject::PrepareCollapseVertex(NiShape* shape, UndoStateShape &uss, const std::vector<int> &indices) {
	// Get triangle data
	int numVerts = shape->GetNumVertices();

	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<int> triParts;
	NifSegmentationInfo inf;

	if (!workNif.GetShapeSegments(shape, inf, triParts)) {
		std::vector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
		workNif.GetShapePartitions(shape, partitionInfo, triParts);
	}

	// Note that indices is already in sorted order.
	std::vector<int> ntris, nverts;
	auto addneighbor = [&nverts](int vi) {
		for (int evi : nverts)
			if (vi == evi)
				return;

		nverts.push_back(vi);
	};

	std::vector<int> nrtris; // non-replaced triangles

	for (int vi : indices) {
		ntris.clear();
		nverts.clear();

		// Collect lists of neighboring triangles and vertices.
		for (int ti = 0; ti < tris.size(); ++ti) {
			const Triangle &t = tris[ti];
			if (t.p1 != vi && t.p2 != vi && t.p3 != vi)
				continue;

			if (ntris.size() >= 3)
				return false;

			ntris.push_back(ti);

			// Order of vertex collection is important, or we'll end up
			// with a backwards triangle.
			if (t.p1 == vi) {
				addneighbor(t.p2);
				addneighbor(t.p3);
			}
			else if (t.p2 == vi) {
				addneighbor(t.p3);
				addneighbor(t.p1);
			}
			else {
				addneighbor(t.p1);
				addneighbor(t.p2);
			}

			if (nverts.size() > 3)
				return false;
		}

		// Make sure no neighboring vertices are welded to this vertex.
		for (int nvi = 0; nvi < nverts.size(); ++nvi)
			for (int ii = 0; ii < indices.size(); ++ii)
				if (indices[ii] == nverts[nvi])
					return false;

		// Put triangles to delete in uss.delTris.
		for (int vti = 0; vti < ntris.size(); ++vti) {
			int ti = ntris[vti];
			uss.delTris.push_back(UndoStateTriangle{ ti, tris[ti], ti < triParts.size() ? triParts[ti] : -1 });
		}

		int pti = -1;
		if (nverts.size() == 3) {
			// Determine preferred triangle to replace.
			pti = 0;
			if (!triParts.empty() && ntris.size() == 3 &&
				triParts[ntris[0]] != triParts[ntris[1]] &&
				triParts[ntris[1]] == triParts[ntris[2]])
				pti = 1;

			// Put new triangle in uss.
			int ti = ntris[pti];
			uss.addTris.push_back(UndoStateTriangle{ ti, Triangle(nverts[0], nverts[1], nverts[2]), ti < triParts.size() ? triParts[ti] : -1 });
		}

		// Collect list of non-replaced triangles for triangle renumbering.
		for (int vti = 0; vti < ntris.size(); ++vti)
			if (vti != pti)
				nrtris.push_back(ntris[vti]);
	}

	// We've loaded all the triangle data; now load the vertex data.
	CollectVertexData(shape, uss, indices);

	// Renumber vertex and triangle indices in uss.addTris.
	std::vector<int> vCollapse = GenerateIndexCollapseMap(indices, numVerts);

	std::sort(nrtris.begin(), nrtris.end());
	std::vector<int> tCollapse = GenerateIndexCollapseMap(nrtris, tris.size());

	for (UndoStateTriangle &ust : uss.addTris) {
		ust.t.p1 = vCollapse[ust.t.p1];
		ust.t.p2 = vCollapse[ust.t.p2];
		ust.t.p3 = vCollapse[ust.t.p3];
		ust.index = tCollapse[ust.index];
	}

	// Sort uss.addTris and uss.delTris, since they weren't added in any
	// particular order.
	std::sort(uss.addTris.begin(), uss.addTris.end());
	std::sort(uss.delTris.begin(), uss.delTris.end());

	return true;
}

static int TriangleOppositeVertex(const Triangle &t, int p1) {
	if (t.p1 == p1)
		return t.p3;
	else if (t.p2 == p1)
		return t.p1;
	else
		return t.p2;
}

bool OutfitProject::PrepareFlipEdge(NiShape* shape, UndoStateShape &uss, const Edge &edge) {
	// Get triangle data
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<int> triParts;
	NifSegmentationInfo inf;
	if (!workNif.GetShapeSegments(shape, inf, triParts)) {
		std::vector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
		workNif.GetShapePartitions(shape, partitionInfo, triParts);
	}

	// Find the two neighboring triangles
	int t1 = -1, t2 = -1;
	const Edge redge(edge.p2, edge.p1);
	for (int ti = 0; ti < tris.size(); ++ti) {
		if (tris[ti].HasOrientedEdge(edge)) {
			if (t1 != -1)
				return false;
			t1 = ti;
		}

		if (tris[ti].HasOrientedEdge(redge)) {
			if (t2 != -1)
				return false;
			t2 = ti;
		}
	}
	if (t1 == -1 || t2 == -1)
		return false;

	// Find the non-edge vertex for each neighboring triangle.
	int nev1 = TriangleOppositeVertex(tris[t1], edge.p1);
	int nev2 = TriangleOppositeVertex(tris[t2], edge.p2);

	// Put data into uss.
	int tp1 = t1 < triParts.size() ? triParts[t1] : -1;
	int tp2 = t2 < triParts.size() ? triParts[t2] : -1;
	uss.delTris.push_back(UndoStateTriangle{ t1, tris[t1], tp1 });
	uss.delTris.push_back(UndoStateTriangle{ t2, tris[t2], tp2 });
	uss.addTris.push_back(UndoStateTriangle{ t1, Triangle(edge.p1, nev2, nev1), tp1 });
	uss.addTris.push_back(UndoStateTriangle{ t2, Triangle(edge.p2, nev1, nev2), tp2 });

	// Sort delTris and addTris by index.
	if (t2 < t1) {
		std::swap(uss.delTris[0], uss.delTris[1]);
		std::swap(uss.addTris[0], uss.addTris[1]);
	}

	return true;
}

bool OutfitProject::PrepareSplitEdge(NiShape* shape, UndoStateShape &uss, const std::vector<int> &p1s, const std::vector<int> &p2s) {
	// Get vertex and triangle data
	const std::vector<Vector3>* verts = workNif.GetRawVertsForShape(shape);
	if (!verts)
		return false;

	int newvi = verts->size();

	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	std::vector<int> triParts;
	NifSegmentationInfo inf;
	if (!workNif.GetShapeSegments(shape, inf, triParts)) {
		std::vector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
		workNif.GetShapePartitions(shape, partitionInfo, triParts);
	}

	// Find the two neighboring triangles.
	int t1 = -1, t2 = -1;
	Edge edge, redge;
	for (int ti = 0; ti < tris.size(); ++ti) {
		for (int p1 : p1s) {
			for (int p2 : p2s) {
				if (tris[ti].HasOrientedEdge(Edge(p1, p2))) {
					if (t1 != -1)
						return false;
					t1 = ti;
					edge.p1 = p1;
					edge.p2 = p2;
				}
				if (tris[ti].HasOrientedEdge(Edge(p2, p1))) {
					if (t2 != -1)
						return false;
					t2 = ti;
					redge.p1 = p2;
					redge.p2 = p1;
				}
			}
		}
	}
	if (t1 == -1)
		return false;

	// Find non-edge vertex for each neighboring triangle.
	int nev1 = TriangleOppositeVertex(tris[t1], edge.p1);
	int nev2 = -1;
	if (t2 != -1)
		nev2 = TriangleOppositeVertex(tris[t2], redge.p1);

	// Determine whether we have a welded edge.
	bool welded = t2 != -1 && (redge.p1 != edge.p2 || redge.p2 != edge.p1);
	int newrvi = newvi;
	if (welded)
		++newrvi;

	// Put triangle data into uss.
	if (t1 != -1) {
		int newt1 = t1;
		if (t2 != -1 && t2 < t1)
			++newt1;

		int tp = t1 < triParts.size() ? triParts[t1] : -1;
		uss.delTris.push_back(UndoStateTriangle{ t1, tris[t1], tp });
		uss.addTris.push_back(UndoStateTriangle{ newt1, Triangle(edge.p1, newvi, nev1), tp });
		uss.addTris.push_back(UndoStateTriangle{ newt1 + 1, Triangle(nev1, newvi, edge.p2), tp });
	}
	if (t2 != -1) {
		int newt2 = t2;
		if (t1 != -1 && t1 < t2)
			++newt2;

		int tp = t2 < triParts.size() ? triParts[t2] : -1;
		uss.delTris.push_back(UndoStateTriangle{ t2, tris[t2], tp });
		uss.addTris.push_back(UndoStateTriangle{ newt2, Triangle(redge.p1, newrvi, nev2), tp });
		uss.addTris.push_back(UndoStateTriangle{ newt2 + 1, Triangle(nev2, newrvi, redge.p2), tp });
	}

	// Sort delTris and addTris by index.
	std::sort(uss.delTris.begin(), uss.delTris.end());
	std::sort(uss.addTris.begin(), uss.addTris.end());

	// Get data for the two edge vertices (in a secondary data-collection
	// UndoStateShape).
	UndoStateShape duss;
	std::vector<int> edgeInds{ edge.p1, edge.p2 };
	if (welded) {
		edgeInds.push_back(redge.p1);
		edgeInds.push_back(redge.p2);
	}
	CollectVertexData(shape, duss, edgeInds);
	const UndoStateVertex &p1d = duss.delVerts[0];
	const UndoStateVertex &p2d = duss.delVerts[1];
	const UndoStateVertex &rp1d = duss.delVerts[welded ? 2 : 1];
	const UndoStateVertex &rp2d = duss.delVerts[welded ? 3 : 0];
	const Vector3 &p1 = p1d.pos;
	const Vector3 &p2 = p2d.pos;

	// Create entry for new vertex
	uss.addVerts.emplace_back();
	UndoStateVertex &usv = uss.addVerts.back();
	usv.index = newvi;

	/* Now comes the hard part: determining a good location for the
	new vertex.  Clearly it should lie somewhere on the plane bisecting
	the segment between p1 and p2.  If we wanted to be lazy, we could
	just pick the midpoint.  But that would require the user to adjust
	the position every time.  Better would be if we could fit a circle
	through p1, p2, and the new point so that all three have normals
	perpendicular to the circle.  But there's no guarantee that that's
	possible: the old normals could have different angles to the edge;
	and they could even be non-coplanar.  So we need to average the
	angles for the two normals somehow.  There are several stages
	where this could be done; I choose to do it early. */

	// Calculate normals at p1 and p2 by averaging their triangle normals.
	Vector3 np1, np2;
	for (const Triangle &t : tris) {
		if (t.p1 == edge.p1 || t.p2 == edge.p1 || t.p3 == edge.p1 ||
			t.p1 == redge.p2 || t.p2 == redge.p2 || t.p3 == redge.p2) {
			Vector3 tn;
			t.trinormal(*verts, &tn);
			tn.Normalize();
			np1 += tn;
		}
		if (t.p1 == edge.p2 || t.p2 == edge.p2 || t.p3 == edge.p2 ||
			t.p1 == redge.p1 || t.p2 == redge.p1 || t.p3 == redge.p1) {
			Vector3 tn;
			t.trinormal(*verts, &tn);
			tn.Normalize();
			np2 += tn;
		}
	}

	np1.Normalize();
	np2.Normalize();

	// Starting position of new point: midpoint between p1 and p2
	usv.pos = (p1 + p2) * 0.5;

	Vector3 u12 = p2 - p1;	// unit vector from p1 to p2 (along the edge)
	float elen = u12.length();	// edge length
	u12.Normalize();

	// Working normal for new point: average of np1 and np2, made
	// perpendicular to u12.  (If you want something fancy for usv.normal,
	// calculate it later.  This calculation needs to be done this
	// way for the circle fitter.)
	usv.normal = np1 + np2;
	usv.normal -= u12 * u12.dot(usv.normal);
	usv.normal.Normalize();

	// Now, the angle between npi and usv.normal, in the plane of
	// npi and u12 (since usv.normal isn't necessarily in that plane)
	// would be asin(u12.dot(npi)).  We want to average this for np1
	// and np2 and carefully preserve the sign.
	float angle = asin(u12.dot(np2 - np1) * 0.5);
	// Now, "angle" is the desired circle angle between the new point and
	// either p1 or p2.  It's positive for convex, negative for concave.
	// To figure out how far off of the edge we need to go, we need to
	// take the trigonometric tangent of the correct angle.  It turns out
	// the correct angle is the angle we just calculated divided by 2.
	float curveOffsetFactor = tan(angle * 0.5);
	// Now apply the offset to the new point.  (curveOffsetFactor is positive
	// for convex, negative for concave, and will be no larger than 1.)
	usv.pos += usv.normal * (curveOffsetFactor * elen * 0.5);

	// Calculate uv, color, tangent, bitangent, and eyeData by averaging.
	// We need to make sure normal, tangent, and bitangent are perpendicular.
	usv.uv = (p1d.uv + p2d.uv) * 0.5;
	usv.color.r = (p1d.color.r + p2d.color.r) * 0.5;
	usv.color.g = (p1d.color.g + p2d.color.g) * 0.5;
	usv.color.b = (p1d.color.b + p2d.color.b) * 0.5;
	usv.color.a = (p1d.color.a + p2d.color.a) * 0.5;
	usv.eyeData = (p1d.eyeData + p2d.eyeData) * 0.5;
	usv.tangent = (p1d.tangent + p2d.tangent) * 0.5;
	usv.bitangent = (p1d.bitangent + p2d.bitangent) * 0.5;
	usv.tangent -= usv.normal * usv.normal.dot(usv.tangent);
	usv.tangent.Normalize();
	usv.bitangent -= usv.normal * usv.normal.dot(usv.bitangent);
	usv.bitangent -= usv.tangent * usv.tangent.dot(usv.bitangent);
	usv.bitangent.Normalize();

	// Calculate weights by averaging.  Note that the resulting weights
	// will add up to 1 if each source's weights do.
	for (int si = 0; si < 2; ++si) {
		const std::vector<UndoStateVertexBoneWeight> &swts = si ? p2d.weights : p1d.weights;
		for (const UndoStateVertexBoneWeight &sw : swts) {
			bool found = false;
			for (int dwi = 0; dwi < usv.weights.size() && !found; ++dwi) {
				if (usv.weights[dwi].boneName == sw.boneName) {
					found = true;
					usv.weights[dwi].w += sw.w;
				}
			}

			if (!found)
				usv.weights.push_back(sw);
		}
	}

	for (auto &dw : usv.weights)
		dw.w *= 0.5;

	UndoStateVertex rusv;
	if (welded) {
		// New welded vertex gets exactly the same data except uv and uv-diffs.
		rusv = usv;
		rusv.index = newrvi;
		rusv.uv = (rp1d.uv + rp2d.uv) * 0.5;
	}

	// Unfortunately, we can't just calculate diffs by averaging.  p1
	// and p2 may have moved farther apart, which would require greater
	// curve offset.

	// diffpairs: key is sliderName.
	std::unordered_map<std::string, std::pair<Vector3, Vector3>> diffpairs;
	for (auto &sd : p1d.diffs)
		diffpairs[sd.sliderName].first = sd.diff;
	for (auto &sd : p2d.diffs)
		diffpairs[sd.sliderName].second = sd.diff;

	for (auto &dp : diffpairs) {
		// First, just average the diffs.
		Vector3 diff = (dp.second.first + dp.second.second) * 0.5;
		const SliderData &sd = activeSet[dp.first];
		if (!sd.bUV && !sd.bClamp && !sd.bZap) {
			// Calculate the distance between the moved p1 and p2.
			float delen = (dp.second.second + p2 - dp.second.first - p1).length();
			// Apply more curve offset (for delen > elen)
			// or less (for delen < elen).
			diff += usv.normal * (curveOffsetFactor * (delen - elen) * 0.5);
		}
		usv.diffs.push_back(UndoStateVertexSliderDiff{ dp.first, diff });
		if (welded && !sd.bUV)
			rusv.diffs.push_back(UndoStateVertexSliderDiff{ dp.first, diff });
	}

	if (welded) {
		// Repeat the diff calculation, but only for UV diffs.
		std::unordered_map<std::string, std::pair<Vector3, Vector3>> rdiffpairs;
		for (auto &sd : rp1d.diffs)
			rdiffpairs[sd.sliderName].first = sd.diff;
		for (auto &sd : rp2d.diffs)
			rdiffpairs[sd.sliderName].second = sd.diff;

		for (auto &dp : rdiffpairs) {
			const SliderData &sd = activeSet[dp.first];
			if (!sd.bUV)
				continue;
			Vector3 diff = (dp.second.first + dp.second.second) * 0.5;
			rusv.diffs.push_back(UndoStateVertexSliderDiff{ dp.first, diff });
		}

		uss.addVerts.push_back(std::move(rusv));
	}

	return true;
}

void OutfitProject::CheckMerge(const std::string &sourceName, const std::string &targetName, MergeCheckErrors &e) {
	if (sourceName == targetName) {
		e.shapesSame = true;
		return;
	}

	NiShape *source = workNif.FindBlockByName<NiShape>(sourceName);
	if (!source)
		return;
	NiShape *target = workNif.FindBlockByName<NiShape>(targetName);
	if (!target)
		return;

	size_t maxVertIndex = std::numeric_limits<ushort>().max();
	size_t maxTriIndex = std::numeric_limits<ushort>().max();
	if (workNif.GetHeader().GetVersion().IsFO4())
		maxTriIndex = std::numeric_limits<uint>().max();

	size_t snVerts = source->GetNumVertices();
	size_t tnVerts = target->GetNumVertices();
	if (snVerts + tnVerts > maxVertIndex)
		e.tooManyVertices = true;

	size_t snTris = source->GetNumTriangles();
	size_t tnTris = target->GetNumTriangles();
	if (snTris + tnTris > maxTriIndex)
		e.tooManyTriangles = true;

	std::vector<int> triParts;
	NifSegmentationInfo sinf, tinf;
	bool gotssegs = workNif.GetShapeSegments(source, sinf, triParts);
	bool gottsegs = workNif.GetShapeSegments(target, tinf, triParts);
	if (gotssegs != gottsegs) {
		// Shape with segments and the other without
		e.segmentsMismatch = true;
	}
	else if (gotssegs) {
		// Both shapes have segments
		if (sinf.ssfFile != tinf.ssfFile) {
			// Segment definition file path differs
			e.segmentsMismatch = true;
		}

		if (sinf.segs.size() != tinf.segs.size()) {
			// Shapes have different amount of segments
			e.segmentsMismatch = true;
		}

		for (int si = 0; !e.segmentsMismatch && si < sinf.segs.size(); ++si) {
			if (sinf.segs[si].subs.size() != tinf.segs[si].subs.size()) {
				// Shapes have different amount of sub segments
				e.segmentsMismatch = true;
			}

			for (int ssi = 0; !e.segmentsMismatch && ssi < sinf.segs[si].subs.size(); ++ssi) {
				const NifSubSegmentInfo &sssinf = sinf.segs[si].subs[ssi];
				const NifSubSegmentInfo &tssinf = tinf.segs[si].subs[ssi];
				if (sssinf.userSlotID != tssinf.userSlotID ||
					sssinf.material != tssinf.material ||
					sssinf.extraData != tssinf.extraData) {
					// Sub segment information differs
					e.segmentsMismatch = true;
				}
			}
		}
	}

	std::vector<BSDismemberSkinInstance::PartitionInfo> spinf, tpinf;
	bool gotspar = workNif.GetShapePartitions(source, spinf, triParts);
	bool gottpar = workNif.GetShapePartitions(target, tpinf, triParts);
	if (gotspar != gottpar) {
		// Shape with partitions and the other without
		e.partitionsMismatch = true;
	}
	else if (gotspar) {
		// Both shapes have partitions
		if (spinf.size() != tpinf.size()) {
			// Shapes have different amount of partitions
			e.partitionsMismatch = true;
		}

		for (int pi = 0; !e.partitionsMismatch && pi < spinf.size(); ++pi) {
			if (spinf[pi].partID != tpinf[pi].partID) {
				// Partition slot differs
				e.partitionsMismatch = true;
			}
		}
	}

	auto sShader = workNif.GetShader(source);
	auto tShader = workNif.GetShader(target);
	if ((sShader != nullptr) != (tShader != nullptr)) {
		// Shape with shader and the other without
		e.shaderMismatch = true;
	}
	else if (sShader) {
		// Both shapes have a shader
		if (sShader->GetShaderType() != tShader->GetShaderType()) {
			// Shader type differs
			e.shaderMismatch = true;
		}
		else if (workNif.GetHeader().GetVersion().IsFO4()) {
			if (!StringsEqualInsens(sShader->GetName().c_str(), tShader->GetName().c_str()) ||
				!StringsEqualInsens(sShader->GetWetMaterialName().c_str(), tShader->GetWetMaterialName().c_str())) {
				// Material file paths differ
				e.shaderMismatch = true;
			}
		}

		std::string sTexBase, tTexBase;
		workNif.GetTextureSlot(sShader, sTexBase);
		workNif.GetTextureSlot(tShader, tTexBase);

		if (!StringsEqualInsens(sTexBase.c_str(), tTexBase.c_str())) {
			// Base texture path differs (and possibly UV layout)
			e.textureMismatch = true;
		}
	}

	auto sAlphaProp = workNif.GetAlphaProperty(source);
	auto tAlphaProp = workNif.GetAlphaProperty(target);
	if ((sAlphaProp != nullptr) != (tAlphaProp != nullptr)) {
		// Shape with alpha property and the other without
		e.alphaPropMismatch = true;
	}
	else if (sAlphaProp) {
		// Both shapes have an alpha property
		if (sAlphaProp->flags != tAlphaProp->flags ||
			sAlphaProp->threshold != tAlphaProp->threshold) {
			// Flags or threshold differs
			e.alphaPropMismatch = true;
		}
	}

	e.canMerge =
		!e.partitionsMismatch && !e.segmentsMismatch &&
		!e.tooManyVertices && !e.tooManyTriangles &&
		!e.shaderMismatch && !e.textureMismatch && !e.alphaPropMismatch;
}

void OutfitProject::PrepareCopyGeo(NiShape *source, NiShape *target, UndoStateShape &uss) {
	if (!source || !target)
		return;

	size_t snVerts = source->GetNumVertices();
	size_t tnVerts = target->GetNumVertices();
	size_t snTris = source->GetNumTriangles();
	size_t tnTris = target->GetNumTriangles();

	std::vector<int> vinds(snVerts);
	std::vector<int> tinds(snTris);
	for (int i = 0; i < snVerts; ++i)
		vinds[i] = i;
	for (int i = 0; i < snTris; ++i)
		tinds[i] = i;

	CollectVertexData(source, uss, vinds);
	CollectTriangleData(source, uss, tinds);

	for (int vi = 0; vi < snVerts; ++vi)
		uss.delVerts[vi].index += tnVerts;

	for (int ti = 0; ti < snTris; ++ti) {
		uss.delTris[ti].index += tnTris;
		uss.delTris[ti].t.p1 += tnVerts;
		uss.delTris[ti].t.p2 += tnVerts;
		uss.delTris[ti].t.p3 += tnVerts;
	}

	uss.delVerts.swap(uss.addVerts);
	uss.delTris.swap(uss.addTris);
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
	if (!baseShape) {
		int res = wxMessageBox(_("No reference has been loaded.  For correct bone transforms, you might need to load a reference before importing OBJ files.  Import anyway?"), _("Import without reference"), wxYES_NO);
		if (res == wxNO)
			return 1;
	}

	bool copyBaseSkinTrans = false;
	if (baseShape && !workAnim.shapeSkinning[baseShape->GetName()].xformGlobalToSkin.IsNearlyEqualTo(MatTransform())) {
		int res = wxMessageBox(_("The reference shape has a skin coordinate system that is different from the global coordinate system.  Would you like to copy the reference's global-to-skin transform to the imported shapes?"), _("Copy skin coordinates"), wxYES_NO | wxCANCEL);
		if (res == wxCANCEL)
			return 1;
		if (res == wxYES)
			copyBaseSkinTrans = true;
	}

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

		if (copyBaseSkinTrans)
			workAnim.shapeSkinning[useShapeName].xformGlobalToSkin = workAnim.shapeSkinning[baseShape->GetName()].xformGlobalToSkin;
	}

	return 0;
}

int OutfitProject::ExportOBJ(const std::string& fileName, const std::vector<NiShape*>& shapes, bool transToGlobal, const Vector3& scale, const Vector3& offset) {
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

		std::vector<Vector3> gVerts, gNorms;
		if (transToGlobal) {
			MatTransform toGlobal = workAnim.shapeSkinning[shape->GetName()].xformGlobalToSkin.InverseTransform();
			gVerts.resize(verts->size());
			for (int i = 0; i < gVerts.size(); ++i)
				gVerts[i] = toGlobal.ApplyTransform((*verts)[i]);
			verts = &gVerts;
			if (norms) {
				gNorms.resize(norms->size());
				for (int i = 0; i < gNorms.size(); ++i)
					gNorms[i] = toGlobal.rotation * (*norms)[i];
				norms = &gNorms;
			}
		}

		obj.AddGroup(shape->GetName(), *verts, tris, uvs ? *uvs : std::vector<Vector2>(), norms ? *norms : std::vector<Vector3>());
	}

	if (obj.Save(fileName))
		return 4;

	return 0;
}

int OutfitProject::ImportFBX(const std::string& fileName, const std::string& shapeName, NiShape* mergeShape) {
	if (!baseShape) {
		int res = wxMessageBox(_("No reference has been loaded.  For correct bone transforms, you might need to load a reference before importing FBX files.  Import anyway?"), _("Import without reference"), wxYES_NO);
		if (res == wxNO)
			return 1;
	}

	bool copyBaseSkinTrans = false;
	if (baseShape && !workAnim.shapeSkinning[baseShape->GetName()].xformGlobalToSkin.IsNearlyEqualTo(MatTransform())) {
		int res = wxMessageBox(_("The reference shape has a skin coordinate system that is different from the global coordinate system.  Would you like to copy the reference's global-to-skin transform to the imported shapes?"), _("Copy skin coordinates"), wxYES_NO | wxCANCEL);
		if (res == wxCANCEL)
			return 1;
		if (res == wxYES)
			copyBaseSkinTrans = true;
	}

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

		CreateNifShapeFromData(useShapeName, fbxShape->verts, fbxShape->tris, fbxShape->uvs, &fbxShape->normals);

		if (copyBaseSkinTrans)
			workAnim.shapeSkinning[useShapeName].xformGlobalToSkin = workAnim.shapeSkinning[baseShape->GetName()].xformGlobalToSkin;

		for (auto &bn : fbxShape->boneNames) {
			if (!AnimSkeleton::getInstance().GetBonePtr(bn)) {
				// Not found in reference skeleton, use default values
				AnimBone& cstm = AnimSkeleton::getInstance().AddCustomBone(bn);
				// TODO: call SetParentBone (FbxNode::GetParent?)
				// TODO: call SetTransformBoneToParent (FbxNode::LclTranslation and LclRotation?)
				nonRefBones += bn + "\n";
			}

			workAnim.AddShapeBone(useShapeName, bn);
			workAnim.SetWeights(useShapeName, bn, fbxShape->boneSkin[bn].GetWeights());
		}

		if (!nonRefBones.empty())
			wxLogMessage("Bones in shape '%s' not found in reference skeleton:\n%s", useShapeName, nonRefBones);
	}

	// Set reference NIF in case nothing was loaded yet
	if (!workAnim.GetRefNif())
		workAnim.SetRefNif(&workNif);

	return 0;
}

int OutfitProject::ExportFBX(const std::string& fileName, const std::vector<NiShape*>& shapes, bool transToGlobal) {
	FBXWrangler fbxw;
	fbxw.AddSkeleton(&AnimSkeleton::getInstance().refSkeletonNif);

	for (auto &s : shapes) {
		fbxw.AddNif(&workNif, &workAnim, transToGlobal, s);
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
			s->SetTransformToParent(MatTransform());

			// Clear overall skin transform
			workAnim.shapeSkinning[s->GetName()].xformGlobalToSkin.Clear();
			workNif.SetShapeTransformGlobalToSkin(s, MatTransform());
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
			rootNode->SetTransformToParent(MatTransform());
	}
}

void OutfitProject::RemoveSkinning() {
	for (auto &s : workNif.GetShapes()) {
		workNif.DeleteSkinning(s);
		workAnim.ClearShape(s->GetName());
	}

	workNif.DeleteUnreferencedNodes();
}
